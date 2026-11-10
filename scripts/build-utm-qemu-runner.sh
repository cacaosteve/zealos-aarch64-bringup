#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
frameworks=${UTM_FRAMEWORKS:-/Applications/UTM.app/Contents/Frameworks}
out="$root/build/utm-qemu-probe"
mkdir -p "$out"
cat > "$out/entitlements.plist" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict><key>com.apple.security.hypervisor</key><true/></dict></plist>
EOF
clang "$root/scripts/utm-qemu-runner.c" -Wl,-rpath,"$frameworks" -o "$out/qemu-probe"
codesign --force --sign - --entitlements "$out/entitlements.plist" "$out/qemu-probe"
printf 'UTM QEMU runner: %s\n' "$out/qemu-probe"
