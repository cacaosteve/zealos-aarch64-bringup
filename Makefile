# ZealOS aarch64 Limine bring-up
ROOT     := $(abspath .)
BUILD    := $(ROOT)/build
ESP_DIR  := $(ROOT)/esp
BOOT     := $(BUILD)/zealbooter.elf
KERNEL   := $(BUILD)/kernel.elf
BOOT_PI  := $(BUILD)/zealbooter-pi.elf
KERNEL_PI := $(BUILD)/kernel-pi.elf
DEMO_BC  := $(ESP_DIR)/boot/demo.hcbc
ISO      := $(BUILD)/zealos-aarch64-hello.iso
DISK_IMG := $(ROOT)/build/disk.img
RS_IMG   := $(ROOT)/build/redsea.img
ESP_FAT  := $(BUILD)/esp-fat.img
# Pi SD diagnostic: never silent-fallback to QEMU UART; halt after kernel entry marker.
CFLAGS_PI := -DZEAL_FORCE_PI4 -DZEAL_PI_DIAG

LIMINE_BIN := $(ROOT)/third_party/limine-binary
LIMINE_INC := $(ROOT)/third_party/limine-12.9.0/limine-protocol/include

CLANG    ?= /opt/homebrew/opt/llvm/bin/clang
LLD_FLAG := -fuse-ld=lld
SGDISK   ?= /opt/homebrew/bin/sgdisk

# Match limine-c-template aarch64 flags (ET_EXEC, no PIC/SIMD).
CFLAGS := -target aarch64-unknown-none-elf \
	-std=gnu11 -ffreestanding -fno-builtin -fno-common \
	-fno-stack-protector -fno-stack-check -fno-PIC \
	-ffunction-sections -fdata-sections \
	-mcpu=generic -march=armv8-a+nofp+nosimd -mgeneral-regs-only \
	-mno-outline-atomics -mcmodel=small -mbranch-protection=none \
	-O2 -Wall -Wextra \
	-I$(LIMINE_INC) -I$(ROOT)/src

# Kernel is a Limine module: PIE so ZealBooter can relocate into HHDM RAM.
CFLAGS_KERNEL := -target aarch64-unknown-none-elf \
	-std=gnu11 -ffreestanding -fno-builtin -fno-common \
	-fno-stack-protector -fno-stack-check -fPIE \
	-ffunction-sections -fdata-sections \
	-mcpu=generic -march=armv8-a+nofp+nosimd -mgeneral-regs-only \
	-mno-outline-atomics -mcmodel=tiny -mbranch-protection=none \
	-O2 -Wall -Wextra \
	-I$(ROOT)/src

# Cos/Sin Taylor helpers: same ABI/PIE, but allow FP (kernel_stub stays +nofp).
CFLAGS_F64MATH := -target aarch64-unknown-none-elf \
	-std=gnu11 -ffreestanding -fno-builtin -fno-common \
	-fno-stack-protector -fno-stack-check -fPIE \
	-ffunction-sections -fdata-sections \
	-mcpu=generic -march=armv8-a \
	-mno-outline-atomics -mcmodel=tiny -mbranch-protection=none \
	-O2 -Wall -Wextra \
	-I$(ROOT)/src

LDFLAGS_BOOT := $(LLD_FLAG) -nostdlib -static -Wl,-no-pie \
	-Wl,-m,aarch64elf \
	-Wl,-z,max-page-size=0x1000 \
	-Wl,-z,noexecstack \
	-Wl,--gc-sections \
	-Wl,-T,$(ROOT)/src/linker-booter.ld

LDFLAGS_KERNEL := $(LLD_FLAG) -nostdlib -static -Wl,-pie \
	-Wl,-m,aarch64elf \
	-Wl,-z,max-page-size=0x1000 \
	-Wl,-z,noexecstack \
	-Wl,--gc-sections \
	-Wl,-T,$(ROOT)/src/linker-kernel.ld

BOOT_OBJS   := $(BUILD)/zealbooter.o
KERNEL_OBJS := $(BUILD)/kernel_stub.o $(BUILD)/hc_f64math.o $(BUILD)/vectors.o

QEMU       ?= qemu-system-aarch64
FW_CODE    ?= /opt/homebrew/share/qemu/edk2-aarch64-code.fd
FW_VARS_IN ?= /opt/homebrew/share/qemu/edk2-arm-vars.fd
FW_VARS    := $(BUILD)/edk2-vars.fd

ESP_START_SECTOR := 2048
# 64MiB = 131072 sectors. GPT backup uses the final 34 LBAs (32 PTE + header).
# RedSea (PCI/UTM) is 128 sectors immediately before that backup region.
# EFI ends at RedSea_base-1 so the three regions never overlap.
DISK_SECTS       := 131072
GPT_BACKUP_SECTS := 34
RS_SECTS         := 128
RS_LBA_BASE      := $(shell echo $$(($(DISK_SECTS) - $(GPT_BACKUP_SECTS) - $(RS_SECTS) + 1)))
ESP_END_SECTOR   := $(shell echo $$(($(RS_LBA_BASE) - 1)))
ESP_SECTORS      := $(shell echo $$(($(ESP_END_SECTOR) - $(ESP_START_SECTOR) + 1)))

.PHONY: all clean esp iso run run-serial check-serial run-iso utm pi-sd pi-diag

all: $(BOOT) $(KERNEL) iso esp

$(BUILD):
	mkdir -p $@

$(BUILD)/zealbooter.o: src/zealbooter.c src/handoff.h src/elf64.h src/plat_pi4.h src/mmio_map.h src/linker-booter.ld | $(BUILD)
	$(CLANG) $(CFLAGS) -c src/zealbooter.c -o $@

$(BUILD)/kernel_stub.o: src/kernel_stub.c src/handoff.h src/hc_ir.h src/hc_front.h src/a64_emit.h src/fb_font.h src/virtio_kbd.h src/virtio_tablet.h src/virtio_blk.h src/disk_layout.h src/mmio_map.h $(BUILD)/netofdots_zc.h $(BUILD)/lines_zc.h $(BUILD)/minigr_zc.h $(BUILD)/memsort_zc.h $(BUILD)/peekplot_zc.h $(BUILD)/offbmp_zc.h $(BUILD)/heapstr_zc.h $(BUILD)/catfmt_zc.h $(BUILD)/heapque_zc.h $(BUILD)/jobque_zc.h $(BUILD)/jobrun_zc.h $(BUILD)/taskspawn_zc.h $(BUILD)/popup_zc.h $(BUILD)/doclite_zc.h $(BUILD)/doclib_zc.h $(BUILD)/notes_zc.h $(BUILD)/globshare_zc.h $(BUILD)/life_zc.h $(BUILD)/cartlite_zc.h $(BUILD)/vec2lite_zc.h $(BUILD)/angleslite_zc.h $(BUILD)/coslite_zc.h $(BUILD)/sqrtlite_zc.h $(BUILD)/arglite_zc.h $(BUILD)/commalite_zc.h $(BUILD)/plot3lite_zc.h $(BUILD)/tospilite_zc.h $(BUILD)/colorlite_zc.h $(BUILD)/turtlelite_zc.h $(BUILD)/filllite_zc.h $(BUILD)/initlite_zc.h $(BUILD)/deflite_zc.h $(BUILD)/printlite_zc.h $(BUILD)/msglite_zc.h $(BUILD)/menulite_zc.h $(BUILD)/findlite_zc.h $(BUILD)/fslite_zc.h $(BUILD)/setuplite_zc.h $(BUILD)/ttlite_zc.h $(BUILD)/buflite_zc.h $(BUILD)/inclite_zc.h $(BUILD)/dclite_zc.h $(BUILD)/linedclite_zc.h $(BUILD)/grflite_zc.h $(BUILD)/movelite_zc.h $(BUILD)/checkedlite_zc.h $(BUILD)/cmplite_zc.h $(BUILD)/forinclite_zc.h $(BUILD)/microlite_zc.h $(BUILD)/movestacklite_zc.h $(BUILD)/endlite_zc.h $(BUILD)/drawitlite_zc.h $(BUILD)/latticelite_zc.h $(BUILD)/looplite_zc.h $(BUILD)/demolite_zc.h $(BUILD)/eventlite_zc.h $(BUILD)/playlite_zc.h $(BUILD)/inputlite_zc.h $(BUILD)/rightlite_zc.h $(BUILD)/cursorlite_zc.h $(BUILD)/uplite_zc.h $(BUILD)/ticklite_zc.h $(BUILD)/framelite_zc.h $(BUILD)/plotdclite_zc.h $(BUILD)/abortlite_zc.h $(BUILD)/aimmovelite_zc.h $(BUILD)/idlelite_zc.h $(BUILD)/layerlite_zc.h $(BUILD)/endslite_zc.h $(BUILD)/speedlite_zc.h $(BUILD)/midlite_zc.h $(BUILD)/livelite_zc.h $(BUILD)/accellite_zc.h $(BUILD)/restartlite_zc.h $(BUILD)/widthlite_zc.h $(BUILD)/bothcolorlite_zc.h $(BUILD)/menufulllite_zc.h $(BUILD)/menubiglite_zc.h $(BUILD)/trylite_zc.h $(BUILD)/stepcountlite_zc.h $(BUILD)/anglesfulllite_zc.h $(BUILD)/braceangleslite_zc.h $(BUILD)/bracepilite_zc.h $(BUILD)/setmenulite_zc.h $(BUILD)/nearlatticelite_zc.h $(BUILD)/f64iflite_zc.h $(BUILD)/wraplatticelite_zc.h $(BUILD)/menulooplite_zc.h $(BUILD)/idxalllite_zc.h $(BUILD)/disklat_zc.h $(BUILD)/stocklat_zc.h $(BUILD)/depthbuflite_zc.h $(BUILD)/depthrstlite_zc.h $(BUILD)/depthplotlite_zc.h $(BUILD)/depthlinelite_zc.h $(BUILD)/ramblk_zc.h $(BUILD)/namefile_zc.h $(BUILD)/dirlook_zc.h $(BUILD)/dirdel_zc.h $(BUILD)/fopen_zc.h $(BUILD)/fwrite_zc.h $(BUILD)/multiblk_zc.h $(BUILD)/redsea_zc.h $(BUILD)/rsroot_zc.h $(BUILD)/rsfile_zc.h $(BUILD)/rsalloc_zc.h $(BUILD)/rsfree_zc.h $(BUILD)/rsmulti_zc.h $(BUILD)/rscfile_zc.h $(BUILD)/rscwrite_zc.h $(BUILD)/rscseek_zc.h $(BUILD)/rsclib_zc.h $(BUILD)/rspersist_zc.h $(BUILD)/runzc_zc.h | $(BUILD)
	$(CLANG) $(CFLAGS_KERNEL) -I$(BUILD) -c src/kernel_stub.c -o $@

$(BUILD)/netofdots_zc.h: upstream/NetOfDots.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ NETOFDOTS_ZC

$(BUILD)/lines_zc.h: upstream/Lines.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ LINES_ZC

$(BUILD)/minigr_zc.h: upstream/MiniGr.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ MINIGR_ZC

$(BUILD)/memsort_zc.h: upstream/MemSort.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ MEMSORT_ZC

$(BUILD)/peekplot_zc.h: upstream/PeekPlot.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ PEEKPLOT_ZC

$(BUILD)/offbmp_zc.h: upstream/OffBmp.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ OFFBMP_ZC

$(BUILD)/heapstr_zc.h: upstream/HeapStr.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ HEAPSTR_ZC

$(BUILD)/catfmt_zc.h: upstream/CatFmt.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ CATFMT_ZC

$(BUILD)/heapque_zc.h: upstream/HeapQue.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ HEAPQUE_ZC

$(BUILD)/jobque_zc.h: upstream/JobQue.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ JOBQUE_ZC

$(BUILD)/jobrun_zc.h: upstream/JobRun.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ JOBRUN_ZC

$(BUILD)/taskspawn_zc.h: upstream/TaskSpawn.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ TASKSPAWN_ZC

$(BUILD)/popup_zc.h: upstream/PopUp.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ POPUP_ZC

$(BUILD)/doclite_zc.h: upstream/DocLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ DOCLITE_ZC

$(BUILD)/doclib_zc.h: upstream/DocLib.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ DOCLIB_ZC

$(BUILD)/notes_zc.h: upstream/Notes.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ NOTES_ZC

$(BUILD)/globshare_zc.h: upstream/GlobShare.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ GLOBSHARE_ZC

$(BUILD)/life_zc.h: upstream/Life.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ LIFE_ZC

$(BUILD)/cartlite_zc.h: upstream/CartLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ CARTLITE_ZC

$(BUILD)/vec2lite_zc.h: upstream/Vec2Lite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ VEC2LITE_ZC

$(BUILD)/angleslite_zc.h: upstream/AnglesLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ ANGLESLITE_ZC

$(BUILD)/coslite_zc.h: upstream/CosLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ COSLITE_ZC

$(BUILD)/sqrtlite_zc.h: upstream/SqrtLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ SQRTLITE_ZC

$(BUILD)/arglite_zc.h: upstream/ArgLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ ARGLITE_ZC

$(BUILD)/commalite_zc.h: upstream/CommaLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ COMMALITE_ZC

$(BUILD)/plot3lite_zc.h: upstream/Plot3Lite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ PLOT3LITE_ZC

$(BUILD)/tospilite_zc.h: upstream/TosPiLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ TOSPILITE_ZC

$(BUILD)/colorlite_zc.h: upstream/ColorLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ COLORLITE_ZC

$(BUILD)/turtlelite_zc.h: upstream/TurtleLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ TURTLELITE_ZC
$(BUILD)/filllite_zc.h: upstream/FillLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ FILLLITE_ZC
$(BUILD)/initlite_zc.h: upstream/InitLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ INITLITE_ZC
$(BUILD)/deflite_zc.h: upstream/DefLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ DEFLITE_ZC
$(BUILD)/printlite_zc.h: upstream/PrintLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ PRINTLITE_ZC
$(BUILD)/msglite_zc.h: upstream/MsgLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ MSGLITE_ZC
$(BUILD)/menulite_zc.h: upstream/MenuLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ MENULITE_ZC
$(BUILD)/findlite_zc.h: upstream/FindLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ FINDLITE_ZC
$(BUILD)/fslite_zc.h: upstream/FsLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ FSLITE_ZC
$(BUILD)/setuplite_zc.h: upstream/SetupLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ SETUPLITE_ZC
$(BUILD)/ttlite_zc.h: upstream/TtLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ TTLITE_ZC
$(BUILD)/buflite_zc.h: upstream/BufLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ BUFLITE_ZC
$(BUILD)/inclite_zc.h: upstream/IncLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ INCLITE_ZC
$(BUILD)/dclite_zc.h: upstream/DcLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ DCLITE_ZC
$(BUILD)/linedclite_zc.h: upstream/LineDcLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ LINEDCLITE_ZC
$(BUILD)/grflite_zc.h: upstream/GrfLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ GRFLITE_ZC
$(BUILD)/movelite_zc.h: upstream/MoveLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ MOVELITE_ZC
$(BUILD)/checkedlite_zc.h: upstream/CheckedLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ CHECKEDLITE_ZC
$(BUILD)/cmplite_zc.h: upstream/CmpLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ CMPLITE_ZC
$(BUILD)/forinclite_zc.h: upstream/ForIncLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ FORINCLITE_ZC
$(BUILD)/microlite_zc.h: upstream/MicroLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ MICROLITE_ZC
$(BUILD)/movestacklite_zc.h: upstream/MoveStackLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ MOVESTACKLITE_ZC
$(BUILD)/endlite_zc.h: upstream/EndLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ ENDLITE_ZC
$(BUILD)/drawitlite_zc.h: upstream/DrawItLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ DRAWITLITE_ZC
$(BUILD)/latticelite_zc.h: upstream/LatticeLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ LATTICELITE_ZC
$(BUILD)/looplite_zc.h: upstream/LoopLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ LOOPLITE_ZC
$(BUILD)/demolite_zc.h: upstream/DemoLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ DEMOLITE_ZC
$(BUILD)/eventlite_zc.h: upstream/EventLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ EVENTLITE_ZC
$(BUILD)/playlite_zc.h: upstream/PlayLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ PLAYLITE_ZC
$(BUILD)/inputlite_zc.h: upstream/InputLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ INPUTLITE_ZC
$(BUILD)/rightlite_zc.h: upstream/RightLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RIGHTLITE_ZC
$(BUILD)/cursorlite_zc.h: upstream/CursorLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ CURSORLITE_ZC
$(BUILD)/uplite_zc.h: upstream/UpLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ UPLITE_ZC
$(BUILD)/ticklite_zc.h: upstream/TickLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ TICKLITE_ZC
$(BUILD)/framelite_zc.h: upstream/FrameLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ FRAMELITE_ZC
$(BUILD)/plotdclite_zc.h: upstream/PlotDcLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ PLOTDCLITE_ZC
$(BUILD)/abortlite_zc.h: upstream/AbortLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ ABORTLITE_ZC
$(BUILD)/aimmovelite_zc.h: upstream/AimMoveLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ AIMMOVELITE_ZC
$(BUILD)/idlelite_zc.h: upstream/IdleLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ IDLELITE_ZC
$(BUILD)/layerlite_zc.h: upstream/LayerLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ LAYERLITE_ZC
$(BUILD)/endslite_zc.h: upstream/EndsLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ ENDSLITE_ZC
$(BUILD)/speedlite_zc.h: upstream/SpeedLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ SPEEDLITE_ZC
$(BUILD)/midlite_zc.h: upstream/MidLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ MIDLITE_ZC
$(BUILD)/livelite_zc.h: upstream/LiveLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ LIVELITE_ZC
$(BUILD)/accellite_zc.h: upstream/AccelLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ ACCELLITE_ZC
$(BUILD)/restartlite_zc.h: upstream/RestartLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RESTARTLITE_ZC
$(BUILD)/widthlite_zc.h: upstream/WidthLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ WIDTHLITE_ZC
$(BUILD)/bothcolorlite_zc.h: upstream/BothColorLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ BOTHCOLORLITE_ZC
$(BUILD)/menufulllite_zc.h: upstream/MenuFullLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ MENUFULLLITE_ZC
$(BUILD)/menubiglite_zc.h: upstream/MenuBigLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ MENUBIGLITE_ZC
$(BUILD)/trylite_zc.h: upstream/TryLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ TRYLITE_ZC
$(BUILD)/stepcountlite_zc.h: upstream/StepCountLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ STEPCOUNTLITE_ZC
$(BUILD)/anglesfulllite_zc.h: upstream/AnglesFullLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ ANGLESFULLLITE_ZC
$(BUILD)/braceangleslite_zc.h: upstream/BraceAnglesLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ BRACEANGLESLITE_ZC
$(BUILD)/bracepilite_zc.h: upstream/BracePiLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ BRACEPILITE_ZC
$(BUILD)/setmenulite_zc.h: upstream/SetMenuLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ SETMENULITE_ZC
$(BUILD)/nearlatticelite_zc.h: upstream/NearLatticeLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ NEARLATTICELITE_ZC
$(BUILD)/f64iflite_zc.h: upstream/F64IfLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ F64IFLITE_ZC
$(BUILD)/wraplatticelite_zc.h: upstream/WrapLatticeLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ WRAPLATTICELITE_ZC
$(BUILD)/menulooplite_zc.h: upstream/MenuLoopLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ MENULOOPLITE_ZC
$(BUILD)/idxalllite_zc.h: upstream/IdxAllLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ IDXALLLITE_ZC
$(BUILD)/disklat_zc.h: upstream/DiskLat.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ DISKLAT_ZC
$(BUILD)/stocklat_zc.h: upstream/StockLat.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ STOCKLAT_ZC
$(BUILD)/depthbuflite_zc.h: upstream/DepthBufLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ DEPTHBUFLITE_ZC
$(BUILD)/depthrstlite_zc.h: upstream/DepthRstLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ DEPTHRSTLITE_ZC
$(BUILD)/depthplotlite_zc.h: upstream/DepthPlotLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ DEPTHPLOTLITE_ZC
$(BUILD)/depthlinelite_zc.h: upstream/DepthLineLite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ DEPTHLINELITE_ZC

$(BUILD)/ramblk_zc.h: upstream/RamBlk.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RAMBLK_ZC

$(BUILD)/namefile_zc.h: upstream/NameFile.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ NAMEFILE_ZC

$(BUILD)/dirlook_zc.h: upstream/DirLook.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ DIRLOOK_ZC

$(BUILD)/dirdel_zc.h: upstream/DirDel.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ DIRDEL_ZC

$(BUILD)/fopen_zc.h: upstream/FOpen.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ FOPEN_ZC

$(BUILD)/fwrite_zc.h: upstream/FWrite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ FWRITE_ZC

$(BUILD)/multiblk_zc.h: upstream/MultiBlk.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ MULTIBLK_ZC

$(BUILD)/redsea_zc.h: upstream/RedSea.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ REDSEA_ZC

$(BUILD)/rsroot_zc.h: upstream/RSRoot.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RSROOT_ZC

$(BUILD)/rsfile_zc.h: upstream/RSFile.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RSFILE_ZC

$(BUILD)/rsalloc_zc.h: upstream/RSAlloc.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RSALLOC_ZC

$(BUILD)/rsfree_zc.h: upstream/RSFree.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RSFREE_ZC

$(BUILD)/rsmulti_zc.h: upstream/RSMulti.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RSMULTI_ZC

$(BUILD)/rscfile_zc.h: upstream/RSCFile.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RSCFILE_ZC

$(BUILD)/rscwrite_zc.h: upstream/RSCWrite.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RSCWRITE_ZC

$(BUILD)/rscseek_zc.h: upstream/RSCSeek.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RSCSEEK_ZC

$(BUILD)/rsclib_combined.ZC: upstream/lib/RedSeaCFile.ZC upstream/RSCLib.ZC | $(BUILD)
	cat upstream/lib/RedSeaCFile.ZC upstream/RSCLib.ZC > $@

$(BUILD)/rsclib_zc.h: $(BUILD)/rsclib_combined.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RSCLIB_ZC

$(BUILD)/rspersist_combined.ZC: upstream/lib/RedSeaCFile.ZC upstream/RSCPersist.ZC | $(BUILD)
	cat upstream/lib/RedSeaCFile.ZC upstream/RSCPersist.ZC > $@

$(BUILD)/rspersist_zc.h: $(BUILD)/rspersist_combined.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RSCPERSIST_ZC

$(BUILD)/runzc_combined.ZC: upstream/lib/RedSeaCFile.ZC upstream/RunZC.ZC | $(BUILD)
	cat upstream/lib/RedSeaCFile.ZC upstream/RunZC.ZC > $@

$(BUILD)/runzc_zc.h: $(BUILD)/runzc_combined.ZC scripts/embed-zc.py | $(BUILD)
	python3 scripts/embed-zc.py $< $@ RUNZC_ZC

$(BUILD)/hc_f64math.o: src/hc_f64math.c | $(BUILD)
	$(CLANG) $(CFLAGS_F64MATH) -c src/hc_f64math.c -o $@

$(BUILD)/vectors.o: src/vectors.S | $(BUILD)
	$(CLANG) $(CFLAGS_KERNEL) -c src/vectors.S -o $@

$(BOOT): $(BOOT_OBJS) src/linker-booter.ld
	$(CLANG) $(CFLAGS) $(LDFLAGS_BOOT) $(BOOT_OBJS) -o $@

$(KERNEL): $(KERNEL_OBJS) src/linker-kernel.ld
	$(CLANG) $(CFLAGS_KERNEL) $(LDFLAGS_KERNEL) $(KERNEL_OBJS) -o $@

# ---- Pi 4B diagnostic boot (distinct from QEMU virt) ----
$(BUILD)/zealbooter-pi.o: src/zealbooter.c src/handoff.h src/elf64.h src/plat_pi4.h src/mmio_map.h src/linker-booter.ld | $(BUILD)
	$(CLANG) $(CFLAGS) $(CFLAGS_PI) -c src/zealbooter.c -o $@

$(BUILD)/kernel_stub-pi.o: src/kernel_stub.c src/handoff.h src/hc_ir.h src/hc_front.h src/a64_emit.h src/fb_font.h src/virtio_kbd.h src/virtio_tablet.h src/virtio_blk.h src/disk_layout.h src/mmio_map.h $(BUILD)/netofdots_zc.h $(BUILD)/lines_zc.h $(BUILD)/minigr_zc.h $(BUILD)/memsort_zc.h $(BUILD)/peekplot_zc.h $(BUILD)/offbmp_zc.h $(BUILD)/heapstr_zc.h $(BUILD)/catfmt_zc.h $(BUILD)/heapque_zc.h $(BUILD)/jobque_zc.h $(BUILD)/jobrun_zc.h $(BUILD)/taskspawn_zc.h $(BUILD)/popup_zc.h $(BUILD)/doclite_zc.h $(BUILD)/doclib_zc.h $(BUILD)/notes_zc.h $(BUILD)/globshare_zc.h $(BUILD)/life_zc.h $(BUILD)/cartlite_zc.h $(BUILD)/vec2lite_zc.h $(BUILD)/angleslite_zc.h $(BUILD)/coslite_zc.h $(BUILD)/sqrtlite_zc.h $(BUILD)/arglite_zc.h $(BUILD)/commalite_zc.h $(BUILD)/plot3lite_zc.h $(BUILD)/tospilite_zc.h $(BUILD)/colorlite_zc.h $(BUILD)/turtlelite_zc.h $(BUILD)/filllite_zc.h $(BUILD)/initlite_zc.h $(BUILD)/deflite_zc.h $(BUILD)/printlite_zc.h $(BUILD)/msglite_zc.h $(BUILD)/menulite_zc.h $(BUILD)/findlite_zc.h $(BUILD)/fslite_zc.h $(BUILD)/setuplite_zc.h $(BUILD)/ttlite_zc.h $(BUILD)/buflite_zc.h $(BUILD)/inclite_zc.h $(BUILD)/dclite_zc.h $(BUILD)/linedclite_zc.h $(BUILD)/grflite_zc.h $(BUILD)/movelite_zc.h $(BUILD)/checkedlite_zc.h $(BUILD)/cmplite_zc.h $(BUILD)/forinclite_zc.h $(BUILD)/microlite_zc.h $(BUILD)/movestacklite_zc.h $(BUILD)/endlite_zc.h $(BUILD)/drawitlite_zc.h $(BUILD)/latticelite_zc.h $(BUILD)/looplite_zc.h $(BUILD)/demolite_zc.h $(BUILD)/eventlite_zc.h $(BUILD)/playlite_zc.h $(BUILD)/inputlite_zc.h $(BUILD)/rightlite_zc.h $(BUILD)/cursorlite_zc.h $(BUILD)/uplite_zc.h $(BUILD)/ticklite_zc.h $(BUILD)/framelite_zc.h $(BUILD)/plotdclite_zc.h $(BUILD)/abortlite_zc.h $(BUILD)/aimmovelite_zc.h $(BUILD)/idlelite_zc.h $(BUILD)/layerlite_zc.h $(BUILD)/endslite_zc.h $(BUILD)/speedlite_zc.h $(BUILD)/midlite_zc.h $(BUILD)/livelite_zc.h $(BUILD)/accellite_zc.h $(BUILD)/restartlite_zc.h $(BUILD)/widthlite_zc.h $(BUILD)/bothcolorlite_zc.h $(BUILD)/menufulllite_zc.h $(BUILD)/menubiglite_zc.h $(BUILD)/trylite_zc.h $(BUILD)/stepcountlite_zc.h $(BUILD)/anglesfulllite_zc.h $(BUILD)/braceangleslite_zc.h $(BUILD)/bracepilite_zc.h $(BUILD)/setmenulite_zc.h $(BUILD)/nearlatticelite_zc.h $(BUILD)/f64iflite_zc.h $(BUILD)/wraplatticelite_zc.h $(BUILD)/menulooplite_zc.h $(BUILD)/idxalllite_zc.h $(BUILD)/disklat_zc.h $(BUILD)/stocklat_zc.h $(BUILD)/depthbuflite_zc.h $(BUILD)/depthrstlite_zc.h $(BUILD)/depthplotlite_zc.h $(BUILD)/depthlinelite_zc.h $(BUILD)/ramblk_zc.h $(BUILD)/namefile_zc.h $(BUILD)/dirlook_zc.h $(BUILD)/dirdel_zc.h $(BUILD)/fopen_zc.h $(BUILD)/fwrite_zc.h $(BUILD)/multiblk_zc.h $(BUILD)/redsea_zc.h $(BUILD)/rsroot_zc.h $(BUILD)/rsfile_zc.h $(BUILD)/rsalloc_zc.h $(BUILD)/rsfree_zc.h $(BUILD)/rsmulti_zc.h $(BUILD)/rscfile_zc.h $(BUILD)/rscwrite_zc.h $(BUILD)/rscseek_zc.h $(BUILD)/rsclib_zc.h $(BUILD)/rspersist_zc.h $(BUILD)/runzc_zc.h | $(BUILD)
	$(CLANG) $(CFLAGS_KERNEL) $(CFLAGS_PI) -I$(BUILD) -c src/kernel_stub.c -o $@

$(BOOT_PI): $(BUILD)/zealbooter-pi.o src/linker-booter.ld
	$(CLANG) $(CFLAGS) $(CFLAGS_PI) $(LDFLAGS_BOOT) $(BUILD)/zealbooter-pi.o -o $@

$(KERNEL_PI): $(BUILD)/kernel_stub-pi.o $(BUILD)/vectors.o src/linker-kernel.ld
	$(CLANG) $(CFLAGS_KERNEL) $(CFLAGS_PI) $(LDFLAGS_KERNEL) $(BUILD)/kernel_stub-pi.o $(BUILD)/hc_f64math.o $(BUILD)/vectors.o -o $@

pi-diag: $(BOOT_PI) $(KERNEL_PI)

# UEFI CD image (Limine-recommended aarch64 path)
iso: $(BOOT) $(KERNEL) $(DEMO_BC)
	rm -rf $(BUILD)/iso_root
	mkdir -p $(BUILD)/iso_root/boot/limine $(BUILD)/iso_root/EFI/BOOT
	cp -f $(BOOT) $(BUILD)/iso_root/boot/zealbooter.elf
	cp -f $(KERNEL) $(BUILD)/iso_root/boot/kernel.elf
	cp -f $(DEMO_BC) $(BUILD)/iso_root/boot/demo.hcbc
	cp -f $(ESP_DIR)/limine.conf $(BUILD)/iso_root/boot/limine/limine.conf
	cp -f $(LIMINE_BIN)/limine-uefi-cd.bin $(BUILD)/iso_root/boot/limine/
	cp -f $(LIMINE_BIN)/BOOTAA64.EFI $(BUILD)/iso_root/EFI/BOOT/
	xorriso -as mkisofs -R -r -J \
		-hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(BUILD)/iso_root -o $(ISO)
	@echo "ISO: $(ISO)"

$(ESP_FAT): $(BOOT) $(KERNEL) $(DEMO_BC) $(ESP_DIR)/limine.conf | $(BUILD)
	mkdir -p $(ESP_DIR)/EFI/BOOT $(ESP_DIR)/boot
	cp -f $(LIMINE_BIN)/BOOTAA64.EFI $(ESP_DIR)/EFI/BOOT/BOOTAA64.EFI
	cp -f $(BOOT) $(ESP_DIR)/boot/zealbooter.elf
	cp -f $(KERNEL) $(ESP_DIR)/boot/kernel.elf
	rm -f $(ESP_FAT)
	truncate -s $$(($(ESP_SECTORS)*512)) $(ESP_FAT)
	mformat -i $(ESP_FAT) -F -v ZEALAA64 ::
	mmd -i $(ESP_FAT) ::/EFI ::/EFI/BOOT ::/boot
	mcopy -i $(ESP_FAT) $(ESP_DIR)/EFI/BOOT/BOOTAA64.EFI ::/EFI/BOOT/
	mcopy -i $(ESP_FAT) $(ESP_DIR)/boot/zealbooter.elf ::/boot/
	mcopy -i $(ESP_FAT) $(ESP_DIR)/boot/kernel.elf ::/boot/
	mcopy -i $(ESP_FAT) $(DEMO_BC) ::/boot/
	mcopy -i $(ESP_FAT) $(ESP_DIR)/limine.conf ::/limine.conf

esp: $(ESP_FAT)
	rm -f $(DISK_IMG)
	truncate -s 64M $(DISK_IMG)
	$(SGDISK) -o $(DISK_IMG)
	$(SGDISK) -n 1:$(ESP_START_SECTOR):$(ESP_END_SECTOR) -t 1:ef00 -c 1:'EFI System' $(DISK_IMG)
	dd if=$(ESP_FAT) of=$(DISK_IMG) bs=512 seek=$(ESP_START_SECTOR) conv=notrunc status=none
	@echo "GPT disk: $(DISK_IMG) (ESP $(ESP_START_SECTOR)-$(ESP_END_SECTOR), RedSea LBA $(RS_LBA_BASE)+$(RS_SECTS))"

$(FW_VARS): | $(BUILD)
	cp -f $(FW_VARS_IN) $(FW_VARS)

QEMU_FW = \
	-drive if=pflash,unit=0,format=raw,file=$(FW_CODE),readonly=on \
	-drive if=pflash,unit=1,format=raw,file=$(FW_VARS)

# Match Makefile run-iso / run-serial (canonical virt HW).
QEMU_VIRT = -machine virt,accel=hvf,gic-version=3 -cpu host -m 512 \
	-device ramfb \
	-device virtio-keyboard-device \
	-device virtio-tablet-device

# Persistent RedSea image (64KiB = 128 sectors). Created once; guest writes survive.
$(RS_IMG): | $(BUILD)
	truncate -s 64K $@

QEMU_RS = -drive if=none,file=$(RS_IMG),format=raw,id=rs0 \
	-device virtio-blk-device,drive=rs0

run-iso: iso $(FW_VARS) $(RS_IMG)
	$(QEMU) $(QEMU_VIRT) \
		$(QEMU_FW) \
		$(QEMU_RS) \
		-drive if=none,file=$(ISO),id=cd0,media=cdrom,readonly=on \
		-device virtio-scsi-pci,id=scsi0 \
		-device scsi-cd,drive=cd0,bootindex=0 \
		-serial stdio -display cocoa \
		-name zealos-aarch64-hello

run-serial: iso $(FW_VARS) $(RS_IMG)
	$(QEMU) $(QEMU_VIRT) \
		$(QEMU_FW) \
		$(QEMU_RS) \
		-drive if=none,file=$(ISO),id=cd0,media=cdrom,readonly=on \
		-device virtio-scsi-pci,id=scsi0 \
		-device scsi-cd,drive=cd0,bootindex=0 \
		-serial stdio -display none \
		-name zealos-aarch64-hello

# Boot until "hc IR OK"; fail make if jit_smoke reported FAIL (no shell hang).
check-serial: iso $(FW_VARS) $(RS_IMG)
	@rm -f $(BUILD)/check-serial.log
	@( $(QEMU) $(QEMU_VIRT) \
		$(QEMU_FW) \
		$(QEMU_RS) \
		-drive if=none,file=$(ISO),id=cd0,media=cdrom,readonly=on \
		-device virtio-scsi-pci,id=scsi0 \
		-device scsi-cd,drive=cd0,bootindex=0 \
		-serial stdio -display none \
		-name zealos-aarch64-check \
		> $(BUILD)/check-serial.log 2>&1 & echo $$! > $(BUILD)/check-serial.pid )
	@i=0; \
	while [ $$i -lt 90 ]; do \
	  if grep -q 'hc IR OK (front+host)' $(BUILD)/check-serial.log 2>/dev/null; then \
	    kill `cat $(BUILD)/check-serial.pid` 2>/dev/null || true; \
	    wait `cat $(BUILD)/check-serial.pid` 2>/dev/null || true; \
	    if grep -E 'hc IR FAIL|Upstream .* FAIL|Front .* FAIL' $(BUILD)/check-serial.log; then \
	      echo 'check-serial: FAIL lines present'; exit 1; \
	    fi; \
	    echo 'check-serial: hc IR OK'; \
	    exit 0; \
	  fi; \
	  if grep -q 'hc IR FAIL' $(BUILD)/check-serial.log 2>/dev/null; then \
	    kill `cat $(BUILD)/check-serial.pid` 2>/dev/null || true; \
	    wait `cat $(BUILD)/check-serial.pid` 2>/dev/null || true; \
	    echo 'check-serial: hc IR FAIL'; \
	    grep -E 'FAIL|MsgMask|LoopLite|InputLite' $(BUILD)/check-serial.log | tail -20; \
	    exit 1; \
	  fi; \
	  sleep 1; \
	  i=$$((i+1)); \
	done; \
	kill `cat $(BUILD)/check-serial.pid` 2>/dev/null || true; \
	echo 'check-serial: timeout'; tail -40 $(BUILD)/check-serial.log; exit 1

# UTM-shaped path: boot GPT disk via virtio-blk-pci only (no MMIO RedSea).
# Accepts when serial shows: virtio-blk: OK … pci / virtio-blk: rw OK
run-pci: esp $(FW_VARS)
	$(QEMU) $(QEMU_VIRT) \
		$(QEMU_FW) \
		-drive if=none,format=raw,file=$(DISK_IMG),id=esp \
		-device virtio-blk-pci,drive=esp,bootindex=0 \
		-serial stdio -display none \
		-name zealos-aarch64-pci

run: run-serial

utm: esp $(RS_IMG)
	./scripts/install-utm.sh

pi-sd: pi-diag esp
	./scripts/pi-sd-stage.sh

clean:
	rm -rf $(BUILD)
	rm -f $(ESP_DIR)/boot/zealbooter.elf $(ESP_DIR)/boot/kernel.elf \
		$(ESP_DIR)/boot/hello.elf $(ESP_DIR)/EFI/BOOT/BOOTAA64.EFI
