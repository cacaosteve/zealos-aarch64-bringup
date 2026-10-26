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

# Sidecar for the USB-stick read test; this script only prepares build files.
python3 - "$ROOT/build/ZEALTEST.TXT" <<'PY'
from pathlib import Path
import sys, zlib
pattern = b"ZealOS Pi4B FAT32 read test.\n"
data = (pattern * ((1025 + len(pattern) - 1) // len(pattern)))[:1025]
assert zlib.crc32(data) == 0x873b79ac
Path(sys.argv[1]).write_bytes(data)
PY

cat <<EOF
Pi ESP staged — MULTI-USB2-6: $OUT

This prepares files only; it does not write or erase a card.
Copy these five files to a Pi 4B microSD that already contains pftf RPi4 UEFI:
  boot/kernel.elf
  boot/zealbooter.elf
  boot/demo.hcbc
  limine.conf
  EFI/BOOT/BOOTAA64.EFI
Keep the card's RPI_EFI.fd, bcm2711-rpi-4-b.dtb and firmware files.

For the combined test, connect the existing SanDisk fixture stick to a BLACK
USB 2 port on the Pi. Connect the keyboard and Apple trackpad to two other
ports; those USB2 devices can use blue ports. Do not add another hub or disk.
Cold power-cycle with all three connected and leave them connected.

This candidate reserves six xHCI slots and independent child output contexts,
EP0/interrupt/bulk rings and buffers inside the existing DMA arena. Two or
more connected children select the USB2 multi-device path. One child retains
the earlier keyboard-only, trackpad-only or storage-only route. The full
configuration and boot protocol/readback checks still precede endpoint use.
USB2 BOT uses zero burst sizes; absent USB3 companions stay marked absent in
the parsed descriptor summary and are translated to USB2 endpoint parameters.

Storage remains read-only. All three devices are enumerated/configured first,
then keyboard/pointer queues and their event dispatcher start. The first
healthy input poll automatically runs INQUIRY, capacity, sector/FAT32 and
known-file tests with HID dispatch enabled. No usbread command is needed.
The pending flag is consumed once; failure stops safely without replaying
bootstrap traffic. The earlier manual version passed these media checks
and ten periodic file checks with input online on the physical Pi.

Expected boot summary:
  USB bus MULTI-USB2-6 active/stage/fault=1/15/0 (15 prints as hex 0x0f)
  kbd/mouse/disk=1/1/1 (disk means configured, not read-verified)
  USB storage pending/ready/probe-state/INQUIRY-step=0/1/66/7 after auto-probe
  USB storage automatic/ready-seconds/checks/target=1/<elapsed>/<checks>/0b
  CRC1/CRC2=873b79ac/873b79ac
The initial pre-shell boot report may show pending=1/ready=0; automatic
probing starts in the input loop after all bootstrap ownership is retained.
Child role 1 is keyboard, 2 is pointer, 3 is storage. Stage 13 means that its
endpoints are configured. Type help, move the pointer, hold its primary
button and drag to paint, then type usbstatus. Capture the automatic probe
result and child reports/keys/moves counters.

Move the trackpad during startup. Success sets pending=0/ready=1/checks=1;
file checks then run every five seconds. Keep input active until the message
"ten live file checks passed with input online", then run usbstatus. Expect
checks >= 11 (0x0b), both CRCs 873b79ac, increasing input counters, ideally
interleaved > 0, active=1/fault=0. The ready-seconds field shows elapsed time
in hexadecimal. usbread after success reports the verified state without
reissuing INQUIRY on the live queues; usbstatus remains the normal report.

The preceding board run accepted the exact READ CAPACITY data-IN recovery,
capacity/file reads and sustained input. Recovery remains limited to
an exact matching High-Speed bulk data-IN STALL: Reset Endpoint (TSP=0),
CLEAR_FEATURE(ENDPOINT_HALT), then Set TR Dequeue Pointer past the failed TD.
Each command/control completion and endpoint state is checked before the
next action. The next BOT phase receives the CSW, without resending CBW/data.
A failed SCSI command can then use the existing bounded REQUEST SENSE/retry.
At most four such recoveries are allowed per session. HID events continue
through the shared dispatcher. This adds no media writes or full BOT reset.

If the automatic probe fails, the controller stops safely and input stops too. Capture
USB storage probe-state/INQUIRY-step, USB INQUIRY phase/rc/status/checks/actual,
CSW fields, USB BOT stall/reset/clear-halt/set-dequeue, recovered sense, and
USB read last op/phase/status/control/pointer lines. Those snapshots survive
stopping. The failed probe is never marked ready or
rerun on stale queues; cold boot starts a new automatic test session.
Timeouts, bad event identities, CBW/CSW stalls, phase errors and Full-Speed
targets needing transaction-translator cleanup remain fatal to this session.

The prior bounded readback fallback remains: only a completed Configure
Endpoint command with rejected bulk output can disable an idle storage
slot before clearing DCBAA, leaving keyboard/pointer active with storage
explicitly disabled. Failed slot disable, unknown/transport errors and
startup failures still use common halt/BME-off/alias-restoration cleanup.

This first combined gate does not add hotplug/re-enumeration, writable media,
Apple multitouch/secondary-click gestures or the full Pi graphical desktop.
The SanDisk fixture remains unchanged; its reproducible sidecar remains
$ROOT/build/ZEALTEST.TXT. Do not put the SuperSpeed stick in a blue port for
this test: the accepted legacy direct-SS storage route would be selected.

The boot chain remains Pi UEFI -> Limine -> ZealBooter -> kernel. Early
colored HDMI markers and FDT/PCIe/DMA diagnostics remain available. The Pi
ELFs use explicit Pi UART/platform settings; QEMU/UTM use separate ELFs.
See PI4.md for physical acceptance evidence, DMA translation and recovery.
EOF
