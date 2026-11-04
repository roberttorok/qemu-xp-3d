#!/usr/bin/env bash
# Starts the SharedGL server and the XP VM, and stops the server when XP shuts down.
#
#   ./linux/run-xp.sh DISK.qcow2 [CD.iso] [fps_cap=N]
#
# In XP, D: is the CD you pass here (XP setup or a game), and E: is the helper CD.
# The VM boots from the disk, or from the CD if the disk is still empty.
#
# fps_cap=N sets the frame limit for 3D games (default 60, 0 = unlimited). It is decided
# here on the host; nothing inside XP or the game can change it. Frames above the
# screen's refresh are never seen but cost host CPU.

# You can change these two.
RAM=2G
CPUS=2

# Keep everything below as it is. In particular:
#   device order          it sets the PCI slots, and XP's drivers look for their cards there
#   -cpu Opteron_G3       the XP files are compiled for this CPU
#   -machine pc, IDE, AC97, USB    XP has built-in drivers for these
#   64 MB of shared memory    the server's -m and the memory backend's size must match
#   hpet=off, driftfix=slew, lost_tick_policy, disable_s3, disable_s4    the timer and ACPI settings everything was measured with
#   ram_size_mb=256       QXL working memory. Each frame is copied in, and QEMU frees it in batches of 32; the default 64 MB ran out every ~16 frames and stalled XP: visible micro-stutter
# QEMU_EXTRA in the environment adds more QEMU arguments, such as -snapshot.

# Settings come after the disk and CD, as name=value.
FPS_CAP=60
ARGS=()
for arg in "$@"; do
  case "$arg" in
    fps_cap=*)   FPS_CAP=${arg#fps_cap=} ;;
    *=*)         [ -f "$arg" ] && ARGS+=("$arg") || { echo "Unknown setting: $arg"; exit 1; } ;;
    *)           ARGS+=("$arg") ;;
  esac
done
set -- "${ARGS[@]}"
[[ "$FPS_CAP" =~ ^[0-9]+$ ]] && [ "$FPS_CAP" -le 1000 ] || { echo "fps_cap takes a number from 0 (no limit) to 1000"; exit 1; }

[ -n "$1" ] || { echo "Usage: $0 DISK.qcow2 [CD.iso] [fps_cap=N]"; exit 1; }
[ -f "$1" ] || { echo "No disk at $1"; exit 1; }
[ -z "$2" ] || [ -f "$2" ] || { echo "No CD image at $2"; exit 1; }

# Absolute paths, with commas doubled because QEMU treats a comma as a separator.
qemu_path() { local p; p="$(realpath "$1")"; echo "${p//,/,,}"; }
DISK="$(qemu_path "$1")"
CD=""
[ -z "$2" ] || CD=",format=raw,file=$(qemu_path "$2")"
cd "$(dirname "$0")/.." || exit 1

./sharedgl/build-host/sglrenderer -m 64 -f "$FPS_CAP" > server.log 2>&1 &
SERVER=$!
trap 'kill $SERVER 2> /dev/null' EXIT
sleep 1

qemu-system-x86_64 \
  -accel kvm -machine pc,hpet=off,vmport=off,usb=on \
  -cpu Opteron_G3 -smp $CPUS,sockets=1,cores=$CPUS,threads=1 -m $RAM -rtc base=localtime,driftfix=slew \
  -global kvm-pit.lost_tick_policy=delay -global PIIX4_PM.disable_s3=1 -global PIIX4_PM.disable_s4=1 \
  -drive "if=none,id=hd,format=qcow2,cache=none,aio=native,discard=unmap,detect-zeroes=unmap,file=$DISK" -device ide-hd,bus=ide.0,drive=hd,bootindex=0 \
  -drive "if=none,id=cd,media=cdrom,readonly=on$CD" -device ide-cd,bus=ide.1,unit=0,drive=cd,bootindex=1 \
  -drive if=none,id=helper,media=cdrom,readonly=on,format=raw,file=guest.iso -device ide-cd,bus=ide.1,unit=1,drive=helper \
  -device qxl-vga,vgamem_mb=32,ram_size_mb=256 \
  -object memory-backend-file,id=sgl,size=64M,share=on,mem-path=/dev/shm/sharedgl_shared_memory -device ivshmem-plain,memdev=sgl \
  -display gtk,gl=on,zoom-to-fit=on \
  -audiodev pipewire,id=snd -device AC97,audiodev=snd \
  -device usb-tablet -nic none ${QEMU_EXTRA:-}
