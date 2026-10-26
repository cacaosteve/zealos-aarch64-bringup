#!/usr/bin/env python3
"""Install the ISO image onto a disposable QEMU disk and verify reboot persistence."""
import hashlib
import os
import pathlib
import select
import shlex
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
DISK_BYTES = 128 * 1024 * 1024


def file_digest(path: pathlib.Path) -> bytes:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        while block := stream.read(1024 * 1024):
            h.update(block)
    return h.digest()


class Guest:
    def __init__(self, command: list[str], log_path: pathlib.Path):
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.STDOUT, bufsize=0)
        self.buffer = bytearray()
        self.log = log_path.open("wb")

    def until(self, marker: bytes, timeout: int = 120) -> bytes:
        start = time.monotonic()
        while marker not in self.buffer:
            if self.process.poll() is not None:
                raise RuntimeError("QEMU exited before prompt:\n" +
                                   self.buffer[-4000:].decode(errors="replace"))
            if time.monotonic() - start > timeout:
                raise RuntimeError(f"timed out waiting for {marker!r}:\n" +
                                   self.buffer[-4000:].decode(errors="replace"))
            if select.select([self.process.stdout], [], [], 0.2)[0]:
                data = os.read(self.process.stdout.fileno(), 65536)
                if not data:
                    raise RuntimeError("QEMU serial output closed")
                self.buffer.extend(data)
                self.log.write(data)
                self.log.flush()
        end = self.buffer.index(marker) + len(marker)
        result = bytes(self.buffer[:end])
        del self.buffer[:end]
        return result

    def command(self, line: str, expected: str, timeout: int = 180) -> bytes:
        result = bytearray()
        for byte in line.encode():
            self.process.stdin.write(bytes([byte]))
            self.process.stdin.flush()
            result.extend(self.until(bytes([byte]), timeout))
        self.process.stdin.write(b"\r")
        self.process.stdin.flush()
        result.extend(self.until(b"\n> ", timeout))
        if expected.encode() not in result:
            raise RuntimeError(f"{line}: missing {expected!r}:\n" +
                               result.decode(errors="replace"))
        print(f"PASS {line}: {expected}", flush=True)
        return bytes(result)

    def close(self) -> None:
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
        self.log.close()


def build_command(temp: pathlib.Path, vars_file: pathlib.Path,
                  target: pathlib.Path, iso: pathlib.Path | None) -> list[str]:
    fw = os.environ.get("FW_CODE", "/opt/homebrew/share/qemu/edk2-aarch64-code.fd")
    platform = shlex.split(os.environ.get(
        "QEMU_VIRT", "-machine virt,accel=hvf,gic-version=3 -cpu host -m 512 "
        "-device ramfb -device virtio-keyboard-device -device virtio-tablet-device"))
    command = [os.environ.get("QEMU", "qemu-system-aarch64")] + platform
    command += ["-drive", f"if=pflash,unit=0,format=raw,file={fw},readonly=on",
                "-drive", f"if=pflash,unit=1,format=raw,file={vars_file}",
                "-serial", "stdio", "-monitor", "none", "-display", "none"]
    if iso:
        command += ["-drive", f"if=none,format=raw,file={target},id=target",
                    "-device", "virtio-blk-pci,drive=target,bootindex=1",
                    "-drive", f"if=none,file={iso},id=cd,media=cdrom,readonly=on",
                    "-device", "virtio-scsi-pci,id=scsi",
                    "-device", "scsi-cd,drive=cd,bootindex=0"]
    else:
        command += ["-drive", f"if=none,format=raw,file={target},id=target",
                    "-device", "virtio-blk-pci,drive=target,bootindex=0"]
    return command


def boot_to_prompt(guest: Guest, stage: str) -> bytes:
    start = time.monotonic()
    boot = guest.until(b"\n> ", timeout=240)
    required = ("hc IR OK (front+host)", "virtio-blk: rw OK",
                "zc: native compiler ready", "zc: source Window Manager started at boot",
                "zc: graphical terminal started at boot; serial shell remains available")
    text = boot.decode(errors="replace")
    for marker in required:
        if marker not in text:
            raise RuntimeError(f"{stage} boot missing {marker!r}:\n{text[-5000:]}")
    if "kernel: no usable memory for image" in text:
        raise RuntimeError("ISO modules exhausted the guest's usable memory")
    print(f"PASS {stage} boot and startup desktop markers ({time.monotonic()-start:.1f}s)",
          flush=True)
    return boot


def compare_files(expected: pathlib.Path, actual: pathlib.Path) -> None:
    if expected.stat().st_size != actual.stat().st_size:
        raise RuntimeError("installed disk capacity differs from the source image")
    expected_hash, actual_hash = file_digest(expected), file_digest(actual)
    if expected_hash != actual_hash:
        raise RuntimeError(f"installed disk differs: {expected_hash.hex()} != {actual_hash.hex()}")
    print(f"PASS installed raw disk matches byte-for-byte: sha256={actual_hash.hex()}",
          flush=True)


def main() -> None:
    if len(sys.argv) > 2:
        raise SystemExit("usage: check-iso-installer.py [ISO_PATH]")
    iso = pathlib.Path(sys.argv[1]) if len(sys.argv) == 2 else \
        ROOT / "build/zealos-aarch64-hello.iso"
    if not iso.is_absolute():
        iso = ROOT / iso
    source_disk = ROOT / "build/disk.img"
    if not iso.exists() or not source_disk.exists():
        raise SystemExit(f"missing {iso} or {source_disk}; run `make iso` first")
    fw_vars = pathlib.Path(os.environ.get("FW_VARS_IN", "/opt/homebrew/share/qemu/edk2-arm-vars.fd"))
    with tempfile.TemporaryDirectory(prefix="zeal-iso-install-") as name:
        temp = pathlib.Path(name)
        target = temp / "target.img"
        target.touch()
        with target.open("r+b") as stream:
            stream.truncate(DISK_BYTES)
        vars_file = temp / "vars.fd"
        shutil.copyfile(fw_vars, vars_file)

        live = Guest(build_command(temp, vars_file, target, iso),
                     ROOT / "build/check-iso-installer-live.log")
        try:
            boot_to_prompt(live, "ISO live")
            untouched_hash = file_digest(target)
            live.command("zinstall", "Type zinstall ERASE to confirm.")
            if file_digest(target) != untouched_hash:
                raise RuntimeError("the unconfirmed zinstall command changed the target")
            installed = live.command("zinstall ERASE",
                                     "installed and fully read-back verified.", timeout=600)
            if "erase failed" in installed.decode(errors="replace") or \
               "compare failed" in installed.decode(errors="replace"):
                raise RuntimeError("installer reported an error")
        finally:
            live.close()

        compare_files(source_disk, target)
        # Reset UEFI's volatile boot choice; the target is the only bootable disk.
        shutil.copyfile(fw_vars, vars_file)
        installed_guest = Guest(build_command(temp, vars_file, target, None),
                                ROOT / "build/check-iso-installer-disk-boot.log")
        try:
            boot_to_prompt(installed_guest, "installed disk")
            status = installed_guest.command("zstatus", "desktop: winmgr=1 gterm=1")
            if "presented_frames=" not in status.decode(errors="replace"):
                raise RuntimeError("installed disk did not start the graphical desktop")
            installed_guest.command(
                "zput Tests/InstallerPersist.ZC I64 InstallerPersist(){return 40+2;}",
                "zput: saved Tests/InstallerPersist.ZC")
            installed_guest.command("zload disk:Tests/InstallerPersist.ZC",
                                   "zc: loaded disk:Tests/InstallerPersist.ZC")
            installed_guest.command("zcall InstallerPersist",
                                    "zc => 0x000000000000002a")
        finally:
            installed_guest.close()

        shutil.copyfile(fw_vars, vars_file)
        rebooted = Guest(build_command(temp, vars_file, target, None),
                         ROOT / "build/check-iso-installer-reboot.log")
        try:
            boot_to_prompt(rebooted, "rebooted installed disk")
            rebooted.command("zstatus", "desktop: winmgr=1 gterm=1")
            rebooted.command("zload disk:Tests/InstallerPersist.ZC",
                             "zc: loaded disk:Tests/InstallerPersist.ZC")
            rebooted.command("zcall InstallerPersist", "zc => 0x000000000000002a")
        finally:
            rebooted.close()
    print("PASS ISO installer → disposable VirtIO disk → rebooted desktop and saved data",
          flush=True)


if __name__ == "__main__":
    main()
