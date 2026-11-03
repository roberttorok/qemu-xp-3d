#!/usr/bin/env bash
# Starts the SharedGL server and the XP VM, and stops the server when XP shuts down.
#
#   ./windows/run-xp.sh DISK.qcow2 [CD.iso]
#
# In XP, D: is the CD you pass here (XP setup or a game), and E: is the helper CD.
# The VM boots from the disk, or from the CD if the disk is still empty.

# You can change these two.
RAM=2G
CPUS=2

# Keep everything below as it is. In particular:
#   kernel-irqchip=off    QEMU only patches XP's slow interrupt-priority accesses with its own APIC
#   SDL_RENDER_VSYNC=0    waiting for vsync stalls XP
#   device order          it sets the PCI slots, and XP's drivers look for their cards there
#   -cpu Opteron_G3       the XP files are compiled for this CPU
#   -machine pc, IDE, AC97, USB    XP has built-in drivers for these
#   64 MB of shared memory    the server's -m and ivshmem-win's size must match
#   hpet=off, driftfix=slew, disable_s3, disable_s4    the timer and ACPI settings everything was measured with
#   ram_size_mb=256       QXL working memory. Each frame is copied in, and QEMU frees it in batches of 32; the default 64 MB ran out every ~16 frames and stalled XP: visible micro-stutter
# QEMU_EXTRA in the environment adds more QEMU arguments, such as -snapshot.

[ -n "$1" ] || { echo "Usage: $0 DISK.qcow2 [CD.iso]"; exit 1; }
[ -f "$1" ] || { echo "No disk at $1"; exit 1; }
[ -z "$2" ] || [ -f "$2" ] || { echo "No CD image at $2"; exit 1; }

# QEMU needs Windows paths, with commas doubled because it treats a comma as a separator.
qemu_path() { local p; p="$(cygpath -am "$1")"; echo "${p//,/,,}"; }
DISK="$(qemu_path "$1")"
CD=""
[ -z "$2" ] || CD=",format=raw,file=$(qemu_path "$2")"
cd "$(dirname "$0")/.." || exit 1

# The server is a MINGW64 build, so it needs that environment's DLLs. See the Makefile.
PATH=/mingw64/bin:/usr/bin ./sharedgl/build-win/sglrenderer.exe -m 64 > server.log 2>&1 &
trap 'taskkill //f //im sglrenderer.exe > /dev/null 2>&1' EXIT
sleep 1

SDL_RENDER_SCALE_QUALITY=linear SDL_RENDER_VSYNC=0 MSYS2_ARG_CONV_EXCL='*' qemu-11.0.1/build/qemu-system-x86_64.exe -L qemu-11.0.1/pc-bios \
  -accel whpx -machine pc,hpet=off,vmport=off,usb=on,kernel-irqchip=off \
  -cpu Opteron_G3 -smp $CPUS,sockets=1,cores=$CPUS,threads=1 -m $RAM -rtc base=localtime,driftfix=slew \
  -global PIIX4_PM.disable_s3=1 -global PIIX4_PM.disable_s4=1 \
  -drive "if=none,id=hd,format=qcow2,discard=unmap,detect-zeroes=unmap,file=$DISK" -device ide-hd,bus=ide.0,drive=hd,bootindex=0 \
  -drive "if=none,id=cd,media=cdrom,readonly=on$CD" -device ide-cd,bus=ide.1,unit=0,drive=cd,bootindex=1 \
  -drive if=none,id=helper,media=cdrom,readonly=on,format=raw,file=guest.iso -device ide-cd,bus=ide.1,unit=1,drive=helper \
  -device qxl-vga,vgamem_mb=32,ram_size_mb=256 \
  -device 'ivshmem-win,name=Local\sharedgl_shared_memory,size=64M' \
  -display sdl \
  -audiodev sdl,id=snd,in.voices=0 -device AC97,audiodev=snd \
  -device usb-tablet -nic none ${QEMU_EXTRA:-}
