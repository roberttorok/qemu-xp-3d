# Compiles what you have downloaded and cloned. See README.md.
# This file never downloads, clones or patches anything. If something is missing, it stops and says so.
#
#   make            everything below, in order
#   make qemu       configures and compiles QEMU, on Windows only
#   make server     the program on the host that draws on your GPU
#   make opengl32   the replacement opengl32.dll for XP
#   make driver     the XP kernel driver and its installer
#   make programs   small test programs for XP
#   make wine9x     unpacks the prebuilt Direct3D DLLs
#   make cnc-ddraw  unpacks the prebuilt DirectDraw DLL
#   make qxl        unpacks the prebuilt QXL display driver
#   make cd         adds the helper .BAT files and puts all the XP files on a CD image, guest.iso

CD  := guest/cd
OBJ := build-xp
SGL := sharedgl
XPK := $(SGL)/kernel/winxp

ifeq ($(OS),Windows_NT)
  # XP is 32-bit, so its files need MSYS2's 32-bit compiler, which has to find its own DLLs first.
  CC32   := PATH=/mingw32/bin:/usr/bin gcc
  DDK    := /mingw32/include/ddk
  MKISO  := xorriso -as mkisofs -quiet
  # GCC 16 defaults to C23, which this older code doesn't compile as. MSYS2's C library needs XP's printf.
  HOST32 := -std=gnu17 -D__USE_MINGW_ANSI_STDIO=0
  DRIVER_STD := -std=gnu17
else
  CC32   := i686-w64-mingw32-gcc
  DDK    := /usr/share/mingw-w64/include/ddk
  MKISO  := genisoimage -quiet
  HOST32 :=
  DRIVER_STD :=
endif

# k8-sse3 matches the VM's virtual CPU. 0x0501 means "target Windows XP".
XPFLAGS := $(HOST32) -march=k8-sse3 -mtune=generic -mfpmath=sse -mstackrealign -O2 -s -D_WIN32_WINNT=0x0501 -DWINVER=0x0501

.PHONY: all qemu server opengl32 driver programs wine9x cnc-ddraw qxl cd check-qemu check-sharedgl

ifeq ($(OS),Windows_NT)
all: qemu server cd
else
all: server cd
endif


# ---- Checks that the README's download, clone and patch steps were done ----

check-qemu:
	@test -f qemu-11.0.1/configure || { echo "QEMU's source is missing. Do the download step for QEMU in README.md."; exit 1; }
	@test -f qemu-11.0.1/hw/misc/ivshmem-win.c || { echo "QEMU isn't patched. Do the patch step for QEMU in README.md."; exit 1; }

check-sharedgl:
	@test -d $(SGL)/src || { echo "SharedGL's source is missing. Do the clone step for SharedGL in README.md."; exit 1; }
	@test -f $(XPK)/sgldrv.c || { echo "This isn't the windows-support branch of SharedGL. Do the clone step for SharedGL in README.md."; exit 1; }


# ---- QEMU, for a Windows host ----

QEMU_BUILD := qemu-11.0.1/build

qemu: check-qemu $(QEMU_BUILD)/build.ninja
	cd $(QEMU_BUILD) && ninja qemu-system-x86_64.exe qemu-img.exe
	$(QEMU_BUILD)/qemu-system-x86_64.exe -device help | grep ivshmem-win

$(QEMU_BUILD)/build.ninja:
	mkdir -p $(QEMU_BUILD)
	cd $(QEMU_BUILD) && ../configure --target-list=x86_64-softmmu --enable-whpx --enable-sdl --enable-spice --enable-opengl --disable-werror --disable-docs --disable-gtk --disable-vnc --disable-capstone --disable-guest-agent


# ---- The server, for the host ----

SERVER_SRC := $(addprefix $(SGL)/src/server/,context.c dynarr.c frameprof.c main.c overlay.c processor.c sgldebug.c) $(SGL)/src/client/scratch.c

ifeq ($(OS),Windows_NT)
server: check-sharedgl $(SGL)/build-win/sglrenderer.exe
$(SGL)/build-win/sglrenderer.exe: $(SERVER_SRC)
	mkdir -p $(@D)
	# UCRT64, like everything else on Windows.
	PATH=/ucrt64/bin:/usr/bin gcc -O2 -std=gnu17 -w -DSDL_MAIN_HANDLED -I$(SGL)/inc $(SERVER_SRC) -o $@ -lSDL2 -lepoxy -lws2_32 -lwinmm -liphlpapi
else
server: check-sharedgl
	cmake -S $(SGL) -B $(SGL)/build-host -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-Wno-incompatible-pointer-types -Wno-int-conversion -Wno-implicit-function-declaration"
	cmake --build $(SGL)/build-host --target sglrenderer -j
endif


# ---- opengl32.dll, for XP ----

CLIENT_OBJ := $(addprefix $(OBJ)/,winmain.o pb.o spinlock.o glimpl.o scratch.o windrv.o)
CLIENT_DEP := $(wildcard $(SGL)/inc/client/*.h) $(SGL)/inc/sglprof.h $(SGL)/src/client/platform/xpwgl.inl

opengl32: check-sharedgl $(CD)/opengl32.dll

$(CD)/opengl32.dll: $(CLIENT_OBJ)
	mkdir -p $(@D)
	$(CC32) -shared -s -o $@ $^ -Wl,--kill-at -Wl,--export-all-symbols -static-libgcc -lsetupapi -lgdi32 -luser32 -lwinmm

# GL functions must be stdcall, and -mrtd does that for the whole file. It would also make GCC
# treat its own fprintf and friends as stdcall, which corrupts the stack, so those are turned off.
$(OBJ)/glimpl.o: EXTRA := -mrtd -fno-builtin-fprintf -fno-builtin-printf -fno-builtin-fputs -fno-builtin-fputc -fno-builtin-puts -fno-builtin-putchar

$(OBJ)/%.o: $(SGL)/src/client/%.c $(CLIENT_DEP)
	mkdir -p $(@D)
	$(CC32) $(XPFLAGS) -I$(SGL)/inc -w -DSGL_XP -DSGL_NO_NETWORK $(EXTRA) -c $< -o $@

$(OBJ)/windrv.o: $(SGL)/src/client/platform/windrv.c $(CLIENT_DEP)
	mkdir -p $(@D)
	$(CC32) $(XPFLAGS) -I$(SGL)/inc -w -DSGL_XP -DSGL_NO_NETWORK -c $< -o $@


# ---- The kernel driver, for XP ----

driver: check-sharedgl $(CD)/sgldrv.sys $(CD)/sgldrv.inf $(CD)/sglinstall.exe

# A driver has no C library or normal start-up code, and XP requires these image settings.
$(CD)/sgldrv.sys: $(XPK)/sgldrv.c
	mkdir -p $(@D)
	$(CC32) $(DRIVER_STD) -O2 -s -march=pentium2 -w -fno-stack-protector -fno-stack-check -fno-aggressive-loop-optimizations -fno-asynchronous-unwind-tables -D_X86_=1 -I$(DDK) \
	  -shared -nostdlib -nostartfiles -o $@ $< \
	  -Wl,--subsystem,native:5.1 -Wl,--major-os-version,5,--minor-os-version,1 -Wl,--entry,_DriverEntry@8 \
	  -Wl,--enable-reloc-section -Wl,--image-base,0x10000 -Wl,--disable-nxcompat -Wl,--disable-dynamicbase \
	  -lntoskrnl -lhal

$(CD)/sgldrv.inf: $(XPK)/sgldrv.inf
	mkdir -p $(@D)
	cp $< $@

$(CD)/sglinstall.exe: $(XPK)/sglinstall.c
	mkdir -p $(@D)
	$(CC32) $(XPFLAGS) -o $@ $< -lnewdev -static-libgcc


# ---- Test programs, for XP ----

programs:
	mkdir -p $(CD)/DX
	$(CC32) $(XPFLAGS) -o $(CD)/sgldiag.exe  guest/sgldiag.c  -lsetupapi -lcfgmgr32 -static-libgcc
	$(CC32) $(XPFLAGS) -o $(CD)/sglgears.exe guest/sglgears.c -lopengl32 -lgdi32 -luser32 -lm -static-libgcc
	$(CC32) $(XPFLAGS) -o $(CD)/sglmode.exe  guest/sglmode.c  -static-libgcc
	$(CC32) $(XPFLAGS) -o $(CD)/sglbench.exe guest/sglbench.c -lsetupapi -lwinmm -static-libgcc
	$(CC32) $(XPFLAGS) -o $(CD)/sglcost.exe  guest/sglcost.c  -lsetupapi -static-libgcc
	$(CC32) $(XPFLAGS) -o $(CD)/sglmouse.exe guest/sglmouse.c -luser32 -ladvapi32 -static-libgcc
	$(CC32) $(XPFLAGS) -DCOBJMACROS -o $(CD)/DX/cube9.exe guest/cube.c -ld3d9 -lgdi32 -luser32 -lm -static-libgcc
	$(CC32) $(XPFLAGS) -DCOBJMACROS -DUSE_D3D8 -o $(CD)/DX/cube8.exe guest/cube.c -ld3d8 -lgdi32 -luser32 -lm -static-libgcc


# ---- Prebuilt third-party DLLs, for XP ----

WINE9X := wine9x-1.7.55.45-sse3.zip
DDRAW  := cnc-ddraw-v7.1.0.0.zip

wine9x:
	@test -f $(WINE9X) || { echo "$(WINE9X) is missing. Do the download step for wine9x in README.md."; exit 1; }
	mkdir -p $(CD)/DX
	# wine9x names its DLLs wined8, wined9 and winedd. Games look for the Microsoft names.
	unzip -q -o -j $(WINE9X) wined3d.dll license.txt -d $(CD)/DX
	unzip -p $(WINE9X) wined8.dll > $(CD)/DX/d3d8.dll
	unzip -p $(WINE9X) wined9.dll > $(CD)/DX/d3d9.dll
	unzip -p $(WINE9X) winedd.dll > $(CD)/DX/ddraw.dll

cnc-ddraw:
	@test -f $(DDRAW) || { echo "$(DDRAW) is missing. Do the download step for cnc-ddraw in README.md."; exit 1; }
	mkdir -p $(CD)/DDRAW
	unzip -p $(DDRAW) ddraw.dll > $(CD)/DDRAW/ddraw.dll
	cp guest/ddraw-16bit-colors.ini $(CD)/DDRAW/ddraw.ini
	cp cnc-ddraw-LICENSE $(CD)/DDRAW/LICENSE


qxl:
	@test -f qxl_xp_x86.zip || { echo "qxl_xp_x86.zip is missing. Do the download step for the QXL driver in README.md."; exit 1; }
	mkdir -p $(CD)/QXL
	unzip -q -o -j qxl_xp_x86.zip 'xp/x86/*' -d $(CD)/QXL


# ---- The CD image ----

cd: opengl32 driver programs wine9x cnc-ddraw qxl
	cp guest/COPYTEST.BAT guest/INSTALL.BAT guest/DXVERSION.REG $(CD)/
	$(MKISO) -J -r -V SHAREDGL -o guest.iso $(CD)
	@echo guest.iso contains: && ls $(CD)
