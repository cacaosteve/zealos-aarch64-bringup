# Freestanding compiler integration. Imported compiler and local host share ABI
# flags; the rest of the kernel continues to avoid generated SIMD/FP code.
ZC_VENDOR := $(ROOT)/third_party/aiwnios/c
ZC_NAMES := lex parser optpass hash cque ffi_gen arm64_asm arm_backend
ZC_VENDOR_OBJS := $(addprefix $(BUILD)/zc/,$(addsuffix .o,$(ZC_NAMES)))
ZC_OBJS := $(ZC_VENDOR_OBJS) $(BUILD)/zc/runtime.o $(BUILD)/zc/libc.o $(BUILD)/zc/context.o $(BUILD)/zc/ffi.o
ZC_FLAGS := -target aarch64-unknown-none-elf -std=gnu11 -ffreestanding \
    -fno-builtin -fPIE -fno-stack-protector -fno-stack-check \
    -ffunction-sections -fdata-sections -march=armv8-a -mcmodel=small \
    -mno-outline-atomics -mbranch-protection=none -ffixed-x28 -O2 \
    -I$(ROOT)/src/zc/libc -I$(ROOT)/src/zc -I$(ZC_VENDOR) \
    -DZEAL_FREESTANDING -DZEAL_GUEST_EXCEPTIONS -include $(ROOT)/src/zc/port.h
ZC_HEADERS := $(wildcard src/zc/*.h src/zc/libc/*.h third_party/aiwnios/c/*.h)

$(BUILD)/zc:
	mkdir -p $@
$(BUILD)/zc/%.o: third_party/aiwnios/c/%.c $(ZC_HEADERS) src/zc/build.mk | $(BUILD)/zc
	$(CLANG) $(ZC_FLAGS) -w -Wno-incompatible-pointer-types -Wno-int-conversion -c $< -o $@
$(BUILD)/zc/runtime.o: src/zc/runtime.c $(ZC_HEADERS) src/zc/build.mk | $(BUILD)/zc
	$(CLANG) $(ZC_FLAGS) -Wall -Wextra -c $< -o $@
$(BUILD)/zc/libc.o: src/zc/libc.c $(ZC_HEADERS) src/zc/build.mk | $(BUILD)/zc
	$(CLANG) $(ZC_FLAGS) -Wall -Wextra -c $< -o $@
$(BUILD)/zc/context.o: src/zc/context.S | $(BUILD)/zc
	$(CLANG) $(ZC_FLAGS) -c $< -o $@
$(BUILD)/zc/ffi.o: third_party/aiwnios/asm/ffi_call_tos_aarch64.s | $(BUILD)/zc
	$(CLANG) -target aarch64-unknown-none-elf -c $< -o $@
.PHONY: zc-objects
zc-objects: $(ZC_VENDOR_OBJS)

ZC_SOURCES := $(shell find upstream/pinned upstream/bootstrap tests/compat -type f)
$(BUILD)/sources.tar: $(ZC_SOURCES) third_party/aiwnios/LICENSE scripts/build-source-bundle.py scripts/arm64_source.py scripts/check-arm64-source.py upstream/arm64/GrScreen.md | $(BUILD)
	python3 scripts/build-source-bundle.py $@

.PHONY: check-upstream check-compat check-compat-pci check-grscreen
check-upstream:
	python3 scripts/check-upstream.py
check-compat: check-upstream iso
	QEMU="$(QEMU)" QEMU_VIRT="$(QEMU_VIRT)" FW_CODE="$(FW_CODE)" FW_VARS_IN="$(FW_VARS_IN)" python3 scripts/check-compat.py
check-compat-pci: check-upstream esp
	python3 scripts/check-arm64-source.py
	QEMU="$(QEMU)" QEMU_VIRT="$(QEMU_VIRT)" FW_CODE="$(FW_CODE)" FW_VARS_IN="$(FW_VARS_IN)" python3 scripts/check-compat.py --pci
check-grscreen: check-upstream esp
	python3 scripts/check-arm64-source.py
	QEMU="$(QEMU)" QEMU_VIRT="$(QEMU_VIRT)" FW_CODE="$(FW_CODE)" FW_VARS_IN="$(FW_VARS_IN)" python3 scripts/check-compat.py --pci --probe-module /System/Win.ZC --probe-winmgr --probe-grscreen
