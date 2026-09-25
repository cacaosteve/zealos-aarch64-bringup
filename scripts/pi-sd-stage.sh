#!/usr/bin/env bash
# Stage a Pi 4 UEFI ESP payload (Limine + Pi-diag ZealBooter + kernel).
# Does NOT write an SD card — copy onto a card that already has pftf RPi4 UEFI.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/build/pi-esp"
ESP="${ROOT}/esp"
BOOT_PI="${ROOT}/build/zealbooter-pi.elf"
KERNEL_PI="${ROOT}/build/kernel-pi.elf"

cd "$ROOT"
make pi-diag
# Limine EFI + demo.hcbc come from the virt ESP tree (unrelated to UART/plat).
make esp

if [[ ! -f "$BOOT_PI" || ! -f "$KERNEL_PI" ]]; then
  echo "ERROR: missing Pi diag ELFs ($BOOT_PI / $KERNEL_PI)" >&2
  exit 1
fi

rm -rf "$OUT"
mkdir -p "$OUT/EFI/BOOT" "$OUT/boot"
cp -f "$ESP/EFI/BOOT/BOOTAA64.EFI" "$OUT/EFI/BOOT/"
# Explicit Pi builds — never ship QEMU-UART zealbooter/kernel to the SD.
cp -f "$BOOT_PI" "$OUT/boot/zealbooter.elf"
cp -f "$KERNEL_PI" "$OUT/boot/kernel.elf"
cp -f "$ESP/boot/demo.hcbc" "$OUT/boot/" 2>/dev/null || true
cp -f "$ESP/limine-pi.conf" "$OUT/limine.conf"

cat <<EOF
Pi ESP staged (PI-DIAG-4): $OUT

FB markers (keep firmware splash; do not expect a cleared screen):
  yellow   ~y40   ZealBooter entry
  green/red ~y60  UART MMIO map OK / FAIL (TX deferred — UART left NULL)
  orange   ~y200  past map, continuing FB-only
  purple   ~y220  enter handoff/module path
  teal     ~y240  module present (red = missing)
  olive    ~y260  loading ELF (red = load fail)
  cyan     ~y80   jumping to kernel
  magenta  ~y120  kernel_entry
  green/red ~y144 kernel UART remap OK / FAIL
  white    ~y280  PI-DIAG-2: UART TX OK
  green×3  ~y314+ PI-DIAG-3: soft CNTP ticks
  yellow   ~y350  PI-DIAG-3: soft CNTP done
  cyan×3   ~y384+ PI-DIAG-4: GICv2 IRQ ticks
  white    ~y420  PI-DIAG-4: IRQ OK then serial shell prompt

Next: boot; after white IRQ bar expect FB/UART shell prompt (help|halt|...).
  USB-TTL @ 115200 on GPIO 14/15 if you want serial input.

QEMU virt path is unchanged (make / make run-serial) — distinct ELFs.

See PI4.md for the track and acceptance criteria.
EOF
