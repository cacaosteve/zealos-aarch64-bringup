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
printf 'PI-DESKTOP-2 prepared: %s\n' "$OUT"
cat <<'EOF'
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
entry retains the USB diagnostic shell. Connect a supported USB keyboard
and pointer for the native desktop. The bundled source archive supplies
the apps, and the USB storage test stick is optional.

Expect PI-DESKTOP-2 progress text, then the native terminals and centered
animated Z splash. In a guest terminal try 40+2; then Dir, usbstatus, and
psdread. psdread initializes the SD controller and reads the card's partition
table; it does not write sectors. The current full-card FAT-only layout is
expected to report no ZealOS data partition. On a card flashed from the new
Pi image, `psdopen` binds the unique 0xDA data partition and enables bounded
disk writes only within it. The image builder initializes the two append-only
source banks and a full-size RedSea volume containing `/Home` and `/Tmp`.
After `psdopen`, ordinary `FileWrite` create/replace and `DirMake` are enabled
within existing directory capacity; deletion, rename, and automatic directory
growth are not. `.ZC` saves use the separate source banks. `rspersist` is
limited to the 128-sector bring-up format and must not be used to initialize
this image.
The desktop uses a packed 800x600 surface centered and integer-scaled into
the firmware display mode. Borders are expected on larger screens. The
physical width/height/pitch/scale and logical surface are reported at startup.
Confirm keyboard input, pointer movement and menu clicks. Capture the desktop
and command output. If startup stops, capture the last PI-DESKTOP-2/zc/USB
message; the second Limine entry is the diagnostic recovery path.

USB storage remains read-only. The native source writer stays unbound until
`psdopen` is run. Bundled sources come from the boot module, not VirtIO.
This staging script prepares files only; it does not flash a card. Physical
SD read/write, flush, cold-boot persistence, and app acceptance remain open.
EOF
