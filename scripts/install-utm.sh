#!/usr/bin/env bash
# Refresh the single UTM VM "ZealosAarch64Hello" with our Limine ESP.
#
# Clone quirks this script handles:
#   - Resolve by utmctl UUID (folder name may still be the clone's).
#   - Keep the clone's Drive Identifier / ImageName so UTM Start works.
#   - Overwrite that image file with build/disk.img (Zeal ESP contents).
#   - QEMU.AdditionalArguments must be an argv *array*, not a string.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DISK_IMG="$ROOT/build/disk.img"
RS_IMG="$ROOT/build/redsea.img"
UTMCTL="/Applications/UTM.app/Contents/MacOS/utmctl"
DOCS="$HOME/Library/Containers/com.utmapp.UTM/Data/Documents"
VM_NAME="ZealosAarch64Hello"
CLONE_FROM="${UTM_CLONE_FROM:-Omarchy QEMU}"

if [[ ! -f "$DISK_IMG" ]]; then
  echo "missing $DISK_IMG — run: make esp" >&2
  exit 1
fi
# RS_IMG still used by QEMU harness (MMIO). UTM uses PCI boot-disk tail instead.
if [[ ! -f "$RS_IMG" ]]; then
  truncate -s 64K "$RS_IMG" || true
fi
if [[ ! -x "$UTMCTL" ]]; then
  echo "UTM not found" >&2
  exit 1
fi

"$UTMCTL" stop "$VM_NAME" 2>/dev/null || true
sleep 1

if ! "$UTMCTL" list 2>/dev/null | grep -q "$VM_NAME"; then
  echo "utmctl: cloning '$CLONE_FROM' → $VM_NAME"
  "$UTMCTL" clone "$CLONE_FROM" --name "$VM_NAME"
fi

UUID="$("$UTMCTL" list | awk -v n="$VM_NAME" 'index($0,n){print $1; exit}')"
if [[ -z "${UUID:-}" ]]; then
  echo "VM '$VM_NAME' not listed" >&2
  exit 1
fi

DEST="$(python3 - <<PY
import plistlib, pathlib
want = "$UUID".upper()
docs = pathlib.Path("$DOCS")
for d in docs.glob("*.utm"):
    c = plistlib.loads((d / "config.plist").read_bytes())
    if c.get("Information", {}).get("UUID", "").upper() == want:
        print(d)
        break
PY
)"
if [[ -z "$DEST" || ! -d "$DEST" ]]; then
  echo "could not resolve .utm for $UUID" >&2
  exit 1
fi
echo "Resolved $VM_NAME → $DEST"

python3 - <<PY
import plistlib, pathlib, shutil, uuid as uuidlib

dest = pathlib.Path("$DEST")
disk_src = pathlib.Path("$DISK_IMG")
rs_src = pathlib.Path("$RS_IMG")
cfg_path = dest / "config.plist"
cfg = plistlib.loads(cfg_path.read_bytes())

cfg.setdefault("Information", {})["Name"] = "$VM_NAME"
cfg["Information"]["Notes"] = (
    "ZealOS aarch64 Limine bring-up. Disk image bytes are our ESP; "
    "filename may still be Omarchy-QEMU.raw from the UTM clone. "
    "RedSea lives at LBA 130911 (128 sectors) before the GPT backup; "
    "make utm preserves that range. QEMU harness also has MMIO redsea.img."
)

# Preserve existing disk slot identity (UTM caches drive ids from the clone).
drives = cfg.get("Drive") or []
disk = None
for d in drives:
    if d.get("ImageType") == "Disk":
        disk = d
        break
if disk is None:
    disk = {
        "Identifier": str(uuidlib.uuid4()).upper(),
        "ImageType": "Disk",
        "Interface": "VirtIO",
        "InterfaceVersion": 1,
    }
    drives = [disk]
    cfg["Drive"] = drives

img_name = disk.get("ImageName") or "Omarchy-QEMU.raw"
disk["ImageName"] = img_name
disk["ImageType"] = "Disk"
disk["Interface"] = disk.get("Interface") or "VirtIO"
disk["InterfaceVersion"] = disk.get("InterfaceVersion") or 1
disk["ReadOnly"] = False
# Drop any extra drives (CD etc.) so only our ESP can boot.
cfg["Drive"] = [disk]

data = dest / "Data"
data.mkdir(exist_ok=True)
target = data / img_name

# Preserve RedSea window (LBA 130911, 128 sectors) across ESP refresh.
# Must match src/disk_layout.h / Makefile RS_LBA_BASE + RS_SECTS.
RS_LBA_BASE = 130911
RS_SECTS = 128
RS_BYTES = RS_SECTS * 512
old_rs = None
if target.is_file() and target.stat().st_size >= (RS_LBA_BASE + RS_SECTS) * 512:
    with open(target, "rb") as f:
        f.seek(RS_LBA_BASE * 512)
        old_rs = f.read(RS_BYTES)

shutil.copyfile(disk_src, target)
if old_rs and len(old_rs) == RS_BYTES:
    with open(target, "r+b") as f:
        f.seek(RS_LBA_BASE * 512)
        f.write(old_rs)
    print(f"RedSea preserved @ LBA {RS_LBA_BASE} ({RS_BYTES} bytes)")
else:
    print(f"RedSea @ LBA {RS_LBA_BASE}: fresh (zeros)")

# Convenience second name for humans / older scripts
shutil.copyfile(disk_src, data / "zealos-esp.raw")

sys = cfg.setdefault("System", {})
sys["Architecture"] = "aarch64"
sys["MemorySize"] = 512
sys["Target"] = "virt"
if "CPUCount" in sys and sys["CPUCount"] > 4:
    sys["CPUCount"] = 2

qemu = cfg.setdefault("QEMU", {})
qemu["UEFIBoot"] = True
qemu["Hypervisor"] = True
# CRITICAL: must be a list of tokens. A string makes Start unavailable.
# UTM strips -drive here; kbd/tablet MMIO devices are allowed.
qemu["AdditionalArguments"] = [
    "-machine", "gic-version=3",
    "-device", "virtio-keyboard-device",
    "-device", "virtio-tablet-device",
]

cfg["Display"] = [{
    "DownscalingFilter": "Linear",
    "DynamicResolution": False,
    "Hardware": "virtio-ramfb",
    "HeightPixels": 600,
    "WidthPixels": 800,
    "NativeResolution": False,
    "UpscalingFilter": "Nearest",
}]
# Built-in Terminal (raw Mode "Terminal") — shell visible even if Spice is blank.
cfg["Serial"] = [{
    "Mode": "Terminal",
    "Target": "Auto",
    "Terminal": {
        "BackgroundColor": "#000000",
        "ForegroundColor": "#FFFFFF",
        "Font": "Menlo",
        "FontSize": 12,
        "CursorBlink": True,
    },
}]

# Drop stale Omarchy EFI BootOrder so firmware re-discovers our ESP.
import os
vars_src = pathlib.Path(os.path.expanduser(
    "~/Library/Containers/com.utmapp.UTM/Data/Library/Caches/qemu/edk2-arm-vars.fd"))
vars_dst = data / "efi_vars.fd"
if vars_src.is_file():
    shutil.copyfile(vars_src, vars_dst)
    print(f"EFI vars reset: {vars_dst}")

cfg_path.write_bytes(plistlib.dumps(cfg, fmt=plistlib.FMT_XML))
print(f"Disk file: {target} ({target.stat().st_size} bytes)")
print(f"Drive id:  {disk.get('Identifier')}")
print(f"ImageName: {img_name}")
PY

"$UTMCTL" list | grep -E "UUID|$VM_NAME" || true
echo
echo "One VM: $VM_NAME. Start it in UTM (or: utmctl start $VM_NAME)"
echo "Serial: utmctl attach $VM_NAME"
echo "Expect: Limine → ZealBooter → shell '>'  (not Omarchy)"
echo "UTM freeze M33–M39: rscatalog; runzc UseAdd/Notes/MemSort (see ACCEPTANCE.md)."
