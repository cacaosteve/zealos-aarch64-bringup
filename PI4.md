# Raspberry Pi 4B — UEFI bring-up track

**Decision:** use **Pi UEFI** ([pftf/RPi4](https://github.com/pftf/RPi4)), not native `kernel8.img`.

That keeps the same chain as QEMU/UTM:

```
Pi ROM → start4.elf → RPI_EFI.fd (UEFI)
      → Limine BOOTAA64.EFI
      → ZealBooter → kernel.elf
```

Native firmware+DTB is a fallback only if UEFI on the board is unusable.

## Parallel with UTM

| Track | Goal |
|--|--|
| **UTM / QEMU virt** | ZealOS *compatibility* ladder (HolyC, RedSea, Lattice compose through M199; M200–M206 PL011 RX, shell idle, depth clear, StockLat NULL, PopUpColor headers/titles). M33–M39 freeze bar + eyes-on Lattice notes in ACCEPTANCE.md. Not “full ZealOS” yet; `latticeplay` (DiskLat) ≠ `stockplay` (StockLat body, soft HeadMark DrawIt); `stocklat` = scripted StockLat. |
| **Pi 4B UEFI** | First *physical* platform: boot/timer/IRQ + FB shell **reached**; M200 `uartrx` / RX prep for USB-TTL @ 115200; then SD/USB/GPU. |

Virtio drivers do **not** apply on the Pi. Need BCM2711 UART, GICv2, and later SD/display.

## Boot-stage HDMI bars (PI-DIAG-1)

`make pi-sd` ships **distinct** ELFs (`zealbooter-pi.elf` / `kernel-pi.elf`) built with
`-DZEAL_FORCE_PI4 -DZEAL_PI_DIAG`. Missing DTB never selects QEMU UART `0x09000000`.
The Pi kernel **does not clear** the firmware FB before the first marker, then
halts after kernel entry (no GIC / virtio / JIT).

| Color | Approx Y | Meaning |
|--|--|--|
| Yellow | 40 | ZealBooter entry (Pi path) |
| Green / red | 60 | UART MMIO map OK / FAIL (TX deferred) |
| Orange | 200 | Past map, continuing FB-only |
| Purple | 220 | Enter handoff / module path |
| Teal / red | 240 | Module present / missing |
| Olive / red | 260 | Loading ELF / load fail |
| Cyan | 80 | Jumping to kernel |
| Magenta | 120 | Kernel entry |
| Green / red | 144 | Kernel UART remap OK / FAIL |
| Cream | 168 | About to touch UART (PI-DIAG-2 build) |
| Sky | 184 | PL011 CR enable returned |
| White | 168–194 | UART TX fully returned (overwrites cream/sky) |

Booter leaves `uart` NULL until after the cyan jump so a bad PL011 TX cannot
stall before kernel entry. `uart_phys` is still filled in the handoff. Kernel
enables UARTEN|TXE (keeps firmware baud) and prints `hello from EL… (PI-DIAG-2)`.

QEMU/UTM keep plain `limine.conf` and the non-`-pi` ELFs (virt UART, virtio).

Acceptance:
- **PI-DIAG-1 accepted (2026-09-23):** full booter ladder + magenta + kernel green.
- **PI-DIAG-2a accepted (2026-09-23):** white bar under green remap (map + CR enable OK;
  TX did not reach hot pink).
- **PI-DIAG-2b (2026-09-23):** thick dark-blue bar = sync abort on DR write
  (`0x00000080` looked blue, not red). Next image: sky=FR read OK, pink=byte DR OK,
  bright red=abort; third bar orange=mini-UART poke OK / red=fail.
- **PI-DIAG-2c (2026-09-23):** two full-width reds (FR+DR abort) + short cyan (~60% =
  EC≈0x25 data abort) + short magenta (~10% = DFSC≈2 address-size fault L2). Root
  cause: `pt_v2p` on kernel PIE .bss installed a garbage L2 table PA. Fixed: PT
  pages allocated from usable RAM via HHDM (`mmio_pt_set_pool`).
- **PI-DIAG-2 accepted (2026-09-23):** raw Limine HHDM (no Device punch) — greens for
  FR+DR under remap green, white bar below olive. UART TX path live on Pi 4B HDMI
  markers (TTL optional).
- **PI-DIAG-3 accepted (2026-09-23):** three green soft CNTP ticks + yellow done below
  white UART bar. Timer regs OK; GICv2 next.
- **PI-DIAG-4 accepted (2026-09-23):** Device-map of GICD/GICC — cyan IRQ ticks + white;
  FB text `GICv2 IRQ OK`.
- **Shell entry accepted (2026-09-23):** after IRQ OK, FB shows `ZealOS aarch64 shell`
  and the cmd list. Next: prove UART RX on hardware (`help`/`halt` on USB-TTL @ 115200).
  Soft (M150): `uart_getc_nb` clears RSR/ECR after each DR; RXE already on.
## SD card layout

FAT32 (MBR type `0xEF` recommended by pftf):

| Path | Source |
|--|--|
| `RPI_EFI.fd`, `*.dtb`, `fixup4.dat`, `start4.elf`, … | Unpack [pftf/RPi4 release](https://github.com/pftf/RPi4/releases) zip onto card root |
| `EFI/BOOT/BOOTAA64.EFI` | Our Limine (`make esp`) |
| `boot/zealbooter.elf`, `boot/kernel.elf`, `limine.conf` | From `build/pi-esp/` (`make pi-sd` — Pi-diag ELFs, not virt) |

```sh
# On the Mac, after inserting an SD (example disk name):
make pi-sd          # builds build/pi-esp/ from current ESP
# Then manually copy pftf zip + build/pi-esp/* onto the card
# (script prints exact steps; does not wipe disks by default)
```

## Platform deltas vs QEMU `virt`

| Piece | QEMU/UTM `virt` | Pi 4B (BCM2711) |
|--|--|--|
| Interrupt controller | GICv3 | **GIC-400 / GICv2** |
| UART | PL011 `@ 0x09000000` | PL011 mini/PL011 from DT (often `0xfe201000`) |
| Timer | CNTP + GIC PPI | Same CNTP idea; PPI routing differs |
| Storage | virtio-blk | SDHCI / USB later |
| Input / FB | virtio + ramfb | HDMI FB / USB HID later |

Shared above that: HolyC front-end, JIT, RedSea/CFile APIs, demos.

For USB, reuse the YDE USB core/HID work through ARM64 platform hooks, first
proving native input with UTM's default USB 3.0 configuration. Pi 4B then needs
a separate BCM2711 PCIe/VL805 backend with validated DMA, cache, and MMIO
handling. The current VirtIO input drivers are the recovery path on `virt`;
they do not provide physical Pi USB support. See the
[USB port sequence](COMPATIBILITY.md#usb-direction-reuse-yde-with-arm64-platform-hooks).

## PCIe firmware description probe

The booter now parses the firmware FDT's `brcm,bcm2711-pcie` node and appends
its translated register base, PCI memory window, and DMA window to handoff
revision 5. The parser validates all FDT spans and cell widths and does not
read or write controller MMIO. The Pi diagnostic prints the captured values
after PL011 TX succeeds. A host-built synthetic fixture checks the standard
BCM2711 node translation (`0x7d500000` through `/scb/ranges` to `0xfd500000`),
the PCI memory aperture, the 3 GiB DMA aperture, disabled-node handling, and
truncated-FDT rejection (`make check-pi4-pcie-fdt`). This is parser coverage;
the actual UEFI-provided FDT still needs an eyes-on Pi run.

PI-DIAG now makes one guarded, read-only load of the BCM2711 root-complex
link-status register at offset `0x4068`, only when the FDT marks the bridge
enabled and provides a sufficiently large register window. It reports the raw
value plus PHY-link and Data-Link-active bits. A synchronous MMIO abort is
recorded and the diagnostic continues without touching downstream config
space. This still needs a physical Pi run: the synthetic FDT test does not
validate MMIO mapping or actual link state.

After that probe is confirmed, the next gate is a bounded downstream config
read to identify the VL805. Do not enable bus mastering or start xHCI until the
low-address DMA allocator is constrained to the DTB's DMA window.

## Code plan (small batches)

1. **Docs + `make pi-sd`** — done.
2. **Platform constants** (`plat_pi4.h`) — UART/GIC phys; expand via DT later.
3. **Serial hello (PI-DIAG-2)** — **accepted**.
4. **Soft CNTP (PI-DIAG-3)** — **accepted**.
5. **GICv2 + CNTP IRQ (PI-DIAG-4)** — **accepted** (Device-map FF window).
6. **Serial/FB shell** — **reached** (banner + cmd list after IRQ OK). **M200:** `pl011_rx_prep` +
   `uartrx` for USB-TTL @ 115200; still need hardware eyes-on (`uartrx` then `help`/`halt`).
7. **PCIe discovery** — read-only FDT parser, revision-5 handoff, and guarded
   link-status probe are built; synthetic BCM2711 fixture passes. Confirm the
   actual firmware values and status over Pi diagnostic UART before downstream
   config access.
8. **USB input** — VL805 config reads, firmware handoff, constrained DMA,
   xHCI, and physical keyboard/mouse are separate hardware gates.
9. **Storage / display** — after serial input and native USB input are usable.

## Non-goals (for now)

- Apple Silicon / SystemReady PC (decide after Pi serial).
- Replacing Limine on Pi with bare `kernel8.img`.
- Claiming full ZealOS parity on either target.
