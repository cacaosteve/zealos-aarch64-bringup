#!/usr/bin/env bash
# Prepare a Pi native-runtime payload and diagnostic recovery entry; no card IO.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/build/pi-desktop-esp"
cd "$ROOT"
make pi-desktop pi-diag esp
mkdir -p "$OUT/boot" "$OUT/EFI/BOOT"
cp "$ROOT/build/kernel-pi-desktop.elf" "$OUT/boot/kernel.elf"
cp "$ROOT/build/kernel-pi.elf" "$OUT/boot/kernel-diag.elf"
cp "$ROOT/build/zealbooter-pi.elf" "$OUT/boot/zealbooter.elf"
cp "$ROOT/build/sources.tar" "$OUT/boot/sources.tar"
cp "$ROOT/esp/boot/demo.hcbc" "$OUT/boot/demo.hcbc"
cp "$ROOT/esp/EFI/BOOT/BOOTAA64.EFI" "$OUT/EFI/BOOT/BOOTAA64.EFI"
cp "$ROOT/esp/limine-pi-desktop.conf" "$OUT/limine.conf"
python3 "$ROOT/scripts/check-pi-desktop-image.py" "$OUT"
cat <<EOF
PI-DESKTOP-2 prepared: $OUT
This script prepares files only. It never writes or erases a card.
Keep existing Pi UEFI/firmware/DTBs. The seven payload files are:
  boot/kernel.elf           native ZealC/compiler/graphics/runtime + Pi hardware
  boot/kernel-diag.elf      accepted MULTI-USB2-6 recovery kernel
  boot/zealbooter.elf
  boot/demo.hcbc
  boot/sources.tar          bundled ARM64-adapted ZealOS source modules
  EFI/BOOT/BOOTAA64.EFI
  limine.conf

The first Limine entry starts the desktop after five seconds. The second
entry retains the USB diagnostic shell. Use the same three devices and ports,
SanDisk in a black USB2 port. The accepted automatic USB probe must pass
before the native compiler/window manager/graphical terminals start.

Expect PI-DESKTOP-2 progress text, then the native terminals and centered
animated Z splash. In a guest terminal try 40+2; then Dir; and usbstatus.
The desktop uses a packed 800x600 surface centered and integer-scaled into
the firmware display mode. Borders are expected on larger screens. The
physical width/height/pitch/scale and logical surface are reported at startup.
Confirm keyboard input, pointer movement and menu clicks. Capture the desktop
and command output. If startup stops, capture the last PI-DESKTOP-2/zc/USB
message; the second Limine entry is the diagnostic recovery path.

USB storage remains read-only and the native source writer is disabled on
this Pi candidate. Bundled sources come from the boot module, not VirtIO.
This first desktop check does not claim writable persistence or Pi installation.
EOF
