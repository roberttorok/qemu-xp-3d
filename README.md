# qemu-xp-3d

3D acceleration for Windows XP guests in QEMU: OpenGL, DirectDraw and Direct3D games draw on your host's real GPU. You don't need GPU passthrough or a second GPU. It works on Windows and Linux hosts.

```bash
git clone https://github.com/roberttorok/qemu-xp-3d.git
cd qemu-xp-3d
```

Then follow **Windows host** or **Linux host**, then **Inside XP**.

## No warranty

This is a hobby project, made for fun. It comes with no warranty and no support, and we take no responsibility for anything it does. If you lose a VM, a disk image, a game save or anything else while following this guide, that's on you. Keep backups of anything you care about.

## Dependencies

You download each of these yourself in the steps below, and then `make` compiles them.

| Component | Why it's needed |
|---|---|
| [QEMU](https://www.qemu.org/) 11.0.1, Windows only | Our patch adds `ivshmem-win`, the shared-memory card SharedGL needs and Windows's QEMU lacks. Linux uses the distribution's QEMU unchanged |
| [SharedGL](https://github.com/roberttorok/sharedgl), the `windows-support` branch | It carries OpenGL calls from XP to your real GPU. This fork of [dmaivel's SharedGL](https://github.com/dmaivel/sharedgl) adds XP guests and Windows hosts, major speedups, and fixes for crashes, rendering bugs and missing OpenGL functions |
| [wine9x](https://github.com/JHRobotics/wine9x) 1.7.55.45 | It turns Direct3D 8 and 9 into OpenGL, so Direct3D games can reach SharedGL |
| [cnc-ddraw](https://github.com/FunkyFr3sh/cnc-ddraw) 7.1.0.0 | It fixes the wrong colors in 16-bit DirectDraw games |
| [QXL display driver](https://www.spice-space.org/download.html) 0.1-21 | It's the SPICE project's driver for QEMU's own paravirtual display card, it gives XP 32-bit color at high resolutions, and it's the card this whole setup was built and tuned on |

## What's in this repository

| File | What it is |
|---|---|
| `Makefile` | Compiles everything and builds the helper CD for XP, `guest.iso` |
| `windows/run-xp.sh`, `linux/run-xp.sh` | Start the SharedGL server and the VM, with the tuned QEMU settings |
| `windows/patches/qemu/qemu-all.patch` | QEMU's patch, for Windows hosts only |
| `guest/` | Batch files, settings and test programs for inside XP |

## Windows host

Keep this folder on a short path: QEMU's configure step failed from one about 100 characters long.

### 1. Install the tools

Turn on **Windows Hypervisor Platform** in Windows features and reboot, then install [MSYS2](https://www.msys2.org/).

Run everything below in the **UCRT64** terminal, from this folder. QEMU and the SharedGL server both build there.

```bash
pacman -S --needed git make patch diffutils unzip xorriso \
  mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-python \
  mingw-w64-ucrt-x86_64-glib2 mingw-w64-ucrt-x86_64-pixman mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-libepoxy \
  mingw-w64-ucrt-x86_64-spice mingw-w64-ucrt-x86_64-spice-protocol \
  mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-libepoxy \
  mingw-w64-i686-gcc mingw-w64-i686-binutils
```

### 2. QEMU

Download it, unpack it and apply our patch. `tar` prints `Cannot create symlink` errors and reports failure. This is expected: those links are in parts of QEMU we don't build.

```bash
curl -L -o qemu-11.0.1.tar.xz https://download.qemu.org/qemu-11.0.1.tar.xz
tar xf qemu-11.0.1.tar.xz
cd qemu-11.0.1 && git apply ../windows/patches/qemu/qemu-all.patch && cd ..
```

### 3. SharedGL

Clone and check out the `windows-support` branch. It already contains all the changes, so there's nothing to patch.

```bash
git clone https://github.com/roberttorok/sharedgl.git
cd sharedgl && git checkout windows-support && cd ..
```

### 4. wine9x

This is prebuilt, so just download it.

```bash
curl -L -o wine9x-1.7.55.45-sse3.zip https://github.com/JHRobotics/wine9x/releases/download/v1.7.55.45/wine9x-1.7.55.45-sse3.zip
```

### 5. cnc-ddraw

This is prebuilt too. Download it and its license.

```bash
curl -L -o cnc-ddraw-v7.1.0.0.zip https://github.com/FunkyFr3sh/cnc-ddraw/releases/download/v7.1.0.0/cnc-ddraw.zip && curl -L -o cnc-ddraw-LICENSE https://raw.githubusercontent.com/FunkyFr3sh/cnc-ddraw/541b5de218ec3fbd6ea91e606ebfadc07c1786b0/LICENSE
```

### 6. QXL

This is prebuilt too. Later versions have no XP download. The build puts it on the helper CD.

```bash
curl -L -o qxl_xp_x86.zip https://www.spice-space.org/download/windows/qxl/qxl-0.1-21/qxl_xp_x86.zip
```

### 7. Build everything

This compiles what you downloaded, and takes around 5 minutes. Linker warnings like `resolving _glFlush@0` are expected.

```bash
make
```

### 8. Result

On the host, `make` creates:

| On the host | What it is |
|---|---|
| `qemu-11.0.1/build/qemu-system-x86_64.exe` | QEMU with `ivshmem-win`, the shared-memory card |
| `qemu-11.0.1/build/qemu-img.exe` | Creates and converts disk images |
| `sharedgl/build-win/sglrenderer.exe` | The SharedGL server, which draws on your GPU |
| `guest.iso` | The helper CD for XP, described below |
| `build-xp/`, `guest/cd/` | Intermediate files for `guest.iso`. You don't need them |

When you start the VM, the server writes its messages to `server.log`.

`guest.iso` is the helper CD for XP. It contains:

| On the CD | What it is |
|---|---|
| `INSTALL.BAT` | Copies the 3D DLLs into a game's folder and applies `DXVERSION.REG` |
| `DXVERSION.REG` | Makes XP report DirectX 8.1 |
| `opengl32.dll` | SharedGL's client, the replacement OpenGL DLL that games load |
| `sgldrv.sys`, `sgldrv.inf` | The kernel driver that maps the shared memory into games |
| `sglinstall.exe` | Installs that driver |
| `sgldiag.exe` | Checks the path from XP to the server's shared memory |
| `COPYTEST.BAT`, `sglgears.exe` | Copies the test files to `C:\SGLTEST` and `C:\SGLDX`, and an OpenGL test that reports FPS |
| `DX\` | wine9x: `d3d8.dll`, `d3d9.dll`, `ddraw.dll`, `wined3d.dll`, plus `cube8.exe` and `cube9.exe` to test Direct3D |
| `DDRAW\` | cnc-ddraw and its settings, for 16-bit DirectDraw games with wrong colors |
| `QXL\` | The display driver |
| `sglbench.exe`, `sglcost.exe`, `sglmode.exe`, `sglmouse.exe` | Diagnostics for VM overhead, display modes and the mouse. You won't normally need them |

### 9. Create the VM disk

Skip this if you already have an XP disk image. The check is there because `qemu-img create` silently replaces an existing file, and with it an installed XP. It has no option to refuse.

```bash
if [ -e winxp.qcow2 ]; then echo "winxp.qcow2 already exists. Not overwriting it."; else qemu-11.0.1/build/qemu-img.exe create -f qcow2 winxp.qcow2 40G; fi
```

### 10. Start the VM

The script starts the SharedGL server, then the VM, and stops the server when you shut XP down. The second argument is optional: a CD image, such as XP setup or a game.

```bash
./windows/run-xp.sh winxp.qcow2 /c/path/to/game.iso
```

In XP, `D:` is that CD and `E:` is the helper CD. The VM boots from the disk, or from the CD if the disk is still empty.

Games are locked to 60 fps. Add `fps_cap=N` after the disk and CD to change it, or `fps_cap=0` for unlimited. See **Frame rate limit** below.

You can change `RAM` and `CPUS` at the top of the script. **Keep everything else exactly as it is.** It was tuned for XP on this setup, and changing it can cost a lot of speed or stop XP finding its drivers:

- **`kernel-irqchip=off`:** QEMU only patches XP's slow interrupt-priority (TPR) accesses with its own APIC. In a demanding game this took about 35 FPS to about 56-60.
- **`SDL_RENDER_VSYNC=0`:** waiting for vsync holds QEMU's global lock and stalls XP.
- **`-display sdl,gl=on`:** QEMU draws the screen with OpenGL itself, and SDL only provides the window. Motion in games looked closest to Linux's `gtk,gl=on` this way. SDL's own OpenGL renderer (`SDL_RENDER_DRIVER=opengl`) and GTK with `gl=on` crash QEMU on Windows.
- **Device order:** it sets the PCI slots. If `ivshmem-win` moves ahead of `qxl-vga`, XP's display and 3D drivers no longer find their cards.
- **`-cpu Opteron_G3`:** the XP files are compiled for this CPU's instruction set.
- **`-m 64` and `size=64M`:** the server's shared memory and the card's must match.
- **`-machine pc`, IDE, `AC97` and USB:** XP has built-in drivers for these.
- **`ram_size_mb=256` on `qxl-vga`:** each frame passes through the display card's working memory. With the default 64 MB it ran out several times a second and caused micro-stutter.
- **`hpet=off`, `driftfix=slew`, `disable_s3` and `disable_s4`:** these are the timer and ACPI settings everything was measured with.

Now go to **Inside XP**.

## Linux host

### 1. Install the tools

```bash
sudo apt install build-essential cmake git curl patch python3 mingw-w64 mingw-w64-tools \
  libsdl2-dev libepoxy-dev genisoimage unzip qemu-system-x86 qemu-utils
```

Your user needs access to `/dev/kvm`.

### 2. SharedGL

Clone our fork and check out the `windows-support` branch. It already contains all the changes, so there's nothing to patch.

```bash
git clone https://github.com/roberttorok/sharedgl.git
cd sharedgl && git checkout windows-support && cd ..
```

### 3. wine9x

```bash
curl -L -o wine9x-1.7.55.45-sse3.zip https://github.com/JHRobotics/wine9x/releases/download/v1.7.55.45/wine9x-1.7.55.45-sse3.zip
```

### 4. cnc-ddraw

```bash
curl -L -o cnc-ddraw-v7.1.0.0.zip https://github.com/FunkyFr3sh/cnc-ddraw/releases/download/v7.1.0.0/cnc-ddraw.zip && curl -L -o cnc-ddraw-LICENSE https://raw.githubusercontent.com/FunkyFr3sh/cnc-ddraw/541b5de218ec3fbd6ea91e606ebfadc07c1786b0/LICENSE
```

### 5. QXL

```bash
curl -L -o qxl_xp_x86.zip https://www.spice-space.org/download/windows/qxl/qxl-0.1-21/qxl_xp_x86.zip
```

### 6. Build everything

```bash
make
```

### 7. Result

On the host, `make` creates:

| On the host | What it is |
|---|---|
| `sharedgl/build-host/sglrenderer` | The SharedGL server, which draws on your GPU |
| `guest.iso` | The helper CD for XP, described in the Windows section's **Result** step |
| `build-xp/`, `guest/cd/` | Intermediate files for `guest.iso`. You don't need them |

QEMU is your distribution's. When you start the VM, the server writes its messages to `server.log`.

### 8. Create the VM disk

Skip this if you already have an XP disk image. The check is there because `qemu-img create` silently replaces an existing file, and with it an installed XP. It has no option to refuse.

```bash
if [ -e winxp.qcow2 ]; then echo "winxp.qcow2 already exists. Not overwriting it."; else qemu-img create -f qcow2 winxp.qcow2 40G; fi
```

### 9. Start the VM

The script starts the SharedGL server, then the VM, and stops the server when you shut XP down. The second argument is optional: a CD image, such as XP setup or a game.

```bash
./linux/run-xp.sh winxp.qcow2 /path/to/game.iso
```

In XP, `D:` is that CD and `E:` is the helper CD. The VM boots from the disk, or from the CD if the disk is still empty.

Games are locked to 60 fps. Add `fps_cap=N` after the disk and CD to change it, or `fps_cap=0` for unlimited. See **Frame rate limit** below.

You can change `RAM` and `CPUS` at the top of the script. **Keep everything else exactly as it is**, for the reasons given in the Windows section's **Start the VM** step. The APIC setting and `SDL_RENDER_VSYNC` apply only to Windows.

## Inside XP

These commands run in XP's own command prompt. The helper CD is `E:`.

### 1. Install the display driver

The driver is on the helper CD, in `E:\QXL`. In Device Manager, update the display adapter's driver and point it at that folder. Restart XP, then set the display to 32-bit color and the resolution you want.

### 2. Install the 3D driver

Open a command prompt and run:

```bat
E:\sglinstall.exe
E:\sgldiag.exe
```

The first should print `Installed.`. The last line of the second should show `gl 4.6`. If you see `problem=10` or `enum FAILED`, the driver isn't running.

### 3. Test 3D

```bat
E:\COPYTEST.BAT
cd /d C:\SGLTEST
sglgears.exe
```

Gears should spin on screen, and the `GL_RENDERER` line in the command prompt should name your real GPU. Close the window to stop it.

### 4. Install a game

Install the game normally from its CD. Then run `E:\INSTALL.BAT` with the folder holding the game's `.exe`:

```bat
E:\INSTALL.BAT "C:\Program Files\My Game"
```

It works for OpenGL, Direct3D and DirectDraw games alike. It copies all the 3D DLLs there, overwriting any with the same names, and sets the DirectX version XP reports to 8.1, because some programs refuse to start otherwise. You can run it as often as you like. It refuses `system32`, where these files must never go.

If a 16-bit DirectDraw game shows wrong colors, run this as well. It replaces `ddraw.dll` with cnc-ddraw and adds its settings. The CPU then draws that game, not your GPU.

```bat
copy /y E:\DDRAW\*.* "C:\Program Files\My Game"
```

### Frame rate limit

**Games are locked to 60 fps by default.** Frames beyond the screen's refresh rate are never seen, but each one costs host CPU. The lock also makes motion smoother.

The limit is set when you start the VM, never inside XP or by the game. **For unlimited fps**, add `fps_cap=0`:

```bash
./linux/run-xp.sh winxp.qcow2 fps_cap=0
./windows/run-xp.sh winxp.qcow2 fps_cap=0
```

Any other number sets that limit, such as `fps_cap=144` for a 144 Hz monitor or `fps_cap=30` to save more CPU.

The cached display memory needs the current `sgldrv.sys` in XP: after updating from an older build, run `E:\sglinstall.exe` again.

## Credits

[SharedGL](https://github.com/dmaivel/sharedgl) by dmaivel, [wine9x](https://github.com/JHRobotics/wine9x) by JHRobotics, [cnc-ddraw](https://github.com/FunkyFr3sh/cnc-ddraw) by FunkyFr3sh, the [QXL driver](https://www.spice-space.org/) by the SPICE project, and [QEMU](https://www.qemu.org/).

## License

This repository's own files are under the MIT license in `LICENSE`. `windows/patches/qemu/qemu-all.patch` changes QEMU, so it's under QEMU's license, the GPL version 2. The components you download keep their own licenses.

Not affiliated with or endorsed by the QEMU project or Microsoft. QEMU is a trademark of Fabrice Bellard. Windows and Windows XP are trademarks of Microsoft.
