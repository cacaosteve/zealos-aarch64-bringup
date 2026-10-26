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
| **Pi 4B UEFI** | Physical boot/timer/IRQ/FB shell, VL805 PCIe discovery, safe xHCI reset, CPU/cache DMA, routed child descriptors, and selecting the child configuration **accepted**. **Direct SuperSpeed enumeration, bulk endpoint configuration and INQUIRY are accepted** (IMG_9130/9131 and IMG_9133/9134: SanDisk Extreme). **Capacity and repeated sector-0 reads are accepted** (IMG_9135/9136). **FAT32 partition/root-directory discovery is accepted** (IMG_9139/9140). **Known FAT32 file reads are accepted** (IMG_9141/9142: 1025 bytes, CRC32 `873b79ac` twice, exact pattern/repeat match). **Persistent read-only controller ownership is accepted** (IMG_9144–9146: 272 startup checks, command/IN/OUT/event wraps 2/2/1/4, file-after-wrap and ten shell idle checks, active/fault/stopped 1/0/0). **Native USB2 boot-keyboard typing is accepted** (IMG_9147–9150: `help`, `usbstatus`, reports/keys `0x86/0x82`, active/fault 1/0). **Basic Apple trackpad input is accepted** (IMG_9151–9154: generic boot-mouse mode, visible movement/left-drag paint, 2451 reports and nine transfer-ring wraps without a fault). Secondary click/multitouch is not yet accepted. **Simultaneous USB2 keyboard/pointer and read-only storage are accepted** (MULTI-USB2-5, IMG_9169–9175): exact data-IN stall recovery, REQUEST SENSE **2/4/1** (not ready, becoming ready), capacity/FAT32/known-file reads, both CRCs **873b79ac**, ten periodic file checks with input online, checks **0b** and active/fault **1/0**. **Automatic combined USB startup and sustained operation are accepted** (MULTI-USB2-6, IMG_9176/9177): automatic **1**, checks through **13** (19 decimal), both CRCs **873b79ac**, input continuing and active/fault **1/0**. **PI-DESKTOP-1** ran native source and HomeSys startup but did not reach a usable desktop (IMG_9180: overlapping text, no typing/pointer response for over two minutes). **PI-DESKTOP-2 physical presentation and interactive terminal are confirmed** (IMG_9182): firmware 1920×1280/pitch 7680, logical 800×600/pitch 3200 scaled 2×, 1632 composed frames, typed `usbstatus`, USB active/fault/stopped **1/0/0**. The user explicitly confirms visible cursor movement, menu clicks and **40+2; → 42**: **native desktop/basic input milestone accepted** (2026-10-26). The byte-identical accepted M6 diagnostic kernel remains a recovery boot entry. Reconnect and persistent filesystem integration remain future work. |

VirtIO transports do **not** apply to physical Pi devices. BCM2711 UART,
GICv2, framebuffer, VL805 DMA, read-only USB storage and basic input now have
physical evidence below, and the native desktop/basic input milestone is accepted.
Native SD, writable persistence and Pi installation remain work.

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

The booter parses Limine's FDT `brcm,bcm2711-pcie` node and appends
its translated register base, PCI memory window, and DMA window to handoff
revision 6. It reserves a 1 MiB, 64 KiB-aligned arena from usable RAM wholly
inside the translated DMA aperture, recording both CPU physical and PCI bus
addresses. The reservation is metadata only: it does not map or use the arena,
or enable bus mastering/xHCI. The parser validates all FDT spans and cell
widths and does not read or write controller MMIO. The Pi diagnostic prints
the captured windows and reservation over HDMI after the GIC/IRQ check; the
HDMI report now also attempts the guarded root-complex link-status read and,
only when the link is active, bounded read-only PCI 01:00.0 identity/header/BAR
reads. The PL011 is optional for this discovery path. This first pass does not
write endpoint config, enable bus mastering, or initialize xHCI.
Host-built synthetic fixtures check the standard
BCM2711 node translation (`0x7d500000` through `/scb/ranges` to `0xfd500000`),
the PCI memory aperture, the 3 GiB DMA aperture, disabled-node handling, and
truncated-FDT rejection (`make check-pi4-pcie-fdt`), plus reservation bounds,
bus-address translation, memory-map splitting, and capacity (`make check-pi4-dma`,
or both together with `make check-pi4-platform`).
These parser/allocator checks are complemented by the physical Pi report below,
which confirms the actual FDT handoff and reserved arena.

The current Pi Limine entry explicitly loads `bcm2711-rpi-4-b.dtb` from the SD
root, rather than relying on UEFI to publish the same tree through its EFI
configuration table. The HDMI report distinguishes a missing Limine response,
invalid pointer/magic/size, missing PCIe node, and parser errors.

**Physical PCIe discovery accepted (2026-10-20):** the Pi 4B HDMI report shows
`FDT status=ok`, flags `0x1f`, register window `0xfd500000+0x9310`, PCI memory
window bus `0xc0000000` → CPU `0x600000000` (1 GiB), and DMA window
CPU/bus `0` with size `0xc0000000` (3 GiB). The 1 MiB DMA arena is reserved at
CPU/bus address `0x04000000`. Link status `0xb0` has both PHY-link-up and
Data-Link-active bits set. Read-only config access finds PCI `01:00.0` ID
`1106:3483` (VIA Labs VL805), class `0x0c0330` (xHCI), revision 1, with BAR0
`0xf8000004`. The command register remains zero, so memory decoding and bus
mastering are disabled; the diagnostic explicitly leaves xHCI off. This accepts
FDT translation, DMA reservation, link gating, and endpoint identity on the
physical Pi. It does not yet show USB device enumeration or keyboard/mouse input.

The HDMI FDT/PCIe discovery report is confirmed on the physical Pi and needs no
serial adapter. For interactive UART input/logs, connect a USB-to-3.3 V TTL
adapter to the Mac and cross its RX/TX with Pi GPIO 14/15, plus GND. Leave the
adapter's VCC wire disconnected.

PI-DIAG makes a guarded load of the BCM2711 root-complex link-status
register at offset `0x4068`, only when the FDT marks the bridge enabled and
provides the register window. When both PHY link and Data Link are active, it
maps the controller window as Device MMIO and reads bus 1/device 0/function 0
through the BCM2711 indirect config index and data registers. It reports the
vendor/device ID, class/revision, header type, command/status, and the BAR
dwords valid for a type-0 endpoint or type-1 bridge. It validates the FDT
window, alignment, and address bounds, rejects link-down access, and recovers
synchronous MMIO aborts. Host tests cover the config address plan and the
BAR-to-CPU-window translation plus the rule that bus mastering must already be
off (`make check-pi4-platform`).

The physical capability-probe photo (captured 2026-10-21 at 09:44, before the
live-window fix was built) shows BAR0 `0xf8400004` and `0xdeadbead` in the xHCI
capability registers. The read-only root-complex report captures window 0 as
PCI bus base `0xf8000000`, CPU base `0x600000000`, and 64 MiB, while the FDT
describes a broader `0xc0000000` → `0x600000000` 1 GiB aperture. The old FDT
translation put BAR `0xf8400000` at CPU `0x638400000`, outside the live window;
window 0 maps it to CPU `0x600400000`. The error registers remained clear.
This explains why the old guarded reads returned poison values.

The updated Pi kernel (2026-10-21) derives the BAR's CPU address from the live
outbound window registers, after verifying the BAR and complete capability
header fit both that window and the FDT bus aperture. The host checks pass, and
the kernel was built at 09:54, staged at 09:59, then copied to the microSD with
the prior kernel backed up. The 09:44 photo predates that copy.

**Physical xHCI capability read accepted (2026-10-21 10:27):** the fresh Pi
photos show BAR0 `0xf8400004` translated to CPU `0x600400000` by the live window,
with valid xHCI capability/version, structural parameter, doorbell, and runtime
register values instead of the earlier `0xdeadbead` poison. PCIe error-valid
fields remain clear and the memory-error address/cause are zero. The diagnostic
restores the endpoint command word, leaves Bus Master Enable clear, does not
write any xHCI register, and does not use the reserved DMA arena. This accepts
safe capability-header access on the physical Pi; it does not mean xHCI has
started or that USB devices are usable. Firmware handoff, cache-safe DMA,
controller startup, and physical HID input remain later steps.

**Operational and root-port snapshots accepted (2026-10-21):** fresh Pi
photos show the read-only USBCMD/USBSTS and USBLEGSUP snapshots plus five
bounded PORTSC values (the controller reports five root ports). The report still
says `bus master off; xHCI not started`; no xHCI register writes or DMA
occurred. This verifies that capability, operational, ownership, and port
status can all be inspected through the checked BAR/window mapping while
preserving the safety boundary. Port status is not device enumeration or HID
input.

**Physical xHCI firmware handoff accepted (2026-10-21):** the latest Pi 4B
photos show `handoff=firmware-released` after the guarded USBLEGSUP OS-owned
request. PCI bus mastering stayed off and xHCI remained stopped. This accepts
the firmware handoff gate; it does not yet verify controller reset or USB I/O.

The next staged diagnostic performs a bounded Host Controller Reset only after
the handoff succeeded and fresh reads confirm `RUN=0`, `Halted=1`, `CNR=0`, and
PCI Bus Master Enable clear. It writes only `USBCMD.HCRST`, waits up to one
counter-second for HCRST to clear and CNR to return to zero, then confirms the
controller is halted with Run/Stop clear. It does not set Run/Stop, program DMA
structures, enable Bus Mastering, or use the reserved DMA arena. Per xHCI, this
reset reinitializes operational and root-port state; USB 3 downstream ports may
receive a hot/warm reset, and USB input can remain unavailable until later
controller initialization or a reboot. This is a physical reset/readiness test,
not device enumeration.

**Physical halted reset accepted (2026-10-21):** the latest Pi 4B photos show
`handoff=firmware-released`, `HCRST=complete`, and post-reset command/status
values. The kernel reports `complete` only after fresh reads confirm Run/Stop
and HCRST clear, Halted set, and CNR clear. The microSD's `boot/kernel.elf`
matches the staged Pi diagnostic image. This verifies that the VL805 accepts a
bounded HCRST after firmware releases legacy ownership while PCI Bus Master
Enable stays clear. Keep Bus Master Enable off until the DMA arena and all
controller-visible structures have explicit cache and address-translation
validation.

The next image runs a CPU-only check after the reset gate. It verifies that the
reserved DMA range is represented exactly once in the memory map, that its CPU
and bus addresses agree with the firmware DMA aperture, and that the HHDM
address is safe. It writes two cache lines, cleans them to PoC, invalidates
them, checks the readback, and cleans the reserved lines back to zero. It must
report `DMA CPU/cache=pass line=0x40` on the Pi 4B. This confirms CPU access and
the cache-maintenance instructions for the reserved mapping; it does not
exercise device-initiated DMA or establish that a controller can see the data.
Bus mastering stays disabled and xHCI stays stopped. Only after this check is
accepted should we add a bounded device-DMA test and then controller startup,
enumeration, and physical HID input.

**Physical CPU-only DMA/cache validation accepted (2026-10-21):** the latest
Pi photo shows `DMA CPU/cache=pass line=0x40 (CPU-only; xHCI stopped)`. This
confirms the reserved arena mapping, CPU access, 64-byte cache-line maintenance,
and readback on the real board. It does not prove that the VL805 can fetch or
write DMA structures. The tested pair was backed up on the card under
`ZEALOS-BACKUP-20261021-DMA-CPU`; its kernel and booter hashes matched the
staged files, with the UEFI firmware, DTBs, and config left unchanged.

**Bounded xHCI No-op DMA probe (2026-10-21):** the probe runs only after fresh
halted/reset checks and the physical CPU/cache pass. It allocates a DCBAA, one
command ring, one event ring, an ERST, and any advertised scratchpads inside the
reserved 1 MiB arena. It verifies 4 KiB page support, alignment, the firmware
DMA aperture, and 32-bit limits when AC64 is absent. PCI Bus Master Enable is
set only after the structures are initialized and cache-clean. The controller
starts with one-slot capacity but no enabled slots; the sole command is xHCI
No-op. A matching Command Completion Event proves command fetch and event write
through device DMA. No port is enumerated and no USB data transfer is submitted.
The probe then halts the controller and restores the original PCI command word,
verifying Bus Master Enable is clear.

The first Pi run reported `xHCI DMA no-op=invalid-layout events=0x00000000`.
The follow-up photo reaches preflight stage 4 with layout rc `0xffffffff`,
failure mask `0x0000000a`, fresh USBCMD/USBSTS `0xdeaddead`, USBLEGSUP
`0x01000401`, and original PCI command `0x0000`. Bits 1 and 3 mark the
RUN/HCRST and CNR safety checks. This is not a live controller state: the
capability/state probe had temporarily enabled PCI Memory Space Enable (MSE),
then restored the original command word; the DMA preflight subsequently read
the BAR with MSE clear. The VL805 returned the repeated `0xdeaddead` poison
value, and the safety gate correctly prevented the no-op command and kept BME
off.

The new image enables and verifies MSE with BME still clear before its fresh
operational-register reads. It restores the original PCI command on every
preflight rejection; if all layout checks pass, it keeps MSE through the
one-command probe and restores the original command after halting. PAGESIZE
still must have 4 KiB support and no reserved upper bits. Host tests cover that
check, layout bounds, safety gates, and completion parsing.

**Physical MSE preflight accepted (2026-10-21):** the next Pi photo shows
preflight stage 4, layout rc `0`, PAGESIZE `1`, failure mask `0`, fresh
USBCMD/USBSTS `0x00000000/0x00000011`, USBLEGSUP `0x01000401`, and original PCI
command `0x0000`. Thus the operational BAR is readable with MSE, 4 KiB pages
are supported, the controller is halted and out of reset, ownership is with
the OS, and all six safety gates passed. The report still said
`skipped-safety-gate`: the caller had inverted the helper's conventional
zero-means-success return and stopped before DMA structures were built. That
call-site check is corrected. The next card image retries only the existing
bounded one-command DMA proof; it still does not enable a slot or submit USB
data. The card's tested kernel was
`c81beca86c8958c35a16c72439911e905797a16c6589bd605de9459ea5cc805c` and its
booter was unchanged at
`6716f2268ea2d68fe74567fcadd60f64df27288227a638eb8b947fe8ee95f782`.
The retry is now staged and readback-verified on the card: kernel
`b134de17e329f67dd4ec29dca83528227ae4635399f53dec8c72abb3f3d3400c`, booter
`6716f2268ea2d68fe74567fcadd60f64df27288227a638eb8b947fe8ee95f782`. The
replaced pair is backed up under
`ZEALOS-BACKUP-20261021-XHCI-NOOP-RETRY`; UEFI, DTBs, and config were untouched.
The card was safely ejected after byte-for-byte verification.

**First physical No-op DMA attempt (2026-10-21):** the next Pi screenshot
reports `xHCI DMA no-op=noop-timeout events=0`, preflight stage 6, layout rc
`0`, PAGESIZE `1`, and safety mask `0`. This means the controller passed the
preflight, accepted RUN (the separate result would be `run-timeout` if Halted
did not clear), and received the No-op doorbell, but no completion event was
visible in the one-second poll. The result remained `noop-timeout`, so the
cleanup reported neither a stop timeout nor a PCI restore failure. The next
diagnostic image records the live USBCMD/USBSTS, CRCR, ERDP, and first event
TRB immediately before the controller is halted, and advances the stage marker
through BME, RUN, and doorbell so the next photo localizes the stall.
That trace image is staged on the card with kernel
`bcac1995c53199a779bb06a4e42ca32ac431667adbe462498f07f1fc97a1ce99` and the
unchanged booter hash above. The replaced pair is saved under
`ZEALOS-BACKUP-20261021-XHCI-NOOP-TRACE`; only those two boot files were
changed, and the card was safely ejected after byte-for-byte verification.

**Physical No-op trace result (2026-10-21):** the Pi photo shows stage 11 after
the one-second No-op wait, `events=0`, live USBCMD/USBSTS `1/0x10`, CRCR low
`0x8`, ERDP pointing at the event-ring base, and an all-zero first event TRB.
CRCR bit 3 is CRR, so the command ring was reported running; CRCR does not
provide a useful pointer readback because its pointer and RCS fields read as
zero. The screenshot therefore narrows the stall to command processing or
event production, but does not identify which DMA leg failed. The next bounded
image checks CONFIG/DCBAAP/ERSTSZ/ERSTBA/ERDP readbacks while BME is still off.
If the No-op again produces no event and CRCR.CRR is set, it requests command
abort and waits one more second for a command-completion event before halting
xHCI and restoring the original PCI command. The diagnostic never enables a
slot or submits a USB data transfer.

The setup-readback/command-abort diagnostic is built and staged for the next
Pi boot. Its kernel hash is
`00e788479b4eceb2aa56d16edc8cdf44041629ab9c7113c1195f365f85bb74d1`; the
booter remains `6716f2268ea2d68fe74567fcadd60f64df27288227a638eb8b947fe8ee95f782`.
The previous pair is backed up at
`ZEALOS-BACKUP-20261021-XHCI-ABORT-DIAG/boot`; only `boot/kernel.elf` and
`boot/zealbooter.elf` were replaced. The staged files were byte-for-byte
verified on the card, UEFI firmware/DTBs/config were left untouched, and
`/dev/disk14` was safely ejected.

**Second physical No-op attempt (2026-10-21):** the latest photos show
`noop-timeout`, zero events, preflight stage `0xE`, and abort state `3` (the
bounded abort-event wait expired). Setup-readback mask `0x1F` shows all five
pre-BME reads completed; CONFIG and ERSTSZ read back as `1`, and the three
address registers match the programmed values. This narrows the failure beyond
the previous iteration: xHCI accepted the setup register writes, but the No-op
and abort still produced no visible event. Reviewing the xHCI initialization
rules found that we wrote ERSTBA before ERDP. The [Intel xHCI specification](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf)
orders ERSTSZ and ERDP before ERSTBA, whose write starts the event-ring state
machine. The next image corrects that sequence; all tests pass, but it still
needs this physical verification.

The ERST-order image is now staged on the card with kernel
`31bcd4ccc22c7c01f963195fcf9a230dedd05dfd3127d7151c1bd10bf0ab571f` and the
unchanged booter hash above. The replaced pair is backed up under
`ZEALOS-BACKUP-20261021-XHCI-ERST-ORDER`; only the kernel and booter were
replaced. Both staged files were byte-for-byte verified, UEFI firmware/DTBs/
config were untouched, and the card was safely ejected.

**Third physical No-op attempt (2026-10-21):** the latest photos show the
ERST-order image still reports `noop-timeout` with zero events. The card's
kernel hash was `31bcd4ccc22c7c01f963195fcf9a230dedd05dfd3127d7151c1bd10bf0ab571f`,
confirming that this was the expected build. The preflight still reached stage
`0xE`, all gates were clear, and setup readback mask `0x1F` confirmed the
programmed registers. So changing ERST register order alone did not make the
controller produce an event.

Reviewing the [Intel xHCI specification's register conventions](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf)
found another issue: HCCPARAMS1 reports AC64, but our 64-bit address-register
helper emitted two 32-bit stores, high half first. The spec calls for one
Qword access when AC64 is supported; split writes, when unavoidable, must be
low half first. The next image uses one aligned AArch64 Qword MMIO store for
the xHCI address registers and writes only the low half if AC64 is clear.
The Qword-access image is built and staged on the card with kernel
`c69ecb5c05efaf929bf61802ddd0ab60ac82d845411d1f67f6e6e840c40148b7`; the
booter is unchanged (`6716f2268ea2d68fe74567fcadd60f64df27288227a638eb8b947fe8ee95f782`).
The prior pair is backed up under
`ZEALOS-BACKUP-20261021-XHCI-QWORD-MMIO/boot`. Only the kernel and booter were
replaced, the staged files were byte-for-byte verified, UEFI firmware/DTBs/
config were untouched, and the card was safely ejected. The following
physical-attempt note records the result from this image.

**Fourth physical No-op attempt (2026-10-21):** the Pi booted the Qword-MMIO
kernel confirmed by the card hash, but the photo still shows `noop-timeout`,
`events=0`, successful pre-BME setup readbacks, and abort state `3`. The
Qword-access correction therefore did not resolve event production. A further
specification review found that the probe configured the primary event-ring
registers but omitted the primary interrupter's IMAN and IMOD registers before
setting USBCMD.RUN. The [Intel xHCI specification](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf)
requires all primary interrupter registers to be initialized before RUN. The
next bounded image clears a pending IMAN.IP while keeping IMAN.IE disabled for
polling, sets IMOD to zero, and verifies both readbacks before enabling PCI bus
mastering. This addresses a concrete initialization omission; physical No-op
completion still needs verification.

The IMAN/IMOD initialization image is staged on the card. Its pre-BME
setup-readback mask now requires seven fields (`0x7F`), and the diagnostic
prints IMAN and IMOD alongside CONFIG, ERSTSZ, ERSTBA, and ERDP. The tests
also reject an enabled/pending IMAN or nonzero IMOD readback. `make
check-pi4-platform pi-diag pi-sd` passed; the build emitted existing compiler
warnings. The kernel hash is
`f48a6de3df9fbfeb64e3518798b8c7b4cbcddee56c0b357d7c0ad6dc55b1c6a4`, and the
unchanged booter hash is
`6716f2268ea2d68fe74567fcadd60f64df27288227a638eb8b947fe8ee95f782`. The
replaced Qword-MMIO pair is backed up under
`ZEALOS-BACKUP-20261021-XHCI-IMAN-IMOD/boot`; both staged files were
byte-for-byte verified and `/dev/disk14` was safely ejected. Only the kernel
and booter files were copied; firmware, DTBs, and configuration were left
untouched.

**Fifth physical No-op attempt (2026-10-21):** the latest Pi photos show the
IMAN/IMOD image reached the running-controller test (`stage=0xE`), with all
seven pre-BME setup readbacks accepted (`setup mask=0x7F`), but still timed out
with zero events. The bounded abort also timed out. This confirms that
interrupter initialization alone did not fix command completion. Comparing
the host doorbell path with the Linux xHCI driver revealed a missing posted
write flush: the driver reads back the doorbell register after writing it.
The next diagnostic adds that same-register readback and captures USBSTS,
CRCR, and IMAN before issuing Command Abort, so abort-side state changes no
longer obscure the timeout state. The probe remains limited to one No-op and
restores PCI Bus Master Enable off afterward.

The doorbell-flush diagnostic builds and all `make check-pi4-platform`
checks pass. Existing compiler warnings remain. Its kernel is
`d638bff08ee449cc30c86b1af8fa6961a07bbadb880e134f6b7fae6d775680c1`; the
booter remains
`6716f2268ea2d68fe74567fcadd60f64df27288227a638eb8b947fe8ee95f782`.
It is staged to the mounted card with the prior boot pair backed up before
copying under
`ZEALOS-BACKUP-20261021-XHCI-DB-FLUSH/boot`. Both new boot files were
byte-for-byte readback-verified against the build and the card was safely
ejected. The UEFI firmware, DTBs, and config remain untouched.

**Doorbell-flush physical result (2026-10-22):** the latest photos show the
doorbell readback completing, but the No-op still times out with zero events;
CRCR.CRR remains set through the bounded abort wait, and the event TRB remains
zero. Setup readbacks are complete (`0x7F`), PCIe error-valid fields are clear,
and cleanup reports `HCRST=complete` with Bus Master Enable restored off. The
probe still does not enable a slot or submit USB data.

The Pi screenshots reattached on 2026-10-23 confirm the same prior-image
address mismatch directly: the reported DMA window is CPU/bus `0/0` with size
`0xC0000000`, while the arena is CPU `0x04000000` and bus `0x40000000`. They
also show setup readback mask `0x7F`, zero events, live `CRCR.CRR=1`, and
`noop-timeout`. These are evidence from the earlier image, before the current
identity-mapping and Link-TRB pair was written to the card; they do not verify
the current candidate.

The same report exposes an address inconsistency that needs to be resolved
before changing more xHCI sequencing: it prints the PCIe DMA window as CPU/bus
base `0/0`, size `0xC0000000`, but the reserved arena as CPU `0x04000000`, bus
`0x40000000`, and programs the xHCI rings at that bus address. For the displayed
identity DMA window, this allocator should hand xHCI bus address
`0x04000000`. The [Raspberry Pi Linux BCM2711 PCIe node](https://github.com/raspberrypi/linux/blob/rpi-6.18.y/arch/arm/boot/dts/broadcom/bcm2711.dtsi#L3145-L3201)
also describes the Pi 4B PCIe DMA range as 0 to 0 for the first 3 GiB. The old
host-test fixture covered a non-identity offset but not the identity mapping
reported by this Pi. Regressions now check both the allocator directly and the
end-to-end FDT-parse-to-arena-reservation path for the 0-to-0 case. The next
card image is rebuilt from the current matching booter/kernel sources; first
verify that its reported arena CPU and bus addresses agree, then use the same
bounded No-op probe to see whether the controller produces an event. This
separates a stale-image/handoff mismatch from the remaining device-DMA issue.

There is an internal consistency check here: the CPU-cache probe and xHCI
preflight call the same DMA-window validator before bus mastering. If the
printed 0-to-0 window and 64 MiB-to-1 GiB arena addresses were all the values
used by those checks, the probe would stop with an invalid-window result rather
than reach `noop-timeout`. So the photo is evidence of an inconsistent report
or image/handoff pair, not yet proof that the VL805 actually received a bad
address. The new end-to-end host regression confirms that the current matched
source pair maps this reported FDT to arena bus `0x04000000`.

The fresh matching pair is built and staged at `build/pi-esp`. The kernel
SHA-256 is
`e01e6aabb745fff83ae84342a64064712e92f1ae72bef01b3973e7fa783294ca`; the
booter SHA-256 is
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. Both
staged files compare byte-for-byte with their Pi-specific build outputs. The
new identity-mapping host regression and all `make check-pi4-platform` checks
pass.

**Identity-DMA card image (2026-10-23):** the new matching pair was copied to
the mounted `ZEALPI` FAT32 boot volume. The previous active files were backed
up at `ZEALOS-BACKUP-20261023-IDENTITY-DMA/boot/`. The live card files compare
byte-for-byte with the staged pair: kernel
`e01e6aabb745fff83ae84342a64064712e92f1ae72bef01b3973e7fa783294ca`, booter
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. No
firmware, DTB, Limine, or configuration files were changed. Next, boot this
card on the Pi 4B and capture the FDT DMA window, arena CPU/bus addresses, and
bounded xHCI No-op report; expect the identity map to give CPU and bus arena
addresses both `0x04000000`.

**Command-ring format correction (2026-10-23):** a source audit found the
one-shot command ring had a valid No-op at TRB 0 but left the segment's final
TRB zero. The [xHCI 1.2b specification](https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf)
requires the last TRB in each command-ring segment to be a Link TRB; for a
single-segment ring it points back to the segment base. The ring builder now
seeds that Link TRB with cycle state 1 and Toggle Cycle set, with a host test
for the pointer and flags. This did not explain the first No-op timeout by
itself, but removes a malformed ring from the device-DMA test. The updated
`make pi-sd` pair passes `make check-pi4-platform` and was copied to the card;
the former identity-DMA pair is preserved at
`ZEALOS-BACKUP-20261023-XHCI-LINK-TRB/boot/`. The current kernel SHA-256 is
`bd537d6301279cdfbebb450cb728902563c80d40ac2149b82870d0a706b90992`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. The
pair passed host validation and was staged to the Pi card; the physical result
is recorded below.

**Identity-DMA + Link-TRB physical result (2026-10-23):** the attached Pi
photos match the card's prior kernel
`bd537d6301279cdfbebb450cb728902563c80d40ac2149b82870d0a706b90992` and
booter `0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
The FDT window is CPU/bus `0/0` with size `0xC0000000`; the reserved 1 MiB
arena is CPU/bus `0x04000000`, and CPU cache validation passes. The xHCI
pre-BME setup mask is `0x7F`, so all seven setup readbacks passed. The bounded
No-op still times out with zero events; live status is `USBSTS=0x10` and
`CRCR.CRR=1`, so the command ring remains reported running. Correction:
`USBSTS=0x10` is Port Change Detect (PCD), not Host System Error (HSE); HSE is
bit 2 (`0x04`). This verifies the identity-map fix and narrows the remaining
failure to xHCI command/event processing after RUN, rather than the firmware
CPU-to-bus mapping.

The next candidate applies the real VL805 `TRB_OVERFETCH` workaround from the
[emu68 xHCI driver](https://github.com/rondoval/emu68-xhci-driver): each logical
4 KiB ring now has an 8 KiB-aligned, 8 KiB zeroed backing allocation, while
the Link TRB and ERST continue to describe only the logical 256-TRB segment.
This gives the controller owned memory if it prefetches beyond a segment
boundary. Host coverage checks the alignment, spacing, and arena bounds. The
candidate is staged under `build/pi-esp`; kernel SHA-256 is
`f1919abe30cccf7e5340a05257c8c68d798ec6ab3b0f7aacbc5637389efb64c1` and
booter SHA-256 remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
The pair is copied to the microSD and byte-for-byte verified. Its former
kernel (`bd537d6301279cdfbebb450cb728902563c80d40ac2149b82870d0a706b90992`)
and unchanged booter are backed up under
`ZEALOS-BACKUP-20261023-VL805-OVERFETCH/boot/`; firmware, DTBs, and config are
untouched. The card was safely ejected. This candidate still needs a physical
Pi run. The linked [RISC OS forum post](https://www.riscosopen.org/forum/forums/4/topics/15609?limit=25&page=1#posts-107982)
is background only: it asks whether that era's RISC OS stack used the Pi's
USB 3 sockets in SuperSpeed mode; the reply says it did not at that time. It
does not describe the initial VL805 DMA/No-op path tested here.

**VL805 overfetch physical result (2026-10-23):** the new Pi photos show the
identity FDT DMA range (`0/0`, size `0xC0000000`) and a passing CPU cache test;
the reserved arena is CPU/bus `0x04000000`, size `0x00100000`. The setup mask
is `0x7F`, with `DCBAAP=0x04000000`, `ERDP=0x04022000`, and
`ERSTBA=0x04024000`, matching the padded ring layout for this controller's
advertised scratchpad count. The No-op still times out with zero events and
`CRCR.CRR=1`; the status is `USBSTS=0x10` (PCD), not HSE. The 8 KiB VL805
overfetch backing therefore did not remove the command/event stall.

That phase-trace candidate recorded USBSTS immediately after RUN and after the
No-op doorbell, checked HSE bit `0x04`, and skipped the doorbell if HSE was
already set. It remained limited to one No-op and did not enable a slot or
transfer USB data. Its kernel SHA-256 was
`d0140eaffc31115190c7829fc6db1abe49aad5739729e40f5c564d1e153e4e48`; the
booter was
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. Both
files were byte-for-byte verified; the replaced overfetch pair is backed up
under `ZEALOS-BACKUP-20261023-PHASE-TRACE/boot`.

**Independent event-ring DMA diagnostic (2026-10-23):** the latest photos still
show the correct identity-mapped DMA window and arena, all seven setup readbacks
passing, then `noop-timeout`, zero events, `USBSTS=0x10` (PCD), `CRCR.CRR=1`,
and abort state 3 (bounded abort wait expired). The 8 KiB VL805 overfetch
padding and the corrected HSE-bit phase trace did not resolve the stall. The
next probe now sets USBCMD.EWE when starting xHCI and waits up to three
counter-seconds for MFINDEX to wrap and the primary interrupter's MFINDEX Wrap
Event before ringing the command doorbell. A successful type-39 event proves
the controller wrote the event ring through DMA independently of command-ring
fetch; a stalled MFINDEX is inconclusive when every root port is disconnected,
disabled, training, or powered off. After this bounded check, EWE is cleared
and the image still issues only one No-op Command, then halts xHCI and restores
PCI Bus Master Enable off. The event TRB type and layout follow the [Intel
xHCI specification](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf).

The image passes `bash -n scripts/pi-sd-stage.sh`, `git diff --check`, and
`make check-pi4-platform`; `make pi-sd` rebuilt the Pi-specific ELFs. Kernel
SHA-256 is
`13e782f10115b083fa56adc53ec34a07d6a5db7c0dbaae55a847bf3f101b07b6`; booter
SHA-256 remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. The
pair was copied to the mounted `ZEALPI` card, byte-for-byte verified, and the
previous pair was preserved under
`ZEALOS-BACKUP-20261023-MFINDEX-WRAP/boot/`. The card was safely ejected.
Next, boot it on the Pi 4B and photograph the full `xHCI event-ring wrap`,
MFINDEX, status/control, No-op, USBSTS/CRCR/IMAN, and abort-state lines so the
new independent event-DMA check can locate the stall.

**MFINDEX-stall physical result (2026-10-23):** photos `IMG_8903.heic` through
`IMG_8905.heic` show the candidate above reached the DMA probe. The FDT window
and reserved arena remain valid (`0/0`, 3 GiB; arena CPU/bus
`0x04000000/0x04000000`, size 1 MiB), and the CPU/cache check and all seven
setup-register readbacks pass. The independent event ring remains empty:
MFINDEX is `0/0`, event status/control are `0xffffffff/0xffffffff`, and the
No-op still times out. The earlier PORTSC snapshot showed port 1 connected,
enabled, powered, and at high speed (`0x40000e03`), but that snapshot was taken
before the probe's HCRST; the latest image did not record the post-reset port
state. The Intel xHCI specification says an enabled root port should generate a
type-34 Port Status Change Event when its port-change signal transitions, and
the primary event ring receives these events.

The next candidate adds a bounded, post-HCRST root-port test: it reports the
live port state, clears only asserted RW1C change bits, briefly resets the first
powered/connected port, and waits up to one counter-second for the matching
type-34 event and PRC. This can briefly reset a connected USB device, but it
does not enable a slot or transfer USB data. The diagnostic also accepts and
records a valid MFINDEX event while waiting for the port event. This separates
an idle/disabled-port MFINDEX stall from an event-ring DMA failure and puts the
port link in a known reset path before the single No-op command. `make check-pi4-platform`,
`make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass.
Candidate kernel SHA-256 is
`feaa0de44a497e3eccbb8de0b9617ddf5270a381b915770d8fdffb61fe145ec3`; booter
SHA-256 remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
The pair is staged on the microSD and byte-for-byte verified. The previous
kernel (`13e782f10115b083fa56adc53ec34a07d6a5db7c0dbaae55a847bf3f101b07b6`)
and unchanged booter are backed up under
`ZEALOS-BACKUP-20261023-PORT-RESET-EVENT/boot/`; firmware, DTBs, and config
were left untouched. The card was safely ejected. Boot it on the Pi 4B with
HDMI attached and photograph the full report, especially the post-reset
`PORTSC`, port-reset-event state and TRB words, MFINDEX, No-op, USBSTS/CRCR,
and abort-state lines. The diagnostic briefly resets the first powered,
connected USB root port it finds; keep that in mind if a keyboard or other
device is connected.

**Port-reset/MFINDEX physical result (2026-10-23):** photos `IMG_8906.heic`
through `IMG_8908.heic` show the `feaa0de4…` candidate. The FDT identity DMA
window (`0/0`, 3 GiB), arena CPU/bus addresses (`0x04000000/0x04000000`), CPU
cache check, and all seven pre-BME register readbacks still pass. The event-ring
wrap test reported `mfindex-stalled` while the connected port was still
disabled. The subsequent bounded reset did activate port 1: PORTSC changed from
`0x400202e1` to `0x40200e03`, with PED and PRC set, and USBSTS reported PCD
(`0x10`). Yet the type-34 event ring remained empty and the No-op timed out with
zero events and CRCR.CRR still set. MFINDEX later read `0x1f40`, showing it
advanced after the port reset; that end sample was taken after the earlier
wrap-poll timeout. This candidate therefore confirms the port reset state
change but still does not prove event-ring DMA.

The next diagnostic moves the connected-port reset before the MFINDEX test. It
waits up to one counter-second for the port event, then gives an active MFINDEX
up to three counter-seconds to wrap and report its type-39 event. If the counter
wraps but the ring stays empty, that will isolate event-ring delivery from the
previous all-ports-disabled condition. The one-No-op limit and no-slot/no-data
policy remain. `make check-pi4-platform`, `make pi-sd`, shell syntax, and
`git diff --check` pass. Kernel SHA-256 is
`89fe7f7e91bcaf3f903450c9fb70686c04355a856c0199d76046dabbab7b4f99`; booter
SHA-256 remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. This
pair is staged on the microSD and byte-for-byte verified. The replaced pair is
preserved under `ZEALOS-BACKUP-20261023-RESET-FIRST-MFINDEX/boot/`; firmware,
DTBs, Limine, and configuration were untouched. The card was safely ejected.
Boot it on the Pi 4B and capture the full xHCI report, especially the
post-reset PORTSC, event-ring wrap state and MFINDEX start/end, the type-34
event result, No-op, USBSTS/CRCR/IMAN, and abort-state lines.

**Reset-first/MFINDEX-wrap physical result (2026-10-23):** the latest Pi
photos show the `89fe7f7e…` candidate reached the diagnostic and completed the
connected-port reset: PORTSC moved from `0x400202e1` to `0x40200e03`, with PED
and PRC set and USBSTS PCD (`0x10`). The MFINDEX start/end samples were
`0x1f40/0x018f`, so it advanced through wrap. The primary event ring remained
empty, with event status/control still `0xffffffff/0xffffffff`; the reset-event
wait did not find a type-34 event, and the No-op timed out with zero events and
CRCR.CRR set. Thus the port state transition and running MFINDEX are proven,
but event-ring posting is not. The current card kernel hash matched
`89fe7f7e91bcaf3f903450c9fb70686c04355a856c0199d76046dabbab7b4f99`.

The next candidate sets the primary interrupter's `IMAN.IE` while keeping
global `USBCMD.INTE` clear. Per the [Intel xHCI specification](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf),
`IMAN.IE` permits interrupt generation; it does not gate Event TRB writes.
This is a spec-conforming initialization check, while polling remains enabled
and host IRQ delivery stays off. It is not expected to fix a missing DMA event
by itself, though the physical result will check for a VL805-specific
difference. The image still performs only the connected-port reset and one
No-op, enables no slot, submits no USB data, then halts xHCI and restores PCI
bus mastering off. Host tests check that IMAN.IE is read back and that the RUN
value explicitly leaves USBCMD.INTE clear. Candidate kernel SHA-256 is
`d2b448a3668bf2ff10f58b85c9bce744c8559eba9f26d4bb0b367b4b62ba334d`; booter
SHA-256 remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. Both
files were byte-for-byte verified on the card. The replaced `89fe7f7e…` pair
is backed up under `ZEALOS-BACKUP-20261023-IMAN-IE-TEST/boot/`; the card was
safely ejected. The previous pair is preserved at that backup path. Firmware,
DTBs, Limine, and configuration were untouched. Boot this candidate on the Pi
4B with HDMI attached and photograph the full xHCI report, especially IMAN,
MFINDEX start/end, event-ring wrap result and TRB, port-reset event result and
TRB, No-op, USBSTS/CRCR, and abort state. The expected IMAN readback has IE
set, while USBCMD.INTE remains clear.

**IMAN.IE physical result and next test (2026-10-23):** photos `IMG_8912.heic`
through `IMG_8914.heic` show that the `d2b448a3…` candidate reached the probe
with the expected FDT identity DMA range (`0..0xc0000000`) and arena
CPU/bus=`0x04000000/0x04000000`. All seven xHCI setup readbacks passed and
IMAN read back as IE enabled, but the port-reset event, MFINDEX Wrap Event,
and No-op completion were still absent. MFINDEX advanced through wrap
(`0x1f40` to `0x018f`); the event TRB remained zero and the No-op timed out.
This rules out the inactive-counter explanation, but the FDT describes an
address translation contract rather than proving that the firmware-programmed
BCM2711 inbound windows actually match it. The Raspberry Pi Linux host driver
describes the BCM2711 `RC_BAR[1..3]_CONFIG` registers as inbound windows and
configures BAR2 from `dma-ranges`; see its
[readback and setup code](https://github.com/raspberrypi/linux/blob/rpi-6.18.y/drivers/pci/controller/pcie-brcmstb.c#L1088-L1268).

The image added a read-only snapshot of inbound BAR1/2/3 and `MISC_CTRL`. Its
kernel SHA-256 was
`f07977ba61b126595cb8647d9891217d45121b3d7635d5ecfe6c7b093d3f4c59`; booter
SHA-256 remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. The pair
was copied to `ZEALPI` and byte-for-byte verified; the replaced pair is backed
up under `ZEALOS-BACKUP-20261023-INBOUND-DMA-SNAPSHOT/boot/`. The card was
safely ejected.

**Inbound-window physical result (2026-10-23):** photos `IMG_8915.heic` through
`IMG_8917.heic` confirm that image reached the Pi. BAR1 and BAR3 are disabled
(`0/0`); BAR2 is `0x11/0`, matching Linux's predicted 4 GiB inbound window at
PCI address zero for the 3 GiB identity DMA range. The FDT window is CPU/bus
`0/0`, size `0xc0000000`; the reserved arena is CPU/bus
`0x04000000/0x04000000`, and CPU/cache validation passes. PCIe error registers
remain clear. Thus the firmware's inbound translation is consistent with the
FDT, closing that suspect. However, the event ring remains empty and the No-op
still times out. The same report shows `MISC_CTRL=0x88003000`:
`SCB_ACCESS_EN` and `CFG_READ_UR_MODE` are enabled, and the burst field matches
BCM2711's 128-byte setting, but `RCB_MPS_MODE` (`0x400`) and `RCB_64B_MODE`
(`0x80`) are clear. Raspberry Pi Linux's
[BCM2711 PCIe setup](https://github.com/raspberrypi/linux/blob/rpi-6.18.y/drivers/pci/controller/pcie-brcmstb.c#L1309-L1332)
sets both bits while initializing the root complex. This is a concrete
firmware-versus-Linux setup difference, not yet a confirmed DMA cause.

The next candidate applies only those two RCB bits around the bounded xHCI
probe. It saves `MISC_CTRL`, preserves all unrelated bits, verifies the new
value, runs the existing single No-op test, halts xHCI and clears BME, then
restores and verifies the exact original value. The report shows before,
test, restored, and state-mask values. The host test checks that the update
does not alter other bits. This isolates whether matching Linux's RCB
initialization changes event-ring or command DMA; it does not claim a fix. The
write now occurs only after the xHC has been freshly verified halted and the
DMA layout has passed its gate. `make check-pi4-platform`, `make pi-sd`, and
`git diff --check` pass. The final candidate kernel SHA-256 is
`78d2f559d577dfd028906cfb99dcbb9653a7eb5fdb417b5da8ff37955aa7a3e2`; booter
SHA-256 remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.

The corrected final `78d2f559…` pair is staged on ZEALPI and byte-for-byte
verified; the card was safely ejected. The intermediate `0a1d20f1…` pair was
preserved at `ZEALOS-BACKUP-20261023-BCM2711-RCB-EARLY-WRITE/boot/`, and the
last physically tested pair remains at
`ZEALOS-BACKUP-20261023-BCM2711-RCB-MODES/boot/`. Firmware, DTBs, Limine,
configuration, and other boot files were left untouched. This candidate has
not yet been physically tested. Its expected diagnostic values are
`MISC_CTRL` before/test/restored of
`0x88003000/0x88003480/0x88003000`, state `0x1f`, followed by the event-ring
wrap and No-op results.

**BCM2711 RCB-mode physical result and next test (2026-10-23):** photos
`IMG_8918.heic` through `IMG_8920.heic` show the `78d2f559…` candidate. FDT
DMA remains CPU/bus `0/0` for `0xc0000000`; the reserved arena and CPU cache
check pass at `0x04000000/0x04000000`. The seven setup readbacks passed.
`MISC_CTRL` changed from `0x88003000` to `0x88003480` for the test, then was
restored exactly to `0x88003000` (state `0x1f`). The event ring still received
zero events: MFINDEX advanced through wrap (`0x1f40` to `0x018f`) with no Wrap
Event, the powered connected port reset changed PORTSC from `0x400202e1` to
`0x40200e03` without a Port Status Change Event, and the single No-op timed
out with USBSTS PCD (`0x10`) and CRCR running. Thus the RCB mode change is
verified and restored, but it did not fix xHCI DMA.

The next isolated experiment tests the alternate BCM2711 inbound mapping used
by the [Spectrum4 bare-metal Pi xHCI implementation](https://github.com/spectrum4/spectrum4/blob/main/src/spectrum4/kernel/pcie.s#L2552-L2588):
it configures BAR2 low/high as `0x11/0x4` (a 4-GiB window beginning at PCIe
address 16 GiB) and uses the same high address bits for xHCI DMA pointers
(including DCBAAP and command-ring addresses; see
[its xHCI initialization](https://github.com/spectrum4/spectrum4/blob/main/src/spectrum4/kernel/pcie.s#L3535-L3556)).
This differs from the Linux/FDT identity DMA range measured on this Pi and is
only a controlled hypothesis, not a correction to the FDT. The new probe first
requires the exact observed baseline `BAR2 lo/hi=0x11/0`, an xHC advertising
AC64, the already-passing FDT arena validation, and the existing halted/BME-off
gates. It then shifts all device-visible addresses by `0x0000000400000000`,
keeps CPU memory at `0x04000000`, writes and verifies only BAR2 high=`4`, and
runs the same single connected-port reset, MFINDEX wrap and No-op checks. On
completion it halts xHCI, clears BME, restores BAR2 high to its captured value,
and verifies the restore. If the halt or BME-off state cannot be verified, it
leaves the translation untouched and reports a restore failure. No RCB bits
are changed in this candidate. Host tests
cover the exact baseline gate, AC64 requirement, overflow handling and alias
math. `make check-pi4-platform`, `make pi-sd`, and `git diff --check` pass.
Candidate kernel SHA-256 is
`461f851e2037d6e657c3365245c6b99c029a859a2a3565fcd5a4f8b3d116b7b9`; booter
SHA-256 remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
The pair is staged on ZEALPI and byte-for-byte verified; the card was safely
ejected. The replaced `78d2f559…` pair is preserved under
`ZEALOS-BACKUP-20261023-16G-DMA-ALIAS/boot/`. Firmware, DTBs, Limine,
configuration, and the other boot files were left untouched.

**16-GiB DMA-alias physical result (2026-10-23):** photos `IMG_8921.heic`
through `IMG_8923.heic` confirm the candidate booted and passed FDT discovery,
arena validation, and CPU/cache validation. BAR2 reports low/original-high/
test-high/restored-high/state `0x11/0/4/0/0x1f`, and the bus alias bias is
`0x0000000400000000`; the setup register pointers carry high dword 4. The xHC
stopped at stage 6, before BME or RUN, with `setup mask=0x7f`, IMAN `0x2`,
IMOD `0x1`, and `xHCI DMA no-op=setup-readback-failed`, `events=0`. The
diagnostic had written zero to IMOD but then incorrectly required the entire
register to read as zero. Intel's xHCI specification defines IMOD as a 16-bit
IMODI interval plus an upper 16-bit IMODC counter; IMODI 0 disables moderation,
and 1 is a 250 ns interval, while IMODC is a separate counter
([register definition](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf)).
The observed low value `1` therefore trips an over-strict pre-BME software
check; this run does **not** test event-ring or command DMA through the alias.
BAR2 restoration was verified, and the controller remained stopped.

The next image keeps the same reversible BAR2 alias experiment and relaxes
only the IMOD readback check: accept IMODI 0 or 1, ignore the upper IMODC
counter, and continue to require all other setup fields to match. This allows
the probe to reach its already-bounded connected-port reset, MFINDEX-wrap, and
single No-op stages without changing the hardware experiment. Host tests cover
both supported low IMODI values, a nonzero IMODC, and rejection of interval 2.
The correction is built and staged: `make check-pi4-platform`, `make pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass. Kernel SHA-256 is
`97a5d6befe3cd0c3ac165c9b30564186c4b2ea6fb048e0d4dc1b74699c2f8ed6`; the
booter remains `0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
The replaced `461f851e…` kernel and unchanged booter are backed up at
`ZEALOS-BACKUP-20261023-IMOD-ALIAS-RETRY/boot/`. Both new boot files were
byte-for-byte verified on ZEALPI, then the microSD was safely ejected. Only
`boot/kernel.elf` and `boot/zealbooter.elf` were replaced; firmware, DTBs,
Limine, configuration, and other boot files were untouched. This image is
ready for another physical Pi run. The following result supersedes the
expectation that it would pass the pre-BME setup check.

**Pre-BME setup-readback result (2026-10-23):** photos `IMG_8925.heic`
through `IMG_8927.heic` show the `97a5d6be…` image booted successfully. FDT
discovery, the identity-mapped arena (`CPU/bus=0x04000000`), CPU/cache
validation, BAR2 alias programming, and restoration all pass. The controller
stops safely at stage 6 before BME or RUN. All seven setup reads completed;
IMAN is `0x2`, IMODI is `0`, CONFIG and ERSTSZ are `1`, but the combined setup
readback check fails. This run therefore still does not test xHCI event-ring or
command DMA through the alias.

The next image adds a setup-mismatch bitmask while preserving the same
pre-BME stop on any mismatch. Bits are: `0x01` incomplete read mask, `0x02`
IMAN, `0x04` IMODI, `0x08` CONFIG, `0x10` DCBAAP, `0x20` ERSTSZ, `0x40`
ERSTBA, and `0x80` ERDP. It is staged with kernel SHA-256
`3307c27e20ef3707fed626651232a15e436c7d95dfaafd34d6847720ca34eda2` and
unchanged booter SHA-256
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
The replaced pair was byte-for-byte backed up at
`ZEALOS-BACKUP-20261023-SETUP-MISMATCH/boot/`; the new kernel and booter were
byte-for-byte verified on ZEALPI, then the card was safely ejected. Only
`boot/kernel.elf` and `boot/zealbooter.elf` were replaced. Firmware, DTBs,
Limine, configuration, and other boot files remain untouched. On the next
boot, capture `xHCI DMA no-op`, `xHCI setup mismatch`, and the three setup
pointer lines so the failing address can be isolated before changing DMA
behavior.

**64-bit setup-pointer readback (2026-10-23):** photos `IMG_8928.heic` through
`IMG_8930.heic` show the `3307c27e…` kernel booted and all seven setup reads
completed, but the pre-BME guard stopped at stage 6 with mismatch mask
`0xd0`: DCBAAP, ERSTBA, and ERDP. The arena was identity mapped at
`0x04000000`, with the experimental PCIe bus bias `0x0000000400000000`.
Readbacks were DCBAAP `0x0000000400000000`, ERSTBA `0x0000000000424000`, and
ERDP `0x0000000000422000`; they do not match the aliased addresses expected
from the validated arena. BME and RUN remained off, the controller remained
stopped, and BAR2 was restored.

The next diagnostic changes only xHCI pointer-register access width: AC64
registers are now written as two ordered Dword MMIO accesses (low first, high
second), each followed by a DSB. The xHCI 1.2 specification allows this when
the platform cannot issue Qword accesses, and the local emu68 xHCI driver uses
the same low/high sequence. This is a focused compatibility test; matching
readbacks must still pass before BME or RUN can be enabled. `make
check-pi4-platform`, `make pi-sd`, and `git diff --check` pass. Kernel SHA-256
is `8838b43e6b3fb2e81bfa98614170376a6ba09840658203294e7511df20fd6836`; the
booter remains `0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
The replaced `3307c27e…` kernel and booter are backed up at
`ZEALOS-BACKUP-20261023-XHCI-DWORD-PAIR/boot/`. The new files were byte-for-byte
verified on ZEALPI and the card safely ejected. Only the two boot ELFs were
replaced; firmware, DTBs, Limine, configuration, and other boot files remain
untouched. The next physical run should report `xHCI setup mismatch=0` and
advance past stage 6 before any DMA is attempted.

**Aliased setup pointers and basic xHCI DMA accepted (physical 2026-10-23):**
photos `IMG_8931.heic` through `IMG_8933.heic` show the `8838b43e…` image
passed the pre-BME setup guard with mismatch `0`, alias bias
`0x0000000400000000`, and BAR2 `0x11/0/4/0/0x1f`. The FDT DMA arena is
identity mapped at CPU/bus `0x00400000`. The controller reached the bounded
runtime probe: the connected-port reset event passed, MFINDEX wrapped and
produced its event, and the No-op Command completed. The report shows four
event TRBs and no setup or DMA error. The xHC halted and BAR2 was restored at
the end of the probe. This physically confirms event-ring DMA and one command
fetch/completion through the 16-GiB alias. It still does not enable a slot,
enumerate a USB device, or transfer USB data.

The next candidate makes one narrow command-ring progression check: seed two
No-op commands and require successful completion events that reference the
first and second TRBs in order. This should confirm the command dequeue pointer
advances while keeping device slots and USB transfers disabled. The ring's
wrap Link TRB remains seeded but is not deliberately reached in this two-command
trial. `make check-pi4-platform`, `make pi-sd`, `bash -n
scripts/pi-sd-stage.sh`, and `git diff --check` pass. Kernel SHA-256 is
`4a5652d4488cd5689b549943aadadf043c56c305d853aa1a58119cb6269cf5a4`; booter
SHA-256 remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. This
pair is staged on ZEALPI and byte-for-byte verified; the replaced
`8838b43e…` kernel and unchanged booter are backed up at
`ZEALOS-BACKUP-20261023-XHCI-TWO-NOOPS/boot/`. The card was safely ejected.
Only `boot/kernel.elf` and `boot/zealbooter.elf` were replaced; firmware,
DTBs, Limine, configuration, and other boot files remain untouched.

**Sequential command-ring DMA accepted (physical 2026-10-23):** photos
`IMG_8934.heic` through `IMG_8936.heic` show the `4a5652d4…` image passed
setup readback (`mismatch=0`) and completed both No-op commands in order.
The report shows `noop-pair-pass`, `commands=2`, five event TRBs, a passing
connected-port reset event, and a passing MFINDEX-wrap event. The 16-GiB DMA
alias remains active for the bounded test (`BAR2=0x11/0/4/0/0x1f`) and is
restored before exit. This confirms command-ring dequeue advancement and
event-ring DMA on the Pi; no slot or USB transfer was used in this image.

The next image tests the smallest slot lifecycle: after the two No-ops, Enable
Slot must return a valid ID within the one-slot limit; the probe then publishes
a matching Disable Slot command and requires its completion with that same ID.
The slot is empty and released immediately. The probe does not address a USB
device, configure an endpoint, or transfer data. If an error interrupts the
cycle, the diagnostic records whether a temporary slot remains active, then
halts the controller. Host checks, Pi build, script syntax, and whitespace
checks pass. The staged kernel SHA-256 is
`f1cbcf611d19ce900a17247bb8daa87c5161e543eb9fbfd1f28c1c3e5788a579`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
The prior pair is backed up at
`ZEALOS-BACKUP-20261023-XHCI-SLOT-CYCLE/boot/`. Both staged files were
byte-for-byte verified, only `boot/kernel.elf` and `boot/zealbooter.elf` were
replaced, and the microSD was safely ejected. The empty-slot physical result is
recorded in the next milestone.

**Empty-slot cycle accepted; command-ring wrap staged (2026-10-23):** the
photos `IMG_8937.heic` through `IMG_8939.heic` showed
`xHCI DMA no-op=slot-cycle-pass`, `events=0x00000007`,
`commands=0x00000004`, `slot-id=0x00000001`, and `active=0`. Setup readback
reported no mismatch, the arena was CPU/bus `0x04000000`, and the BAR2 test
alias was restored after the port-reset and MFINDEX-wrap events passed. This
confirmed an empty slot can be enabled and disabled and released. Its captured
PORTSC speed-ID field was zero, so the result did not justify addressing a
device.

The next image started its command ring at TRB 252, completed three No-ops,
followed the Link TRB at 255 with Toggle Cycle set, then completed a fourth
No-op at TRB 0 using cycle state zero. The host suite
`make check-pi4-platform` and Pi build `make pi-sd` passed; shell syntax and
`git diff --check` passed. Its kernel SHA-256 was
`77f7e5d1fa6d6cbe6907460c0ffc92b36d775a63cf13f3dc92a5ffd6caff474d`; the
booter SHA-256 was
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. The
previous `f1cbcf61…` kernel and unchanged booter were backed up at
`ZEALOS-BACKUP-20261023-XHCI-CMD-WRAP/boot/`. Both files were verified
byte-for-byte on ZEALPI; only `boot/kernel.elf` and `boot/zealbooter.elf` were
replaced, and the card was safely ejected.

**Physical command-ring Link TRB wrap passed; post-reset port qualification
staged (2026-10-23):** photos `IMG_8941.heic` through `IMG_8943.heic` show
`xHCI DMA no-op=command-ring-wrap-pass`, `events=0x00000007`,
`commands=0x00000004`, `slot-id=0`, and `active=0`. All four No-op completion
events crossed the segment Link TRB and cycle toggle. The arena was CPU/bus
`0x04000000/0x04000000` (1 MiB); an earlier note incorrectly said
`0x00400000`. Setup readback had no mismatch and the reversible BAR2 alias was
restored. The port reset event passed, but post-reset PORTSC was
`0x40200003`: speed ID 0 and PP clear. The earlier pre-reset discovery had
reported port 1 at high-speed ID 3. The xHCI specification requires a valid
port-speed field after reset before constructing an addressed device context
([Intel xHCI specification, §4.3.1](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf)).
Therefore device addressing remains gated. The next candidate waits up to
100 ms after PRC and reports the settled PORTSC speed ID and readiness
(connected, powered, enabled, U0, and speed ID 1–4); it does not enable a slot,
address a device, or transfer USB data.

`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` passed. The new kernel SHA-256 is
`56527c88db320bf1152252f9a873702f15593a241944522982f40ed078caa86c`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. The
previous kernel and booter are backed up at
`ZEALOS-BACKUP-20261023-XHCI-PORT-QUALIFY/boot/`. Both new files were verified
byte-for-byte on ZEALPI, only `boot/kernel.elf` and `boot/zealbooter.elf` were
replaced, and the card was safely ejected. Boot this card and send the next
report photo. A `port-ready=yes` result with speed ID 1–4 would clear the
immediate reset/readiness gate; any other result points to power, enable, link
state, or speed-field handling that needs diagnosis before enumeration.

**Post-reset port readiness physically confirmed; Address Device candidate
prepared (2026-10-23):** photos `IMG_8944.heic` through `IMG_8946.heic` show
`xHCI DMA no-op=command-ring-wrap-pass`, `events=0x00000007`,
`commands=0x00000004`, and the 1 MiB DMA arena at CPU/bus
`0x04000000/0x04000000`. The port reset event and MFINDEX wrap passed. The
final post-reset sample reports speed ID 3 and readiness yes (connected,
powered, enabled, U0). This clears the gate for creating a device slot.

The next bounded test appends Enable Slot and Address Device after the four
No-ops. It verifies both completion events and the returned slot ID, publishes
an EP0-only input context using the observed high-speed setting and a 64-byte
default packet size, then verifies a nonzero device address and output slot
state 2 (Addressed). With BSR=0, Address Device asks the xHC to issue
SET_ADDRESS ([Intel xHCI specification, §4.3.3](https://www.intel.com/content/dam/www.public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf)).
The probe submits no descriptor, configuration, or HID transfer; it halts the
controller and restores PCI bus mastering off. After the device address has
been assigned, the attached device may retain it until the next port reset or
power cycle.

The xHCI host suite, `bash -n scripts/pi-sd-stage.sh`, `git diff --check`, and
`make pi-sd` pass. The output-context readback invalidates CPU cache lines
without cleaning stale data over device-written bytes. Candidate kernel
SHA-256 is `954ef3edd5f497b7b9985c7b221bb506567a3ac2317a1d35d37abbbae4a52190`;
booter SHA-256 remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. Both
files are staged and hash-verified on ZEALPI. The replaced pair is byte-for-byte
backed up at `ZEALOS-BACKUP-20261023-XHCI-ADDRESS-DEVICE/boot/`. Only
`boot/kernel.elf` and `boot/zealbooter.elf` were replaced. The card was synced,
force-unmounted after macOS `loginwindow` refused a normal unmount despite no
open file handles, and safely ejected. UEFI firmware, DTBs, Limine, config, and
other files were untouched.
Once the card is booted, capture the result, command completion status/control
fields, `USB-address/slot-state`, and the existing BAR2 restore and PCI command
fields. A successful result should report `address-device-pass`, six command
completions, slot 1 active, a nonzero USB address, and slot state 2. This is
the first device-addressing check; descriptor enumeration and HID input remain
unverified.

**Enable Slot protocol-slot-type follow-up (2026-10-24):** photos `IMG_8948.heic`
through `IMG_8950.heic` show the identity-mapped 1 MiB DMA arena at
`0x04000000/0x04000000`, a connected USB 2 root port ready at speed ID 3, and
all four No-op command completions across the Link TRB. The first Enable Slot
completion is `0x05000000/0x00008401`: completion code 5 (TRB Error), with no
slot assigned. The Address Device command was correctly skipped.

The previous command used Slot Type zero without resolving the connected port's
Supported Protocol capability. The xHCI specification requires Enable Slot to
use the 5-bit Protocol Slot Type declared by the Supported Protocol capability
that owns that root port; the zero value is valid only when the capability
declares the USB Protocol Device Slot type ([Intel xHCI 1.2b, §4.6.3 and
§7.2](https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf)). The
next candidate walks the bounded extended-capability chain from HCCPARAMS1.xECP,
matches the 1-based connected root port against Supported Protocol port ranges,
and writes the declared slot type into Enable Slot. It prints the matching
capability offset, protocol revision, slot type, and exact TRB control word.
Host tests cover protocol range selection, all five slot-type bits, cycle/type
encoding, invalid ranges, and a malformed capability link. `make
check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. The candidate kernel SHA-256 is
`79b48352ccb99326ad8c78a133c09d0239adf62d3ccbc7a37d09f9aab91ba596`; the
booter remains `0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
Both were copied to ZEALPI and verified byte-for-byte. The previous pair is
backed up at `ZEALOS-BACKUP-20261024-XHCI-SLOT-TYPE/boot/`; only the two boot
ELFs were replaced. The card was synced and safely ejected. Physical acceptance
is pending; this patch does not assume the field fix alone resolves the TRB
Error.

**Enable Slot before-wrap isolation candidate (2026-10-24):** photos
`IMG_8951.heic` through `IMG_8953.heic` show the same identity-mapped DMA arena,
cache pass, port-reset event, active MFINDEX wrap, and four No-op completions
across the Link TRB. Enable Slot still returns `0x05000000/0x00008401`
(completion code 5, TRB Error) with no slot assigned, even though the submitted
TRB is type 9 with Slot Type 0 (`0x00002400`) and the supported-protocol fields
are reported. Address Device is skipped.

The next probe isolates whether Enable Slot fails because the command-ring
consumer has already crossed the Link TRB: it submits the protocol-correct
Enable Slot at command index 251 with the initial cycle state, then runs the
same four No-ops at indices 252–254 and 0 across the Link TRB. Address Device
remains after wrap at index 1 with cycle zero. The HDMI report now prints the
protocol capability fields, Enable Slot control, and completion words on
separate lines for easier capture. `make check-pi4-platform`, `make pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass. Candidate Pi
kernel SHA-256 is
`55b1ec8ea756ea76b7881dfffdbe58f03a7802cc1e1b173654654ee17f1449f8`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. The
candidate is staged on ZEALPI and hash-verified. The replaced pair is backed up
at `ZEALOS-BACKUP-20261024-XHCI-PREWRAP/boot/`; only `boot/kernel.elf` and
`boot/zealbooter.elf` were replaced. The card was synced and ejected. It needs a
physical Pi 4B run; no Enable Slot success is claimed yet.

**CRCR alignment correction (2026-10-24):** photos `IMG_8954.heic` through
`IMG_8956.heic` show that the index-251 Enable Slot attempt timed out without a
completion event: `xHCI DMA no-op=enable-slot-timeout`, `events=3`,
`commands=0`, `slot-id=0`, Enable Slot control `0x00002401`, and status/control
still `0xffffffff/0xffffffff`. The earlier TRB Error result did not repeat
because this candidate never reached Enable Slot completion.

The timeout exposed a test bug: CRCR was initialized to command index 251, but
`251 * 16` is not 64-byte aligned. CRCR's Command Ring Pointer field occupies
bits 63:6, so the ring must start at a 64-byte boundary ([Intel xHCI 1.2b,
§5.4.5](https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf)). The
new candidate starts Enable Slot at index 248 (`248 * 16` is aligned), then
issues seven No-ops at indices 249–254 and 0 across the existing Link TRB at
255. Address Device remains at index 1 with cycle zero. Host tests now verify
the aligned start and all seven No-op completion pointers. `make
check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. Candidate Pi kernel SHA-256 is
`e501af081944f8da2f9c85ef922c43afff3fd92b4a69d755773227a7497438fb`; booter
SHA-256 remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. Both
were copied to ZEALPI and hash-verified; the replaced pair is backed up under
`ZEALOS-BACKUP-20261024-XHCI-CRCR-ALIGN/boot/`. Only the two boot ELFs changed.
The card was synced and ejected. Physical verification is pending.

**Address Device context isolation candidate (2026-10-24):** photos
`IMG_8961.heic` through `IMG_8963.heic` show that the aligned command-ring fix
worked: Enable Slot completed successfully, all seven No-ops completed across
the Link TRB, and `commands=8`, `slot-id=1`, `active=1` were reported. Address
Device returned completion code 5 (`0x05000000`, TRB Error) with event control
`0x00008401`; the output context retained address/state `0xffffffff`, so no
Addressed state was confirmed.

The next candidate changes only the Address Device diagnostic to BSR=1, which
blocks USB `SET_ADDRESS` and should leave a successfully accepted slot in
Default state (address 0, slot state 1). This separates input-context and
command acceptance from the USB address transaction. The report also prints
the input, output, and EP0 ring bus addresses, `DCBAA[slot]`, key Input Control,
Slot, and EP0 dwords, and the Address Device control word. `tests/pi4_xhci_dma_test.c`
checks both BSR encodings and rejects invalid BSR values. `make
check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. Candidate kernel SHA-256 is
`98c4a9e920a96bf5b138d83521e01f0e25d90f379f110381b69b35a8752160d8`; booter
SHA-256 remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. The
replaced pair was backed up at
`ZEALOS-BACKUP-20261024-XHCI-ADDRESS-CONTEXT/boot/`; only
`boot/kernel.elf` and `boot/zealbooter.elf` were replaced. Both candidate files
were hash-verified on ZEALPI, synced, and the card was safely ejected.
Physical verification is pending; a BSR=1 pass will be followed by a separate
BSR=0 SET_ADDRESS test.

**BSR=1 still returned TRB Error; pre-Link isolation candidate prepared
(2026-10-24):** photos `IMG_8964.heic` through `IMG_8966.heic` show the
physical candidate reached Address Device after the already accepted Enable
Slot and command-ring wrap. It reports `commands=8`, slot 1 active, and
Address Device completion status/control `0x05000000/0x00008401` (completion
code 5, TRB Error). The output-context address/state remained
`0xffffffff/0xffffffff`. The captured Address Device TRB control is
`0x01002e00` (type 11, BSR=1, slot 1, cycle 0). Its context report shows input,
output, and EP0 ring DMA addresses `0x0000000400425000`,
`0x0000000400426000`, and `0x0000000400428000`; DCBAA[1] points to the output
context. The displayed input-context dwords are Add flags `0x3`, Slot dwords
`0x08300000/0x00010000`, and EP0 dwords
`0x00400026/0x00428001/0x00000004/0x00000008`. These values match the intended
high-speed, root-port-1, EP0-only context and show that the test reached the
command, but they do not establish why the VL805 rejected it.

The next candidate keeps those contexts and BSR=1 unchanged, but submits
Address Device immediately after Enable Slot at command-ring index 249 with
cycle 1, before the Link TRB. Enable Slot remains at aligned index 248. This
isolates whether the prior error depended on issuing Address Device after the
ring wrap. The previous physical candidate already accepted seven No-ops
across the Link TRB; this focused candidate does not repeat that test. A pass
should return completion code 1 and output address/state `0/1`, with two total
command completions. It still performs no descriptor or USB data transfer.

`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. `make pi-sd` runs `scripts/pi-sd-stage.sh`, which only
refreshes `build/pi-esp` and explicitly does not write an SD card; no card was
mounted or modified. The prepared payload kernel SHA-256 is
`4a86dd265a17c7677262ea8b04536e869cca761046040fe340be854633354771`; the
booter SHA-256 is
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. The
candidate remains in `build/pi-esp/` for review or later manual staging.

**Pre-Link Address Device candidate staged to ZEALPI (2026-10-24):** after
the card was made available, its prior `boot/kernel.elf` and
`boot/zealbooter.elf` were copied byte-for-byte to
`ZEALOS-BACKUP-20261024-XHCI-PRELINK-ADDRESS/boot/`. The backup kernel SHA-256
is `98c4a9e920a96bf5b138d83521e01f0e25d90f379f110381b69b35a8752160d8`; the
backup booter SHA-256 is
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. Only
the two boot ELFs were replaced. The staged kernel hash is
`4a86dd265a17c7677262ea8b04536e869cca761046040fe340be854633354771` and the
booter hash is
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`; both
match `build/pi-esp/boot/`. The card was synced and safely ejected. Physical
verification from photos `IMG_8970.heic` through `IMG_8972.heic` passed:
Enable Slot and pre-Link Address Device both returned completion code 1, with
two command completions, slot 1 active, and USB address/slot state `0/1`. This
confirms that BSR=1 Address Device succeeds at index 249/cycle 1 before the
Link TRB; the slot remains in Default state as expected, and no USB data
transfer occurred.

**BSR=0 Address Device physically confirmed (2026-10-24):** photos
`IMG_8973.heic` through `IMG_8975.heic` show `address-device-pass`: Enable Slot
and Address Device both completed with code 1, `commands=2`, `slot-id=1`,
`active=1`, and USB-address/slot-state `1/2`. This confirms the VL805 assigned
a device address after the accepted pre-Link Address Device command; it did
not yet test descriptor data DMA.

The next candidate retains the accepted pre-Link index 249, cycle 1, and
EP0-only input context, then submits a standard control GET_DESCRIPTOR(Device)
request for the first eight bytes. Its Setup, IN Data, and OUT Status Stage
TRBs occupy the EP0 ring; the Status Stage carries IOC. The test requires a
Transfer Event pointing to that Status Stage with completion code 1 and zero
residual, then checks descriptor bytes `12 01 00 00 00 00 00 40` (the first
word prints as `0x00000112`, the second as `0x40000000`). This verifies actual
device-to-memory DMA for an EP0 control Data Stage, without configuration or
HID enumeration. Failure or timeout still halts xHCI and restores PCI bus
mastering off. `make
check-pi4-platform`, `make pi-sd`, shell syntax validation, and `git diff
--check` pass. The card's previous pair was backed up byte-for-byte under
`ZEALOS-BACKUP-20261024-XHCI-ADDRESS-DEFAULT/boot/`; those hashes were kernel
`4a86dd265a17c7677262ea8b04536e869cca761046040fe340be854633354771` and booter
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. The new
staged kernel SHA-256 is
`672ce98dd249a1897c0af8c8fa114ee84faa6fca4dbd1c7c1b286935fc9ecc08`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. Both
staged files match `build/pi-esp/boot/`; the card was synced and safely
ejected before the Pi run.

**EP0 GET_DESCRIPTOR candidate staged to ZEALPI (2026-10-24):** after photos
`IMG_8973.heic` through `IMG_8975.heic` confirmed BSR=0 Address Device and
USB-address/slot-state `1/2`, the next candidate added one EP0
GET_DESCRIPTOR(Device) control transfer for the first eight descriptor bytes.
The former card pair was backed up and verified at
`ZEALOS-BACKUP-20261024-XHCI-EP0-GET-DESCRIPTOR/boot/`: kernel SHA-256
`672ce98dd249a1897c0af8c8fa114ee84faa6fca4dbd1c7c1b286935fc9ecc08`, booter
SHA-256 `0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
The staged candidate kernel SHA-256 is
`9a5b216a0473ebec404e0449247af8443e7b0550d47ee704e9461adb1207d001`; the
booter remains `0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
Both card files match `build/pi-esp/boot/` byte-for-byte. The card was synced
and safely ejected. Boot it with the USB keyboard attached and send a clear
photo of the report. The expected pass marker is
`device-descriptor-prefix-pass`, with Transfer Event completion code 1,
residual 0, and descriptor first8 `0x00000112/0x40000000`. This is still a
single control descriptor transfer, not full USB enumeration or HID input.

**EP0 descriptor result and event-trace candidate (2026-10-24):** the user's
photos `IMG_8976.heic` through `IMG_8978.heic` show that the candidate still
reaches Address Device successfully, assigning USB address 1 and slot state 2,
but reports `control-transfer-failed`; the descriptor pass marker is absent.
The event fields in the photos are not sharp enough to transcribe reliably.
The prior code stopped on the first non-Port-Status event and only copied the
control buffer after all event checks passed, so the reported `0xffffffff`
prefix did not establish whether the device data reached memory.

The next diagnostic keeps the same bounded eight-byte GET_DESCRIPTOR request.
It retires and records non-Transfer events while waiting for a type-32 Transfer
Event, prints a check mask for event type/endpoint/slot/TRB pointer/success, and
captures the descriptor prefix after xHCI has halted even when event validation
fails. A valid prefix with an event mismatch would isolate the remaining bug to
event interpretation; an unchanged sentinel would show that inbound descriptor
DMA is still unconfirmed. It does not claim full USB enumeration or HID input.
`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. Before staging, the previous card pair was backed up
and hash-verified at
`ZEALOS-BACKUP-20261024-XHCI-EP0-EVENT-TRACE/boot/`: kernel SHA-256
`9a5b216a0473ebec404e0449247af8443e7b0550d47ee704e9461adb1207d001`, booter
SHA-256 `0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
Only `boot/kernel.elf` and `boot/zealbooter.elf` were replaced. The staged
kernel SHA-256 is
`d20c44a4a72e6cb36cf33839558aee26aea506af932fcb242bed04623d41f98c`; the
booter remains `0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
Both staged files match `build/pi-esp/boot/` byte-for-byte. The card was synced
and safely ejected as `/dev/disk19`.

**Physical xHCI inbound DMA confirmed (2026-10-24):** photos `IMG_8979.heic`
through `IMG_8981.heic` show the event-trace image reached
`device-descriptor-prefix-pass`. The Transfer Event validation mask is `0x1f`
(type, endpoint, slot, TRB pointer, and success all matched), with 29 unrelated
events retired first; the first skipped event is a Command Completion Event.
The report shows 35 total events, two command completions, slot 1 active, and
USB address/slot state `1/2`. The first eight descriptor bytes are
`12 01 10 02 09 00 01 40`: a USB 2.1 hub-class device (class 09, subclass 00,
protocol 01) with 64-byte EP0 packets. Thus the physical controller completed
an EP0 IN transfer and wrote device descriptor data into the DMA buffer. This
is the first confirmed device-to-memory xHCI DMA transfer; it does not yet
prove a complete descriptor read, configuration, downstream hub-port
enumeration, or HID input.

The next bounded candidate reads the full 18-byte Device descriptor and then
the 9-byte Configuration descriptor header through consecutive EP0 control
transfers, each with a distinct DMA buffer and a validated Transfer Event. It
does not issue `SET_CONFIGURATION`, power a hub port, or submit HID traffic.
`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. Before staging, the tested event-trace pair was
backed up and hash-verified under
`ZEALOS-BACKUP-20261024-XHCI-DEVICE-CONFIG-DESCRIPTORS/boot/`: kernel
`d20c44a4a72e6cb36cf33839558aee26aea506af932fcb242bed04623d41f98c`, booter
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. The new
card kernel is `91e21943db33f8312d9f4a4570f9df66669e569f17f6421ac2c469275792fe45`;
the booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. Both
files were byte-for-byte verified against `build/pi-esp/boot/`, synced, and
the card was safely ejected as `/dev/disk19`. UEFI firmware, DTBs, Limine, and
configuration were left unchanged. Boot this card on the Pi and capture the
full descriptor fields plus configuration header words; the expected marker is
`config-header-pass`.

**Full Device and Configuration header physically confirmed (2026-10-24):**
the user's photos `IMG_8982.heic` through `IMG_8984.heic` show the
`91e21943…` candidate reaching `config-header-pass`. The 18-byte Device
descriptor DMA returned USB 2.1 hub class `09/00/01`, EP0 MPS 64, VID/PID
`0424:2514`, one configuration. The Configuration header transfer also
completed with its validated event; it reports `wTotalLength=0x0197` (407),
one interface, configuration value 1, attributes `0xe0`, and max power
`0x32` (100 mA). The event trace reports 44 total events, two command
completions, slot 1 active, and USB address/slot state `1/2`. This confirms
the full device descriptor and configuration-header data reached their DMA
buffers. The photos do not yet show the next hub-descriptor request.

The next bounded candidate sends `SET_CONFIGURATION(1)` with the required
IN-direction Status Stage for a no-data control transfer, then requests and
validates the 9-byte class Hub descriptor. Host tests cover both the setup
packet and Status Stage direction. It does not power any downstream hub port,
reset a child device, or submit HID traffic. `make check-pi4-platform`,
`make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass.
Before staging, the active card kernel and booter were verified as
`91e21943db33f8312d9f4a4570f9df66669e569f17f6421ac2c469275792fe45` and
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39` and
backed up byte-for-byte under
`ZEALOS-BACKUP-20261024-XHCI-HUB-DESCRIPTOR/boot/`. The new candidate kernel
SHA-256 is
`e34f59aa7dc2efd68b85097d6ef228940f561af2ba6eac3a79614d2040260abd`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. Both
were verified against `build/pi-esp/boot/`, synced, and safely ejected as
`/dev/disk19`. UEFI firmware, DTBs, Limine, and configuration were left
unchanged. Boot this card on the Pi and capture the Set Configuration event,
Hub descriptor words, and expected `hub-descriptor-pass` marker; expected hub
port count is 4 for the attached `0424:2514` hub.

**Hub descriptor physically confirmed; hub-port probe staged (2026-10-24):**
photos `IMG_8985.heic` through `IMG_8987.heic` show
`hub-descriptor-pass`, `events=0x38`, two command completions, slot 1 active,
and USB-address/slot-state `1/2`. The no-data `SET_CONFIGURATION` transfer
passed all five event checks; its Status Stage control dword has DIR=IN. The
Hub descriptor reports four ports, characteristics `0x0009` (individual port
power switching), `bPwrOn2PwrGood=0x0a`, hub current `0x32`, removable bitmap
`0x04`, and power-control mask `0xff`. This confirms the descriptor reached
the DMA buffer and gives the next bounded port-power test its timing and
switching mode.

The next candidate issues class `SET_FEATURE(PORT_POWER)` to each of the four
ports, waits the descriptor-specified 20 ms power-good interval, then performs
class `GET_STATUS` on each port and validates the returned power bit. It records
all four status/change pairs, does not reset any downstream device, and sends
no HID transfers. `make check-pi4-platform`, `make pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass. Before staging,
the active kernel and booter hashes were verified as
`e34f59aa7dc2efd68b85097d6ef228940f561af2ba6eac3a79614d2040260abd` and
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, then
backed up byte-for-byte under
`ZEALOS-BACKUP-20261024-XHCI-HUB-PORT-STATUS/boot/`. The new kernel is
`16d16d1b26544b91400b0e985dbb4b0708836377607cd8719a0ad08dd8550aad`; the
booter remains `0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`.
Both card files were verified against `build/pi-esp/boot/` and synced. macOS
initially refused to eject `/dev/disk19` because `loginwindow` dissented; after
the user unlocked the Mac and ejected it, the volume disappeared from the host
mount list. UEFI firmware, DTBs, Limine, and configuration are unchanged.

**Physical hub-port probe result (2026-10-24):** photos `IMG_8988.heic` through
`IMG_8990.heic` show `control-transfer-failed`, `events=0x3d`, two command
completions, and slot 1 active. The run reaches the hub descriptor report but
does not show `hub-port-power-pass`. The event count is five above the prior
hub-descriptor-only run (`0x38`), consistent with the four port-power control
transfers and the first port-status request; this points to a failure during
the first GET_STATUS transaction, but the exact completion code is not legible
in these photos. The next diagnostic candidate prints the active phase/port,
USB setup request, expected Status TRB pointer, and completed power/status
masks beside the summary line. It labels phase 1 as PORT_POWER and phase 2 as
GET_STATUS. The previous card pair was backed up and verified under
`ZEALOS-BACKUP-20261024-XHCI-HUB-PORT-STATUS-DIAGNOSTIC/boot/`. The new kernel
SHA-256 is
`95d4f0ac05cac026cc4d8a9a3d6fb8fd3db4b3c026066e4026e8cac623a5a914`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`. Both
staged files match `build/pi-esp/boot/` byte-for-byte. The card was synced and
successfully ejected as `/dev/disk19`; it is ready for another Pi boot.

**Hub descriptor diagnostic result (2026-10-24):** photos `IMG_8991.heic`
through `IMG_8993.heic` show the `95d4f0ac…` image reaching stage 17 and
reporting `control-transfer-failed`, `events=0x5c`, two command completions,
and slot 1 active. The hub phase/port and request fields remain at their
sentinel values, so the port-power and GET_STATUS loop was not entered. The
photos do not expose a reliable Hub-descriptor Transfer Event result or raw
descriptor contents; this run therefore does not establish whether the
failure was event validation, returned bytes, or descriptor validation. The
next diagnostic records the Hub GET_DESCRIPTOR setup packet, data and Status
TRB controls, DMA address/length, transfer return code, validation result, and
raw descriptor words in the final report, including on transfer failure. It
leaves the USB control request and downstream port behavior unchanged.

`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass for the descriptor-trace candidate. Before staging,
the `95d4f0ac…` kernel, `0b62747a…` booter, and `e37c1b53…` demo were backed
up and verified under
`ZEALOS-BACKUP-20261024-XHCI-HUB-DESC-TRACE/boot/`. The new kernel SHA-256 is
`923dd2ad5964b3aba7bf25d36939ea41b4783b7497e4e129632c556c42f79fe1`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and the
demo remains
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`.
All three card files match `build/pi-esp/boot/` byte-for-byte after sync. Only
the `boot/` payload changed; firmware, DTBs, Limine, and configuration remain
untouched. The card synced and was successfully ejected as `/dev/disk19`. It is
ready for a Pi boot and a clear photo of the Hub GET_DESCRIPTOR report line
and buffer words.

**Ganged-hub power probe staged (2026-10-24):** photos `IMG_8994.heic`
through `IMG_8996.heic` show the `923dd2ad…` candidate completing the hub
`GET_DESCRIPTOR` Transfer Event (`rc=1`) with `events=0x63`, two command
completions, and slot 1 active. The setup packet requests the 9-byte Hub
descriptor (`bmRequestType=0xA0`, `GET_DESCRIPTOR`, type `0x29`). The returned
descriptor words decode to bytes `09 29 04 00 00 32 64 00 ff`: four ports,
`wHubCharacteristics=0`, ganged power switching, a 100 ms power-good delay,
100 mA hub current, and the removable/power masks. The prior probe accepted
only individual switching (`wHubCharacteristics & 3 == 1`), so it stopped
before sending PORT_POWER or reading port status. The new probe supports the
USB 2.0 ganged mode with one PORT_POWER request (to port 1) and individual mode
with one request per port, then waits the descriptor delay and checks all four
port status/change pairs. No child-port reset or HID traffic is added.

`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. Before staging, the previous kernel, booter, and demo
were verified and backed up under
`ZEALOS-BACKUP-20261024-XHCI-HUB-GANGED-POWER/boot/`. The new kernel SHA-256 is
`19cd4a9b9400eedffa4d37a43d4e7a141ef735b00250ce6ffaca3cfda87ac609`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and the
demo remains
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`.
All three staged files match `build/pi-esp/boot/` byte-for-byte after sync.
Only the `boot/` payload changed; firmware, DTBs, Limine, and configuration
remain untouched. The card is ready for a Pi boot and a photo showing the
`hub-port-power-pass` result, four port status/change pairs, and the summary
line.

**Ganged-hub probe result (2026-10-24):** photos `IMG_8997.heic` through
`IMG_8999.heic` show the `19cd4a9b…` candidate still in phase 1, PORT_POWER
for hub port 1, with request type/request `0x23/0x03`, value/index/length
`8/1/0`, and no power/status completion bits recorded. The saved ERDP is
`0x00000004:0x00422ff0`, at the final TRB of the 256-entry event-ring segment;
the current Transfer Event fields remain empty after skipped unrelated events.
The photo therefore does not prove the hub accepted or rejected PORT_POWER.
Reviewing the consumer showed it advanced past entry 255 without wrapping its
consumer cycle, and the MFINDEX wait path did not count the Port Status Change
Events it consumed. The next candidate tracks the Event Ring Consumer Cycle
State, wraps ERDP at the segment boundary, and counts every consumed event. It
will reveal whether the hub request itself completes once the event ring keeps
servicing DMA events.

**Cycle-aware event-ring consumer staged (2026-10-24):** all event polling
paths now share an index/cycle cursor. At entry 255 the cursor wraps to entry 0
and toggles CCS; event readiness compares the TRB Cycle bit against that state,
and ERDP is advanced to the wrapped address. The MFINDEX wait counts consumed
Port Status Change Events. A host test exercises the final entry, wrap, cycle
toggle, stale-entry rejection, and new-cycle acceptance. `make
check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. The previous card payload was verified and backed up
under `ZEALOS-BACKUP-20261024-XHCI-EVENT-RING-CYCLE/boot/`. The new kernel
SHA-256 is `b98c417a1b0d59c785dbe995c2061402bf92a6d936db60a4539b65c08b92d490`;
the booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and the
demo remains
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`. All
three staged files match `build/pi-esp/boot/` byte-for-byte after sync. Only
the `boot/` payload changed; firmware, DTBs, Limine, and configuration remain
untouched. Boot the card and capture the xHCI summary, event cursor index/cycle,
hub request result, and all four port status/change pairs.

**Cycle-aware event ring physically confirmed; Transfer Event validator fixed
(2026-10-24):** photos `IMG_9001.heic` through `IMG_9003.heic` show the
`b98c417a…` kernel consuming `0x107` events with cursor index/cycle `7/0`,
confirming that event delivery continued across a 256-entry wrap and the
consumer toggled its cycle state. Enable Slot, Address Device, the full device
and configuration reads, SET_CONFIGURATION, and the Hub descriptor all
completed. The hub's ganged PORT_POWER request (`bmRequestType/request`
`0x23/0x03`, `wValue/wIndex/wLength` `8/1/0`) also produced a Transfer Event
with completion code 1 and a parameter matching its expected Status TRB
(`0x00000004:0x004280c0`). The run was incorrectly reported as
`control-transfer-failed`: its Transfer Event validation mask was `0x1e`,
missing only the type bit because the event Cycle bit was zero after the ring
wrap. Event readiness already validates Cycle against the consumer cursor, so
the Transfer Event type check must not require Cycle=1. The validator now
checks the type independently of Cycle, and the host test accepts an otherwise
valid Transfer Event with Cycle=0. `make check-pi4-platform`, `make pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass. The current
card payload was verified and backed up under
`ZEALOS-BACKUP-20261024-XHCI-TRANSFER-CYCLE/boot/`; the new kernel SHA-256 is
`0996d56cfdff71795324557d449b58f231855d595e2cc718400940eebc21f1d4`, the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and the
demo remains
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`. The
three staged files match `build/pi-esp/boot/` byte-for-byte after sync; only
the `boot/` payload changed. The microSD was safely ejected. Boot this image
and capture `hub-port-power-pass`, power/status masks, and all four downstream
port status/change pairs.

**EP0 hub-status ring gap fixed and staged (2026-10-24):** photos
`IMG_9004.heic` through `IMG_9006.heic` show the previous `0996d56c…` image
passing the hub's ganged `PORT_POWER` request (`power/status masks=1/0`) and
then timing out on port 1 `GET_STATUS` (`bmRequestType/request=0xa3/0`,
`wValue/wIndex/wLength=0/1/4`). The event-ring cursor and DMA checks remained
healthy. The control TD layout explains the timeout: after the ganged power
Status TRB at EP0 ring entry 12, the controller dequeues entry 13, but the
probe had placed the first port-status Setup TRB at entry 19. Entries 13–18
were zero-cycle unused TRBs, so the xHC could not reach the request. The probe
now lays the four status TDs directly after however many power TDs were sent:
entry 13 for one ganged power request, entry 19 for four individually switched
ports. A host regression test covers both layouts and rejects invalid inputs.

`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. Before replacement, the prior kernel, booter, and demo
were verified and backed up under
`ZEALOS-BACKUP-20261024-XHCI-EP0-CONTIGUOUS/boot/`. The new kernel SHA-256 is
`e29c63d57d5f18d7ad9d31213707163dada3bfd9bb8043361a4605ffbf500470`; booter
SHA-256 remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and
demo SHA-256 remains
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`. All
three card files match `build/pi-esp/boot/` byte-for-byte after sync. Only the
`boot/` payload changed; firmware, DTBs, Limine, and configuration were left
untouched. This image is staged and awaits physical confirmation of
`hub-port-power-pass`, status mask `0xf`, and all four port status/change pairs.
The microSD was synced and safely ejected as `/dev/disk19` for the next Pi
4B boot.

**EP0 status-transfer progress and endpoint trace staged (2026-10-24):**
photos `IMG_9007.heic` through `IMG_9009.heic` show the contiguous-ring
candidate passing the ganged power request and completing port 1 and port 2
`GET_STATUS` (`power/status masks=1/3`), then timing out on port 3
(`phase/port=2/3`, request `0xa3/0`, index 3, length 4). This confirms the
ring-gap change moved the probe past the prior port-1 stall; the remaining
timeout is not yet explained. The next candidate makes no transfer-behavior
change. After xHC halt, it records the current port's Status TD index and data
buffer, the four received buffer words, and output EP0 Context DW0–DW4 so the
dequeue pointer and endpoint state can distinguish a fetch stall from a missing
Transfer Event or returned data.

`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. The previous kernel, booter, and demo were verified
and backed up under
`ZEALOS-BACKUP-20261024-XHCI-EP0-CONTEXT-TRACE/boot/`. The new kernel SHA-256
is `a4dfd7eea78d7ae7dc009c896c1d8796a531687efcdc6c78787305a5ceb54960`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and the
demo remains
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`. All
three card files match the build byte-for-byte after sync. Only the `boot/`
payload changed; firmware, DTBs, Limine, and configuration were left untouched.
The card awaits another Pi boot and is safely ejected as `/dev/disk19`.

**EP0 third-port timeout trace (2026-10-24):** photos `IMG_9010.heic` through
`IMG_9012.heic` show the `a4dfd7ee…` diagnostic candidate. Event delivery
continued (`0x109` events, consumer cursor `9/0`), the ganged power request
completed, and port 1 and port 2 `GET_STATUS` completed (`status events=3`;
both report `PORT_POWER=1`). Port 3's `GET_STATUS` then timed out. Its recorded
Setup/Data/Status controls are `0x00030851/0x00010c11/0x00001021`, and its
inbound data buffer remained zero after the controller halted. This confirms
the request reached the wait path but produced neither a Transfer Event nor a
data write; it does not yet distinguish endpoint dequeue state from a hub
response failure. The image also captured the raw output EP0 context, but its
dequeue pointer was not rendered separately from the context words. The next
candidate adds decoded EP0 endpoint state/dequeue/DCS and the expected bus
addresses of the current Setup, Data, and Status TRBs to make that comparison
unambiguous. No hub reset or HID traffic is added.

The follow-up now prints the decoded EP0 endpoint state, dequeue bus address,
DCS, and expected Setup/Data/Status TRB addresses beside the raw context. This
will show whether the xHC's dequeue pointer reached port 3's TD before the
timeout. `make check-pi4-platform`, `make pi-diag`, `make pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass. The prepared
local payload is `build/pi-esp/boot/`; kernel SHA-256
`d6be20e2521208906b2f76f96f1e1581fee84a3128ab10a9e2cb8f55526dfa62`, booter
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and demo
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`. The
microSD is not currently enumerated by macOS, so no physical card payload has
been changed.

**EP0 dequeue diagnostic result (2026-10-25):** photos `IMG_9017.heic` through
`IMG_9019.heic` show the `d6be20e2…` image running from `ZEALPI`. The xHC
delivered the earlier traffic (`events=0x109`, event cursor `9/0`, two command
completions, slot 1 active); the hub power request and port 1/2 status reads
completed (`power/status masks=1/3`), but downstream port 3's `GET_STATUS`
(`0xa3/0`, index 3, length 4) timed out with a zero data buffer. The current
port-3 TD is at Setup/Data/Status bus addresses
`0x400428130/0x400428140/0x400428150`, with its data buffer at
`0x40042a110`. The raw Output EP0 Context read only after global xHC halt
reported state 1 and dequeue `0x400428000`/DCS 1. That is not a reliable
committed dequeue snapshot: this path had not issued Stop Endpoint to make the
xHC save its final ring position into the Output Endpoint Context.

The new candidate adds a bounded Stop Endpoint command at command-ring index
250 after Address Device at index 249. On the port-3 timeout it captures any
resulting Transfer Event and the matching command completion, then snapshots
the EP0 state/dequeue/DCS and raw Output Context before global halt. This follows
the Stop Endpoint context-save procedure in the [xHCI 1.2b specification](https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf)
and should distinguish an endpoint-ring stall from a hub response failure.
It still adds no hub reset or HID traffic. `make check-pi4-platform`,
`make pi-diag`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass.

Before replacement, the previous `ZEALPI/boot/` kernel, booter, and demo were
copied to `ZEALOS-BACKUP-20261025-XHCI-EP0-STOP-TRACE/boot/` with a `SHA256SUMS`
manifest. The staged Stop Endpoint candidate has kernel SHA-256
`10eece997a950f7f569a24f9bfa0c4c5d9a4e1d9123c912444fca192654750e7`, booter
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and demo
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`. Each
card file was hash-verified against `build/pi-esp/boot/` after sync. Only those
three files under `ZEALPI/boot/` changed; firmware, DTBs, Limine, and config
were untouched. macOS safely ejected the card as `/dev/disk19`; the following
physical run is documented below.

**Hub port-status diagnostic result (2026-10-25):** photos `IMG_9020.heic` to
`IMG_9022.heic` show the staged Stop Endpoint image reached the hub
`GET_STATUS` phase for port 2. The event mask reached `0x3`, so status
Transfer Events were received for ports 1 and 2. The final state was
`hub-port-status-failed`; this is the hub-port POWER-bit check after a successful
port-2 transfer, not a control-transfer timeout. Therefore Stop Endpoint was
correctly not submitted, and the photos contain no `EP0 after Stop` snapshot.
The old probe exited at the first port whose returned status lacked the POWER
bit, leaving later port pairs unobserved. The next candidate keeps that
fail-on-unpowered result but continues the four bounded status requests, records
an explicit unpowered-port mask, and then reports all status/change pairs. A
true timeout still invokes the bounded Stop Endpoint trace. This isolates hub
power reporting from endpoint-stall behavior without resetting downstream
devices or adding HID traffic.

The port-collection candidate passes `make check-pi4-platform`, `make pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check`. It was staged to
`ZEALPI/boot/` after backing up the previous `10eece99…` kernel, booter, and
demo in `ZEALOS-BACKUP-20261025-HUB-PORT-COLLECT/boot/` with a SHA-256
manifest. The new kernel is `61d321726f1466b59cc0e8b8800584f407c9ca15ad38642c7092ab6792c48cd5`;
booter and demo hashes remain `0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`
and `e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`.
All three card files matched `build/pi-esp/boot/` after sync. Only those files
under `ZEALPI/boot/` changed; firmware, DTBs, Limine, and configuration remain
untouched. macOS safely ejected the card as `/dev/disk19`; it is ready for the
next Pi boot.

**Four-port status result (2026-10-25):** photos `IMG_9023.heic`,
`IMG_9024.heic`, and `IMG_9026.heic` show the `61d321726f…` candidate received
all four hub-port GET_STATUS Transfer Events (`status mask=0xf`) after the
ganged PORT_POWER request (`power mask=0x1`). The descriptor reports four
ports, ganged switching (`wHubCharacteristics=0`), a 200 ms power-good delay,
and a power-control mask of `0xff`. The status pairs show ports 1–3 with the
PORT_POWER bit set and port 4 clear (`unpowered mask=0x8`); the transfers
themselves completed. This extends xHCI DMA validation through all four
downstream control-IN data buffers. The remaining failure is the hub's
port-power report for port 4, not the DMA path or a timeout. USB 2.0 specifies
that a PORT_POWER request to any port in a gang powers that gang, so this
candidate repeats the idempotent request on port 4 as a diagnostic, then reads
all four status pairs again. For this hub the completed power-event mask should
be `0x9` (requests to ports 1 and 4) and the status-event mask `0xf`. It does
not clear power, reset a child, or submit HID traffic.

The ganged-port-4 retry candidate passes `make check-pi4-platform`, `make
pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and `git diff --check`. It retains
the ganged request on port 1 and adds a second PORT_POWER request on port 4;
status TDs move after both power requests. The current card payload was backed
up under `ZEALOS-BACKUP-20261025-HUB-PORT4-POWER-RETRY/boot/` with a SHA-256
manifest. The candidate kernel SHA-256 is
`03a338662527521f0038eb883841cd22a3e0942529666df907335dffbcd37e8c`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and the
demo remains
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`.
The card's prior kernel/booter/demo and SHA-256 manifest are in the named
backup. The staged kernel, booter, and demo hashes were verified against
`build/pi-esp/boot/`; only `ZEALPI/boot/kernel.elf` was replaced. `diskutil`
successfully ejected `/dev/disk19` after sync, so the card is ready for the
next Pi boot. Firmware, DTBs, Limine, and configuration were not changed.

**Four-port power retry result (2026-10-25):** photos `IMG_9027.heic` through
`IMG_9029.heic` show the `03a33866…` candidate completed both requested
PORT_POWER transfers and all four GET_STATUS transfers (`power/status masks
0x9/0xf`). The descriptor still reports four ports with ganged switching,
200 ms power-good delay, and power-control mask `0xff`. However, after requests
for ports 1 and 4, only those ports report PORT_POWER; ports 2 and 3 remain
clear (`unpowered mask=0x6`). This confirms the hub control-IN DMA path for all
four ports while showing that the hub's advertised ganged mode conflicts with
its per-port status behavior. The next candidate sends an idempotent
PORT_POWER request to each of the four ports, then reads all four status/change
pairs. This is a power-on-only diagnostic: it does not clear power, reset any
downstream device, or submit HID traffic. A `0xf` power-event mask and a
`0xf` status-event mask followed by a zero unpowered mask would confirm the
ports respond individually; a different mask will narrow the descriptor or
status inconsistency further.

**Four-port power sweep candidate staged (2026-10-25):** the diagnostic
issues PORT_POWER on ports 1–4 before reading all four status/change pairs.
It passes `make check-pi4-platform`, `make pi-sd`, `bash -n
scripts/pi-sd-stage.sh`, and `git diff --check`. Kernel SHA-256 is
`d2832a8bedd7c67a3f4e5756b3a3595b7530c20b6b2064826eedd40ff23d92d6`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and the
demo remains
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`. Before
replacement, the card's previous kernel, booter, and demo were saved under
`ZEALOS-BACKUP-20261025-HUB-PORT-POWER-SWEEP/boot/` with a `SHA256SUMS`
manifest. All three active card files were hash-checked; only
`ZEALPI/boot/kernel.elf` changed. The card was safely ejected as `/dev/disk19`.
For the next boot, capture the `power/status masks`, `unpowered mask`, and
four `status/change` pairs. A `0x0f/0x0f` event-mask pair with unpowered mask
`0x0` confirms the four ports report powered after the sweep.

**Four-port power sweep result (2026-10-25):** photos `IMG_9030.heic` through
`IMG_9032.heic` show the `d2832a8b…` candidate returned
`hub-port-power-pass`, with all power and status transfers completed
(`0x0f/0x0f`) and unpowered mask `0`. This accepts the hub's per-port power-on
and status-transfer path even though its descriptor says ganged switching.
The full status/change text extends to the right edge of the photograph, so it
does not yet provide a clear connected-child mask. The next candidate adds a
compact connected-port mask from each valid status word while retaining the
per-port power sweep. This will show whether a USB device is connected to any
downstream port before we add child reset/address/configuration traffic.

**Connected-port mask candidate staged (2026-10-25):** the kernel now reports
valid connected-port bits separately from power/status event masks, making it
clear whether a child is attached before any downstream reset or enumeration.
`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. Kernel SHA-256 is
`2f67cc6f86f18a4f245994b7cc89caf05db1c32605934d7f70713712f4b3806a`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and the
demo remains
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`. The
previous card kernel, booter, and demo are backed up under
`ZEALOS-BACKUP-20261025-HUB-CONNECTED-MASK/boot/` with a SHA-256 manifest.
All active card files were verified; only `ZEALPI/boot/kernel.elf` changed.
macOS safely ejected `/dev/disk19`. The next run should show power/status masks
`0xf/0xf`, powered/unpowered masks `0xf/0x0`, and a connected mask. If the
connected mask is zero, attach a USB keyboard or other test device to a
downstream hub port and rerun this same image before implementing child
enumeration.

**Partial-scan powered-mask fix staged (2026-10-25):** powered-mask reporting
now counts only ports with valid GET_STATUS events, so an incomplete status
scan cannot overstate which ports were powered. This follow-up passes
`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check`. Kernel SHA-256 is
`166ba54076d19170b560a4bdaec1b7e1f92fa48c1daddbd82b22fef979528455`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and the
demo remains
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`. The
previous connected-mask payload was backed up under
`ZEALOS-BACKUP-20261025-HUB-POWERED-MASK-FIX/boot/` with a verified SHA-256
manifest. Only `ZEALPI/boot/kernel.elf` was replaced; all active files were
hash-checked and macOS safely ejected `/dev/disk19`. Boot this card and capture
the power/status event masks, powered/unpowered masks, connected mask, and all
four port status/change pairs. If the connected mask is zero, attach a USB
keyboard or another test device to a downstream hub port and rerun before
starting child enumeration.

**Connected child identified on physical Pi (2026-10-25):** photos
`IMG_9033.heic` through `IMG_9035.heic` show `hub-port-power-pass`, power/status
event masks `0xf/0xf`, powered/unpowered masks `0xf/0x0`, and connected mask
`0x4`. Thus all four ports completed their scans and the attached child is on
downstream port 3. This is the first clear physical indication of which hub
port has a child; it does not yet mean the child has been reset or enumerated.

**Connected-child reset candidate staged (2026-10-25):** the Pi image
resets only the lowest downstream port whose GET_STATUS was received and says
both connected and powered. For this photo set that is port 3. It sends one
class `SET_FEATURE(PORT_RESET)`, waits 100 ms, verifies PORT_RESET cleared and
C_PORT_RESET set, clears only C_PORT_RESET, then reads final status and checks
the child remains connected, enabled, and powered. This briefly interrupts
that attached USB device. The diagnostic does not assign the child a slot or
USB address and sends no HID traffic. If the status scan is incomplete or the
port is not powered, it skips reset.

`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. The new kernel SHA-256 is
`3ccca6d83f7070af32efd30f0d6a92228d476b1aece61067bfee5cc5339c7467`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and the
demo remains
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`. The
previous current-card kernel, booter, and demo are backed up under
`ZEALOS-BACKUP-20261025-HUB-CHILD-RESET/boot/` with a SHA-256 manifest. All
three active card files match `build/pi-esp/boot/`; only the kernel changed.
The attempted normal eject of `/dev/disk19` was refused by `loginwindow`, so
the card remains mounted until the Mac is unlocked and safe eject is retried.
Boot this image and capture the `Hub child reset=` summary, port/samples,
request/clear results, reset status/change, and final status/change.

**Connected-child reset accepted; routed-address candidate built (2026-10-25):**
photos \`IMG_9036.heic\` through \`IMG_9038.heic\` show
\`hub-child-reset-pass\` on downstream port 3. The child stayed connected,
powered, and enabled; post-reset status/change was \`0x103/0x0\`, and the
reset-change bit was cleared. Its hub-port status has neither the Low-Speed nor
High-Speed indicator, so the child is Full-Speed (xHCI speed ID 1). The parent
is the High-Speed, four-port, single-TT hub (device protocol 1).

The next candidate preserves the hub's Addressed Slot Context while issuing
Configure Endpoint to set Hub, the four-port count, and descriptor TTT. It
raises xHC MaxSlots to two, enables a distinct child slot, publishes a separate
output context, and issues Address Device with route string 3, root port 1,
TT hub slot 1, and TT port 3. It stops after USB address assignment; child
descriptor reads and HID are not included. This step is built and locally
tested, but not yet physically accepted.

The candidate passes \`make check-pi4-platform\`, \`make pi-sd\`,
\`bash -n scripts/pi-sd-stage.sh\`, and \`git diff --check\`. Kernel SHA-256 is
\`5e5be06babb1d38ba663be30701ea860b49aa33b2ccb10b1eb622de382c85089\`; the
booter remains
\`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39\`, and the
demo remains
\`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0\`.
Physical acceptance should capture \`hub-child-address-pass\`, all five command
completions, Configure Endpoint output hub fields, child Enable Slot and
Address Device completion events, nonzero child USB address/slot state 2, and
the child input Slot Context showing speed 1 and TT parent fields \`1/3\`.
Regardless of the result, the diagnostic halts xHCI and restores PCI bus
mastering and BAR2 before the card can be removed.

The routed-address candidate is staged on ZEALPI at
\`/Volumes/ZEALPI/boot/kernel.elf\`; its SHA-256 is
\`5e5be06babb1d38ba663be30701ea860b49aa33b2ccb10b1eb622de382c85089\`. The
previous active kernel, booter, demo, and \`limine.conf\` were preserved under
\`ZEALOS-BACKUP-20261025-HUB-CHILD-ADDRESS/\` with a verified \`SHA256SUMS\`
manifest. All four active payload hashes match the build, and macOS safely
ejected \`/dev/disk19\`. Boot this card in the Pi 4B and send the HDMI report
showing the Configure Endpoint and child Address Device results.

## Code plan (small batches)

**Hub Configure Endpoint timeout observed; guarded retry prepared (2026-10-25):**
photos `IMG_9040.heic`–`IMG_9042.heic` show the `5e5be06b…` candidate reached
the end of the root-hub and downstream-port checks. The child reset on port 3
passed with final status/change `0x103/0x0`; the hub reports four powered ports
and the connected child remains Full-Speed. The run stopped at stage 21 with
`hub-configure-timeout`, event count `0x112`, event cursor `0x12`/cycle `0`, and
two completed commands (root Enable Slot and Address Device). Configure
Endpoint status/control stayed `0xffffffff/0xffffffff`, so no completion was
observed. HCRST completed and the safety cleanup restored the PCI command state;
the child was not addressed.

The next candidate places a No-op command at ring index 250 immediately after
the root Address Device command, then submits Configure Endpoint at index 251.
This checks that the controller consumes the next command-ring entry before the
hub update. It also records the exact input-context Add/Drop flags and Slot
Context words, Configure Endpoint TRB, CRCR, USBSTS, and command-doorbell
readback if the update times out. Child Enable Slot and Address Device move to
indices 252 and 253. Host tests cover the guard TRB and command positions.

The guarded diagnostic passes `make check-pi4-platform`, `make pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check`. Kernel SHA-256 is
`810e65eaddcedf6e56dae7cc8fb11070c5a5aa756520e5d31623e027fb9cc9ba`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, and the
demo remains
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`.
The replaced active kernel, booter, demo, and `limine.conf` are backed up under
`ZEALOS-BACKUP-20261025-HUB-CONFIG-TIMEOUT/` with a verified `SHA256SUMS`
manifest. All four active card files match `build/pi-esp/` byte-for-byte after
sync, and macOS safely ejected `/dev/disk19`.

**Post-address guard No-op timeout observed; lazy command publication prepared
(2026-10-25):** photos `IMG_9043.heic`–`IMG_9045.heic` show that the guard
No-op timed out before Configure Endpoint was submitted. The downstream port 3
reset still passed; root Enable Slot and Address Device completed, with slot 1
active, but the guard remained at `0xffffffff/0xffffffff`. The event count was
`0x112`, cursor `0x12`/cycle `0`, with two counted command completions. The
transfer poll also reported four unrelated events. The old initializer had
marked all later pre-Link entries as cycle-valid No-ops at startup. The xHC may
have consumed a future command entry before the probe rewrote it; this is the
leading explanation, not yet physically proven.

The next candidate leaves unused pre-Link command entries cycle-invalid for
the initial producer cycle and makes entry zero invalid after the Link TRB
toggles the cycle. Each future command is published only immediately before
its doorbell. Host tests now check those invalid initial entries. The
diagnostic still checks the post-Address-Device guard first, then Configure
Endpoint, child Enable Slot, and child Address Device. It does not repeat the
already accepted independent command-ring wrap test.

The lazy-publication candidate passes `make check-pi4-platform`, `make pi-sd`,
and `git diff --check`. Kernel SHA-256 is
`a69de06be2541ce4145bec638cef0a0ee2e09de1ba088b222cc37b131a892189`; the
booter remains `0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`,
and the demo remains
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`. This
candidate does not yet claim a guard, hub-context, or child-address pass.

The prior active kernel, booter, demo, and `limine.conf` were preserved under
`/Volumes/ZEALPI/ZEALOS-BACKUP-20261025-HUB-COMMAND-CYCLE-GUARD/`; its
`SHA256SUMS` manifest verified. All four active payload files match
`build/pi-esp/` byte-for-byte, and macOS safely ejected `/dev/disk19`. Boot this
card in the Pi 4B and photograph the guard completion, Configure Endpoint
status/control and input-context words, and the child Enable Slot/Address
Device results if the guard passes.

**Routed child Address Device physically accepted (2026-10-25):** photos
`IMG_9046.heic`–`IMG_9048.heic` show the `a69de06b…` lazy-publication image
reaching `hub-child-address-pass`. The guard No-op and hub Configure Endpoint
both report xHCI Success completion code 1; the event cursor is `0x19` at
cycle 1 and six command completions are counted. The child is Full-Speed
(speed ID 1), routed through hub port 3 (route string 3), and assigned slot 2;
Address Device reports Success and the output context reports USB address 2,
slot state 2. The card's active kernel hash matched the candidate when it was
reconnected. This physically accepts root-hub Configure Endpoint, the guarded
command-ring advance, and routed child address assignment. It does not yet
prove transfers through the child's EP0.

The next candidate requests only the first eight bytes of the child's Device
Descriptor on the independent child EP0 ring, using the Full-Speed default
EP0 MPS of 8. It validates descriptor length/type and the speed-appropriate
`bMaxPacketSize0`, and reports the returned words and Transfer Event details.
It does not read the rest of the descriptor, configure an interface, or send
HID traffic. This separates routed child control-transfer DMA from later EP0
MPS adjustment and device-class work.

**Child Device Descriptor prefix candidate staged (2026-10-25):** the probe
submits GET_DESCRIPTOR(Device) for eight bytes on the addressed child's
independent EP0 ring and 8-byte Full-Speed default MPS. It validates the
descriptor length/type and `bMaxPacketSize0` for the reported speed, and
records the transfer return code, status TRB pointer, Setup/Data/Status
controls, and returned words. Host coverage passes `make check-pi4-platform`;
`make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and `git diff --check` also
pass. Kernel SHA-256 is
`cb408976d803dbed2643712b5eb83d259fac92fc8a63d8833945a31d700a658c`; booter
SHA-256 is
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, demo is
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`, and
`limine.conf` is
`4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`. The
replaced active payload was backed up under
`ZEALOS-BACKUP-20261025-CHILD-DESCRIPTOR-PREFIX/` with a verified `SHA256SUMS`
manifest. All four active card files match `build/pi-esp/` byte-for-byte, and
macOS safely ejected the card. Boot this candidate and capture the new summary
state plus the child descriptor Transfer Event, validation, and returned
prefix words. Do not interpret a pass here as full enumeration or HID support.

**Child Device Descriptor prefix physically accepted (2026-10-25):** photos
`IMG_9049.heic`–`IMG_9051.heic` show
`child-device-descriptor-prefix-pass`, transfer return code 1, and validation
1. The returned words are `0x02100112/0x40010009`: an 18-byte Device
Descriptor, USB 2.10, hub class 9, and Full-Speed EP0 `bMaxPacketSize0=64`.
This proves the routed child's first control-IN data packet and descriptor DMA.
The next candidate copies the accepted child EP0 context, changes only its
Max Packet Size to 64, and submits Evaluate Context at the next pre-Link
command position. It then requests and validates the full 18-byte descriptor
on the next three live EP0 ring entries. It does not configure an interface or
send HID traffic.

**Child EP0 MPS evaluation/full descriptor candidate staged (2026-10-25):**
the candidate adds host coverage for the EP0-only Evaluate Context input and
command TRB, validates the output context MPS, then checks the full child
Device Descriptor. `make check-pi4-platform`, `make pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass. The staged
kernel SHA-256 is `2228c124afe0cbcff974dbff6dfb2a5a53e349fee7490f1702e8c3819855013e`;
the booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`,
`demo.hcbc` is
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`, and
`limine.conf` is
`4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`. The
previous active payload was preserved under
`ZEALOS-BACKUP-20261025-CHILD-DESCRIPTOR-EVALUATE/` with a verified manifest.
All active payload files match `build/pi-esp/` byte-for-byte after sync.
macOS safely ejected `/dev/disk19` before its physical run.

**Child EP0 Evaluate Context physical result (2026-10-25):** photos
`IMG_9053.heic`–`IMG_9055.heic` show the Evaluate Context command completed with
completion code 1. The descriptor's requested MPS is `0x40` (64), while the
probe reported output MPS `0` and stopped before the full descriptor request.
The command succeeded; the probe selected `child_output_context +
2*context_size`, which is output DCI 2, instead of EP0 at output DCI 1
(`child_output_context + context_size`). The earlier host fixture used the same
wrong offset, so it failed to expose the bug.

**Child EP0 Output Context DCI 1 correction staged (2026-10-25):** the probe
now derives EP0 from Output Context DCI 1 in one checked helper, preserves that
context when building Evaluate Context's Input Context DCI 1, and reads the
updated MPS from the same location. The host test puts a distinct decoy MPS in
output DCI 2 so it cannot mask a repeat of this indexing error. `make
check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and `git
diff --check` pass. The new kernel SHA-256 is
`4e01ae25cedd135cdf0ba4c804c07f0602ea0306407d57f2540f564559582f29`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`,
`demo.hcbc` is
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`, and
`limine.conf` is
`4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`. The
previous active payload was backed up under
`ZEALOS-BACKUP-20261025-CHILD-EP0-OUTPUT-INDEX/` with a verified manifest.
All four active payload files on ZEALPI match `build/pi-esp/` byte-for-byte.
macOS safely ejected `/dev/disk19` before its physical run; the result is
recorded below. A pass accepts EP0 MPS adjustment and full child descriptor
DMA only; interface configuration and HID remain unverified.

**Child EP0 MPS correction physical result (2026-10-25):** photos
`IMG_9057.heic`–`IMG_9059.heic` show Evaluate Context completed with status
`0x01000000`; requested and output MPS both read `0x40`. The following full
Device Descriptor transfer failed validation. Its Transfer Event points to
the expected Status Stage TRB and passes the type/endpoint/slot/pointer checks
(`checks=0x0f`), but the status word is `0x0001ffff`, so the success check
fails. The old failure path left all five descriptor-word fields at their
`0xffffffff` sentinel and did not expose whether data reached the buffer.

**Full child descriptor failure snapshot candidate staged (2026-10-25):** the
next build fills the 64-byte DMA buffer with an `0xa5` marker and, when a
matching but unsuccessful Transfer Event arrives, invalidates and reports the
five raw descriptor words before cleanup. The report now distinguishes
`observed` from `validated`; a successful event still reports and validates
the same 18-byte descriptor. `make check-pi4-platform pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass. Kernel SHA-256 is
`c233d5c92208a5a526e30d6d6556ee8e31afb48f78598f8d3bcbf909d803336a`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`,
`demo.hcbc` is
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`, and
`limine.conf` is
`4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`. The
previous active payload was backed up under
`ZEALOS-BACKUP-20261025-CHILD-FULL-DESCRIPTOR-DIAGNOSTIC/` with a verified
manifest. All four active card files match `build/pi-esp/` byte-for-byte, and
macOS safely ejected `/dev/disk19`. Boot this card and capture the Transfer
Event line plus the full descriptor `rc/valid/observed` and five-word report;
if the DMA buffer was untouched, its words should show the `0xa5` marker.

**Full child descriptor physical result (2026-10-25):** photos
`IMG_9062.heic`–`IMG_9064.heic` show Evaluate Context completed and the
requested/output EP0 MPS both equal `0x40`. The full descriptor request then
returned a structurally matching Transfer Event for its Status Stage TRB, but
the completion code is 6 (Stall Error). All five captured descriptor words
remain `0xa5a5a5a5`, confirming the child did not write descriptor data to the
DMA buffer. The setup/data/status controls and event checks remain consistent
with the submitted TD, so the failure is now isolated to the child's response
to the request or its EP0 state after Evaluate Context, rather than missing
event delivery or descriptor-buffer DMA.

**Post-Evaluate EP0 GET_STATUS diagnostic (2026-10-25):** the next candidate
submits the standard two-byte device GET_STATUS request immediately after the
successful MPS update and before retrying the full Device Descriptor. It saves
the Transfer Event status/check mask, raw two-byte response, DMA address, and
TRB controls independently. If GET_STATUS stalls or times out, the probe stops
before issuing another request; if it passes, the full descriptor retry shows
whether the failure is specific to GET_DESCRIPTOR. The two new EP0 TDs occupy
ring indexes 3–5 and 6–8 after the initial descriptor TD at 0–2.

**Post-Evaluate GET_STATUS candidate built (2026-10-25):** `make pi-sd`,
`make check-pi4-platform`, `bash -n scripts/pi-sd-stage.sh`, and `git diff
--check` pass. The Pi kernel SHA-256 is
`839eb8ad8a8398d39140dd3d262250eb9823005a8801f6ef7d124b1a21d1c430`; the
booter remains
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`,
`demo.hcbc` is
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`,
`limine.conf` is
`4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`, and
`BOOTAA64.EFI` is
`3f51768c5eb8a0818b8e7aa2ac9a9caef3c64707b5b6db15d1e612de23236b8c`. The
payload is prepared under `build/pi-esp/`; the staging script itself does not
write removable media.

**Post-Evaluate GET_STATUS candidate staged to ZEALPI (2026-10-25):** after
the card reconnected as `/dev/disk19s1`, its current kernel matched the prior
`c233d5c9…` descriptor-snapshot candidate and the other active payload hashes
matched the prepared build. I preserved the previous kernel, booter, demo, and
`limine.conf` under
`ZEALOS-BACKUP-20261025-CHILD-POST-EVALUATE-GET-STATUS/` with a verified
`SHA256SUMS` manifest, replaced only `boot/kernel.elf`, and byte-verified the
kernel, booter, demo, `limine.conf`, and `EFI/BOOT/BOOTAA64.EFI` against
`build/pi-esp/`. macOS safely ejected `/dev/disk19`. Boot this card and capture
the child EP0 GET_STATUS report plus the following full-descriptor result. A
GET_STATUS pass narrows the remaining Stall Error to the descriptor request;
a GET_STATUS failure localizes it to EP0 after Evaluate Context. Interface
configuration and HID input remain unverified.

**Post-Evaluate GET_STATUS physical result (2026-10-25):** photos
`IMG_9067.heic`–`IMG_9069.heic` show a matching Transfer Event for the expected
Status Stage TRB (`checks=0x0f`) with completion code 6 (Stall Error) and
residual `0x1ffff`. The 64-byte response buffer was initialized to `0xa5`; its
captured two-byte device status is `0x0000`, so the child wrote a valid-looking
GET_STATUS payload to DMA before the failed OUT Status Stage. The request
controls are `0x00030851/0x00010c11/0x00001021` (standard device IN, two-byte
Data Stage IN, Status Stage OUT). This narrows the failure to the end of the
control transfer after the MPS Evaluate Context command; the earlier full
descriptor also stalled without changing its buffer. The photos continue to
confirm the Pi FDT/DMA arena and preceding child-descriptor-prefix checks.

**Pre/post-Evaluate GET_STATUS comparison candidate (2026-10-25):** the next
diagnostic sends the same two-byte device GET_STATUS immediately after the
successful 8-byte descriptor prefix, before changing EP0 MPS, then sends it a
second time after Evaluate Context. The first request uses EP0 ring indexes
3–5, Evaluate Context follows, the second request uses indexes 6–8, and the
full descriptor retry moves to 9–11. Both snapshots retain the event parameter,
status/control words, structural check mask, response word, DMA/status bus
addresses, and setup/data/status controls. This directly tests whether the MPS
update is associated with the status-stage Stall Error.

**Pre/post-Evaluate GET_STATUS comparison candidate staged to ZEALPI
(2026-10-25):** `make check-pi4-platform pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass. The staged kernel
SHA-256 is `280cf424789050deb682a1c35c057c0b7ccb3122609198161d06ee179f616dfd`;
the booter, demo, Limine config, and EFI binary retain their verified hashes
from the previous candidate. Before replacing the active payload, all five
files were copied to
`ZEALOS-BACKUP-20261025-CHILD-PREPOST-GET-STATUS/` and checked against its
`SHA256SUMS`. All five staged files were then compared byte-for-byte with
`build/pi-esp/`, and macOS safely ejected `/dev/disk19`. Boot this card and
compare the pre-Evaluate and post-Evaluate GET_STATUS event/status and raw
response lines. Interface configuration and HID input remain future work.

**Pre/post-Evaluate GET_STATUS physical result (2026-10-25):** photos
`IMG_9072.heic`–`IMG_9074.heic` show the comparison stopped on the
pre-Evaluate GET_STATUS, before the Evaluate Context command. Its Transfer
Event matches the expected Status Stage TRB and passes the structural checks,
but reports completion code 6 (Stall Error); the two-byte status response was
written as `0x0000`. The Stall Error is already present on a second child EP0
control transfer while the endpoint still has its default MPS of 8, so this
failure cannot be attributed to the later MPS update. The next candidate
repeats the same 8-byte Device Descriptor request before GET_STATUS. If the
repeated descriptor stalls, the fault is in continuing/reusing the child EP0
transfer ring or endpoint state; if it passes, the failure is specific to
GET_STATUS. The candidate retains the before/after-Evaluate comparison if
those earlier TDs pass.

**Repeated child descriptor comparison staged (2026-10-25):** the candidate
adds an identical eight-byte Device Descriptor TD at EP0 ring indexes 3–5,
before the pre-Evaluate GET_STATUS at indexes 6–8. The repeated prefix is
validated against the accepted descriptor bytes. Evaluate Context, the
post-Evaluate GET_STATUS, and the full descriptor retry move to indexes 9–11,
12–14, and the same command-ring position. Host coverage checks that all TDs
remain consecutive and non-overlapping. `make check-pi4-platform pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass. The staged
kernel SHA-256 is
`24e285c8ae7c50d856d47cdb9e87e8ad2fc954aaf08bb31c3870b2fa0fd8d4ad`; the
booter is
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, demo is
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`,
`limine.conf` is
`4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`, and
`BOOTAA64.EFI` is
`3f51768c5eb8a0818b8e7aa2ac9a9caef3c64707b5b6db15d1e612de23236b8c`. Before
replacing the active payload, all five files were copied into
`ZEALOS-BACKUP-20261025-CHILD-REPEAT-DESCRIPTOR/`, checked against its
`SHA256SUMS`, and compared byte-for-byte with their originals. All five new
active files then matched `build/pi-esp/` byte-for-byte after sync. macOS
safely ejected `/dev/disk17`; this card is ready to boot.

**Repeated child-descriptor physical result (2026-10-25):** photos
`IMG_9076.heic`–`IMG_9078.heic` show that the repeated 8-byte Device Descriptor
transfer succeeds and validates after the first prefix transfer. The next
pre-Evaluate `GET_STATUS` again receives a matching Transfer Event for its
Status Stage TRB with Stall Error (completion code 6); the two-byte response
buffer contains `0x0000`. This rules out a general failure to submit a second
EP0 control TD and confirms that the failure is specific to `GET_STATUS` (or
its two-byte request path), before any MPS update. The next candidate replaces
that pre-Evaluate probe with standard device `GET_CONFIGURATION` (`bmRequestType`
`0x80`, `bRequest` `0x08`, one IN byte) while retaining the post-Evaluate
`GET_STATUS` probe. A pass should return configuration value 0 because the
child has not received `SET_CONFIGURATION`; a stall narrows the issue to
requests beyond `GET_DESCRIPTOR`. USB 2.0 §9.4.2 specifies that
`GET_CONFIGURATION` returns one byte and must return zero in the Addressed
state ([USB-IF specification page](https://www.usb.org/document-library/usb-20-specification),
[§9.4.2 text](https://www.seriesten.org/docs/protocols/USB_2.0_Specification.pdf)).

**GET_CONFIGURATION diagnostic built and staged (2026-10-25):** the new
candidate sends a one-byte standard device `GET_CONFIGURATION` at EP0 ring
indexes 6–8 after the successful repeated descriptor; it requires the returned
configuration value to be zero. The post-Evaluate `GET_STATUS` remains at
indexes 9–11 and the full descriptor retry at 12–14. Host checks validate the
setup packet (`80 08 00 00 00 00 01 00`), one-byte IN Data Stage, and OUT Status
Stage. `make check-pi4-platform pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. The staged kernel SHA-256 is
`eb102e50b70fb40efd8db2558da9d6edf711e872e39fd8b7a3d4b5cbc6c4cf06`; the
booter is
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, demo is
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`,
`limine.conf` is
`4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`, and
`BOOTAA64.EFI` is
`3f51768c5eb8a0818b8e7aa2ac9a9caef3c64707b5b6db15d1e612de23236b8c`. Before
replacing the active payload, the five prior files were copied to
`ZEALOS-BACKUP-20261025-GET-CONFIG-PROBE/` and verified against its SHA256SUMS.
After sync, all five new files matched `build/pi-esp/` byte-for-byte. macOS
unmounted `disk17s1` and safely ejected `/dev/disk17`; the card is ready to boot.

**GET_CONFIGURATION physical result (2026-10-25):** photos `IMG_9080.heic`–
`IMG_9082.heic` show that the repeated 8-byte Device Descriptor still passes.
`GET_CONFIGURATION` returns the expected byte `0x00`, but its Transfer Event
has completion code 6 (Stall Error) and event-check mask `0x0f`: the event is
structurally valid and points to the expected Status Stage TRB, while the
success bit is absent. Thus the one-byte IN Data Stage completed and DMA wrote
the correct value; the failure is the following OUT Status Stage.

**EP0 Transfer TRB encoding correction (2026-10-25):** this result prompted a
spec audit of the shared Setup/Data/Status builders. The Setup Stage builder
was setting bit 4 even though xHCI §6.4.1.2.1 reserves bits 4:1 as zero. It
also set CH in a one-TRB Data Stage TD, even though §6.4.1.2.2 defines CH for
additional TRBs within a Data Stage TD and requires it clear on the terminal
TRB. The builder now leaves both reserved Setup bits and the terminal Data
Stage CH bit clear. `make check-pi4-platform pi-sd` passes, including exact
TRB-field assertions; this is the next physical candidate. The result is a
confirmed spec correction, while whether it resolves the Status Stage Stall
remains to be checked on the Pi. See the [Intel xHCI specification,
§4.11.2.2 and §6.4.1.2.1–2](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf).

**Spec-correct EP0 candidate staged (2026-10-25):** the staged kernel
SHA-256 is `1814959f97176baeb56e556f93bcfd8cb7f7efd67458715bb74a89661b91c610`;
the booter is unchanged at
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, demo at
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`, Limine
config at `4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`,
and EFI at `3f51768c5eb8a0818b8e7aa2ac9a9caef3c64707b5b6db15d1e612de23236b8c`.
The previous active five-file payload was copied and verified in
`ZEALOS-BACKUP-20261025-EP0-TRB-FLAGS/` before overwrite, and every staged file
matches `build/pi-esp/` byte-for-byte after sync. The normal unmount/eject
was initially refused by macOS `loginwindow` while the Mac was locked. After
unlock, macOS successfully unmounted `disk17s1` and ejected `/dev/disk17`; the
card is ready for the next Pi 4B boot.

**Spec-correct child EP0 physical result (2026-10-25):** photos
`IMG_9084.heic`, `IMG_9085.heic`, and `IMG_9087.heic` show the corrected Setup
and terminal Data Stage TRB flags resolving the prior control-transfer Stall
Error. The repeated child Device Descriptor still passes; `GET_CONFIGURATION`
returns `0x00` with a successful, fully checked Transfer Event; Evaluate Context
reports requested/output EP0 MPS `8/8`; post-Evaluate `GET_STATUS` returns
`0x0000` with all event checks passing; and the full 18-byte child Device
Descriptor reports transfer/validation/observed `1/1/1`. Its first words
`0x02000112/0x08000008` decode as USB 2.0, Mass Storage class, and Full-Speed
EP0 MPS 8. This confirms child control-IN DMA through a full descriptor, but
does not configure the child or exercise a bulk/storage endpoint. The next
bounded candidate reads only its nine-byte Configuration Descriptor header.

**Child Configuration-header diagnostic built (2026-10-25):** following the
full-descriptor pass, the candidate submits `GET_DESCRIPTOR(Configuration)`
for nine bytes at child EP0 ring indexes 15–17, validates the returned header,
and saves three raw words plus its Transfer Event and TRB controls. It stops
before `SET_CONFIGURATION` or any endpoint transfer. Host tests check the
consecutive ring placement and reject malformed configuration headers.

**Child Configuration-header candidate staged (2026-10-25):**
`make check-pi4-platform pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. The Pi ESP candidate kernel SHA-256 is
`2f3d33e419e9b59d031f467109d0b0ab27c573ffe40456ddef0f05d868ea20d4`; the
booter is `0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`,
demo is `e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`,
Limine config is
`4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`, and EFI
is `3f51768c5eb8a0818b8e7aa2ac9a9caef3c64707b5b6db15d1e612de23236b8c`.
Before replacing the active payload, all five current files were copied into
`ZEALOS-BACKUP-20261025-CHILD-CONFIG-HEADER/` and verified with its SHA256SUMS.
All five new payload files then matched `build/pi-esp/` byte-for-byte, and
macOS safely ejected `/dev/disk17`. The card was ready for the header check;
this candidate did not configure the child or send endpoint traffic.

**Child Configuration-header physical result (2026-10-25):** photos
`IMG_9089.heic`–`IMG_9091.heic` show the transfer, event, and header validation
all passing (`rc/valid/observed = 1/1/1`, event checks `0x1f`). The returned
words `0x00540209/0xa0040103/0xa5a5a531` decode to a 9-byte Configuration
Descriptor with `wTotalLength=84`, three interfaces, configuration value 1,
attributes `0xa0`, and maximum power 98 mA. The complete descriptor chain has
not yet been fetched; the next candidate requests exactly the reported 84 bytes
and validates descriptor bounds, interface uniqueness, and each alternate
setting's endpoint count before stopping without `SET_CONFIGURATION`.

**Full child Configuration Descriptor candidate built (2026-10-25):** the
probe reuses the bounded configuration buffer only after the header passes,
checks that `wTotalLength` fits its remaining capacity, then submits a standard
`GET_DESCRIPTOR(Configuration)` for exactly that length at child EP0 ring
indexes 18–20. It validates the complete descriptor walk, counts interface
descriptors, unique interfaces and endpoints, and reports the first interface
and endpoint summary. It stops before `SET_CONFIGURATION` or endpoint traffic.
Host tests cover the full descriptor request setup, chain parsing, interface
and endpoint counts, malformed lengths, and DMA-capacity rejection.

**Full child Configuration Descriptor candidate staged (2026-10-25):**
`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. The candidate kernel SHA-256 is
`89758fc2c40c6cbdd5dab1080e985363552e6b19ab99a919b9f114f063964d60`; the
booter is `0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`,
demo is `e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`,
Limine config is
`4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`, and EFI
is `3f51768c5eb8a0818b8e7aa2ac9a9caef3c64707b5b6db15d1e612de23236b8c`.
The prior active payload is preserved and checksum-verified in
`ZEALOS-BACKUP-20261025-CONFIG-FULL-DESCRIPTORS/`; all five staged files match
`build/pi-esp/` byte-for-byte. macOS safely ejected `/dev/disk17`. The physical
check should capture `xHCI child Configuration Descriptors` and its total,
interface, and endpoint summary; this candidate does not configure the child.

**Full child Configuration Descriptor physical result (2026-10-25):** photos
`IMG_9093.heic`–`IMG_9095.heic` show the full 84-byte descriptor transfer and
validation passing (`rc/valid/observed = 1/1/1`, Transfer Event completion
`0x01000000`, checks `0x1f`). The parsed summary is total/value/interface
descriptors/unique interfaces/endpoint descriptors `84/1/3/3/3`. The first
interface is number 0, class 8, subclass 6, protocol `0x50` (mass storage BOT),
with two endpoints. Its first endpoint is `0x81` (bulk IN), attributes 2,
maximum packet 512, interval 0. The current image stopped before selecting
the child configuration.

**Child configuration-selection verification candidate built (2026-10-25):**
after the validated descriptor chain, the probe submits no-data
`SET_CONFIGURATION(1)`, validates the Transfer Event, then requests the
one-byte `GET_CONFIGURATION` value and requires it to equal the descriptor's
configuration value. It uses consecutive child EP0 ring entries 21–25 and a
separate bounded DMA byte. The probe stops before configuring xHCI endpoint
contexts or issuing bulk/HID traffic. Host tests cover the setup packet and
ring placement. `make check-pi4-platform`, `make pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass. The kernel SHA-256
is recorded with the card-stage result below.

**Child configuration-selection verification candidate staged (2026-10-25):**
the prior five-file card payload was preserved and checksum-verified in
`ZEALOS-BACKUP-20261025-CHILD-CONFIG-SELECT-VERIFY/`. The new card payload
matches `build/pi-esp/` byte-for-byte. Kernel SHA-256 is
`761e25460d6d74b668ad3c995d3f6ff7c92f0b171dd18ff2137b9674bc7516fc`; booter
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`; demo
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`; Limine
config `4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`;
EFI `3f51768c5eb8a0818b8e7aa2ac9a9caef3c64707b5b6db15d1e612de23236b8c`.
All staged hashes were read back and verified before `diskutil eject /dev/disk19`
completed successfully. Boot this card and capture the `xHCI child
SET_CONFIGURATION` and `xHCI child GET_CONFIGURATION` report lines. A passing
result should show Transfer Event checks `0x1f`, completion `0x01000000`, and
the returned configuration value `1`; no child endpoint traffic is attempted.

**Child configuration-selection physical result (2026-10-25):** photos
`IMG_9096.heic`–`IMG_9098.heic` show `SET_CONFIGURATION` succeeded with
Transfer Event completion `0x01000000` and checks `0x1f`. The following
`GET_CONFIGURATION` returned and observed value `1`, also with completion
`0x01000000` and checks `0x1f`. The selected configuration is therefore active
on the child. The candidate stopped before enabling its bulk endpoint contexts
or sending storage traffic.

**BOT bulk endpoint-context candidate built (2026-10-25):** the validated
Configuration Descriptor parser now captures the BOT alternate-zero bulk IN
and OUT endpoint addresses, attributes, and packet sizes. The probe seeds
separate transfer rings and endpoint contexts, guards the post-Link command
ring cycle with a No-op, then issues Configure Endpoint for the child slot.
It validates the configured Slot and Output Endpoint Contexts after DMA
readback and sends no CBW, data, CSW, or HID transfer. Unit tests cover BOT
descriptor parsing, malformed high-bandwidth bits, endpoint context fields,
ring dequeue pointers, slot/endpoint states, the wrapped command cycle, and
packet-size/speed rejection. `make check-pi4-platform`, `make pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass.

**BOT bulk endpoint-context candidate staged (2026-10-25):** the prior five
active files were preserved and verified in
`ZEALOS-BACKUP-20261025-CHILD-BULK-CONTEXTS/`. The staged card files match
`build/pi-esp/` byte-for-byte. SHA-256: kernel
`1261d1656881077a4a8cbfe9747f79a3ef8a8549e582d54951581ca7499ead92`, booter
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, demo
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`, Limine
config `4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`, EFI
`3f51768c5eb8a0818b8e7aa2ac9a9caef3c64707b5b6db15d1e612de23236b8c`.
macOS safely ejected `/dev/disk19`. Boot this card and capture the `xHCI
child bulk guard` and `Configure Endpoint` report lines. A pass should report
both command completion codes `0x01000000`, context verification `1`, slot
state 3, and both bulk endpoint states 1. No endpoint data transfer is issued
by this test.

**BOT GET_MAX_LUN and speed-gate candidate built and staged (2026-10-25):**
the new kernel runs the read-only BOT `GET_MAX_LUN` control request over the
child EP0 ring after `SET_CONFIGURATION` / `GET_CONFIGURATION`, records the
Transfer Event pointer, status, control, and one-byte value, then validates
both bulk endpoint MPS values against the child's negotiated speed. The
current Full-Speed/512-byte combination must stop with
`child-bulk-speed-packet-mismatch` before any Configure Endpoint or storage
traffic. A compatible descriptor may proceed through the existing context
verification, still without CBW/data/CSW or HID traffic. `make
check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. Kernel SHA-256 is
`ceca17b6ba36e749ce942daed390f6802aecc134901cba0598167227149f3467`; booter
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`, demo
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`, Limine
config `4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`,
EFI `3f51768c5eb8a0818b8e7aa2ac9a9caef3c64707b5b6db15d1e612de23236b8c`.
The previous five active card files were copied to
`ZEALOS-BACKUP-20261025-GET-MAX-LUN-SPEED-GATE/` and checksum-verified before
replacement. All five new payload files match `build/pi-esp/` byte-for-byte;
firmware, DTBs, and other card files were left unchanged. macOS safely ejected
`/dev/disk19`. Boot this card in the Pi 4B and photograph the summary result,
`Child BOT GET_MAX_LUN` line, and child bulk-endpoint descriptor speed/MPS
values. A normal outcome for the observed device is GET_MAX_LUN pass (value
0), followed by `child-bulk-speed-packet-mismatch` and no Configure Endpoint
completion.

**Bulk endpoint-context physical result and next diagnostic (2026-10-25):**
photos `IMG_9100.heic`–`IMG_9102.heic` show the active candidate stopped at
`no-op-child-bulk-endpoint-configure-failed`, before the endpoint Configure
command. The child is Full-Speed (speed ID 1), while both BOT bulk descriptors
advertise 512-byte maximum packets. Full-Speed bulk endpoints permit at most
64 bytes, so those endpoint contexts cannot be valid for the observed link.
The sentinel bulk-context fields are consistent with stopping before any
endpoint command or storage transfer. This does not establish whether the
device's descriptor is malformed or the speed observation needs further
investigation; it does establish that issuing Configure Endpoint with these
values would be unsafe.

The next card image adds the read-only BOT `GET_MAX_LUN` request on the already
working child EP0 path, then checks both bulk MPS values against the child's
speed. It will report `child-bulk-speed-packet-mismatch` and halt xHCI without
issuing Configure Endpoint when the present Full-Speed/512-byte combination
recurs. A compatible pair may proceed to the existing endpoint-context
verification. No CBW, storage data, CSW, or HID transfer is added. Host tests
check the `GET_MAX_LUN` setup packet and ring placement, legal Full-Speed and
High-Speed bulk packet sizes, and rejection of Full-Speed/512-byte packets.

1. **Docs + `make pi-sd`** — done.
2. **Platform constants** (`plat_pi4.h`) — UART/GIC phys; expand via DT later.
3. **Serial hello (PI-DIAG-2)** — **accepted**.
4. **Soft CNTP (PI-DIAG-3)** — **accepted**.
5. **GICv2 + CNTP IRQ (PI-DIAG-4)** — **accepted** (Device-map FF window).
6. **Serial/FB shell** — **reached** (banner + cmd list after IRQ OK). **M200:** `pl011_rx_prep` +
   `uartrx` for USB-TTL @ 115200; still need hardware eyes-on (`uartrx` then `help`/`halt`).
7. **PCIe discovery and xHCI capabilities** — **physically accepted
   (2026-10-21):** Pi FDT windows, active link, 1 MiB DMA reservation, VL805
   `1106:3483` identity/BAR, live-window BAR translation, and safe capability
   reads confirmed; bus mastering and xHCI stayed off.
8. **xHCI and USB input** — read-only USBCMD/USBSTS and PORTSC snapshots,
   firmware ownership handoff, halted bounded HCRST/CNR reset, and CPU-only
   DMA/cache validation are physically accepted. The Pi 4B identity DMA mapping,
   command-ring Link TRB, and VL805 overfetch padding are confirmed;
   `USBSTS=0x10` is PCD, not HSE. Ordered low/high Dword writes now pass the
   pre-BME setup readback check. The physical run confirms the 16-GiB alias,
   event-ring DMA, a connected-port reset event, MFINDEX-wrap event, sequential
   No-op command completions, empty-slot enable/disable, and command-ring Link-TRB
   cycle wrap; the BAR2 window is restored. Post-reset PORTSC reports speed ID 3
   and readiness yes. Physical tests confirmed pre-Link BSR=1 Address Device,
   BSR=0 USB address assignment, and an EP0 Device-descriptor prefix transfer
   with a fully validated Transfer Event. The attached device is a hub. The
   physical run passed full Device/Configuration and Hub descriptor DMA. The
   physical hub test confirmed the power request and ports 1–2 status reads;
   an earlier port-3 GET_STATUS timed out. A later run received status events
   for all ports, then showed only requested ports 1 and 4 powered despite the
   ganged descriptor. The latest physical photos confirm all four are powered
   and identify a connected child on downstream port 3. Photos
   \`IMG_9036.heic\`–\`IMG_9038.heic\` physically accept the guarded reset and
   C_PORT_RESET clear. Photos `IMG_9046.heic`–`IMG_9048.heic` physically
   accept the hub Configure Endpoint, post-address No-op guard, and routed
   Address Device for a Full-Speed child (speed 1, route port 3, slot 2, USB
   address 2, slot state 2). Photos `IMG_9049.heic`–`IMG_9051.heic` accept
   the child's 8-byte Device Descriptor prefix read, including its reported
   Full-Speed EP0 MPS of 64. Photos `IMG_9053.heic`–`IMG_9055.heic` show the
   Evaluate Context command itself completed, but the diagnostic read the
   wrong output-context index and stopped before validating the new MPS. The
   corrected DCI 1 selection passed physically. The full descriptor then
   stalled before writing its DMA buffer. A post-Evaluate GET_STATUS probe
   received the two-byte zero status data but stalled on its OUT Status Stage.
   The pre/post comparison then showed that the same Stall Error occurs in the
   pre-Evaluate GET_STATUS, before MPS changes. The repeated-descriptor
   candidate passed the second 8-byte Device Descriptor read, then reproduced
   the GET_STATUS stall. The spec-correct EP0 Setup/Data TRBs now pass
   GET_CONFIGURATION, post-Evaluate GET_STATUS, and the complete child Device
   Descriptor. The child Configuration Descriptor header also passes and
   reports three interfaces over an 84-byte configuration. The full chain
   passes structural validation and identifies a Mass Storage BOT interface
   with bulk endpoints. The current candidate selects configuration 1 and
   verifies it with GET_CONFIGURATION. The latest physical candidate reached
   the BOT bulk-context step but stopped safely before Configure Endpoint: the
   child was Full-Speed while its bulk endpoints advertised 512-byte packets.
   The next card candidate probes `GET_MAX_LUN` over EP0 and rejects that
   speed/MPS mismatch before endpoint setup. Bulk storage transfers and HID
   input remain future work.
   The probe retains the bounded Stop Endpoint trace for transfer timeouts.

**GET_MAX_LUN preparation trace staged (2026-10-25):** photos `IMG_9104.heic`
through `IMG_9106.heic` show that the card running kernel
`ceca17b6ba36e749ce942daed390f6802aecc134901cba0598167227149f3467` reached
the new GET_MAX_LUN stage but ended at `child-bot-get-max-lun-failed`; no
Transfer Event or setup/data/status control values were recorded. The child is
now reported at High Speed (speed ID 3), so the previous Full-Speed/512-byte
packet mismatch is no longer the stopping condition. This candidate adds a
`prepare` code to distinguish a missing BOT interface (1), out-of-range
interface number (2), setup seed failure (3), data seed failure (4), status
seed failure (5), and all three TRBs seeded (6). `make check-pi4-platform`,
`make pi-sd`, and `git diff --check` pass. Kernel SHA-256 is
`bc079310e875db6afb5333114c4b330e4507418b85fccfaa9dc4fa061f7ae828`; the
unchanged booter, demo, Limine config, and EFI payload hashes are respectively
`0b62747a6238051d734dc8eef88d36caeef36cd6eef56ac2da7256d1c4fdfc39`,
`e37c1b530962064c451fac288327f7ae799d3e344cc5c77dc2ba55625c0042d0`,
`4900e1570ebe9ddf6f53c568dde5200a8636cfb182fa7ae28798dcc4e8f2cfbd`, and
`3f51768c5eb8a0818b8e7aa2ac9a9caef3c64707b5b6db15d1e612de23236b8c`. All
five previous active payloads were checksum-verified in
`ZEALOS-BACKUP-20261025-GET-MAX-LUN-PREPARE-TRACE/` before replacement. The
new payloads match `build/pi-esp/` byte-for-byte, then were synced and safely
ejected from `/dev/disk17`. Boot this card and capture the `Child BOT
GET_MAX_LUN ... prepare=` line. A value of 1–5 identifies the failed
preparation check; 6 means the control TD was seeded and the following event
fields identify the USB response.

**No BOT interface on the latest physical test (2026-10-25):** photos
`IMG_9109.heic`–`IMG_9111.heic`, `IMG_9113.heic`–`IMG_9116.heic`, and the new
`IMG_9118.heic`–`IMG_9121.heic` show the same `bc079310…f7ae828` kernel
successfully reading and validating an 84-byte configuration descriptor with
three interfaces and three endpoint descriptors. The first interface reports
class 3 (HID), subclass 3, protocol 1, with one endpoint; the summary says
`BOT found=0`. `GET_MAX_LUN prepare=1` therefore means no BOT interface was
present on the selected child, so no control transfer was attempted. This is
consistent with a non-storage child, not a failure to seed the GET_MAX_LUN
TRBs. The five active microSD payloads still match `build/pi-esp/` byte for
byte, so this test required no image update; the `IMG_9120.heic`–`IMG_9121.heic`
repeat the same no-BOT result. After syncing, the card was safely ejected. To
continue the BOT/storage path,
connect a USB Mass Storage stick to the Pi's selected downstream hub port and
capture the same configuration/BOT lines; disconnecting other USB devices
will ensure the probe does not select a HID child first. FAT32 on an MBR
partition is a sensible format for later filesystem work, but this BOT probe
only identifies USB interfaces and does not read the stick's filesystem. HID
endpoint and report handling remains a separate driver step.

**High-Speed child MPS failure (corrected reading, 2026-10-25):** photos
`IMG_9122.heic`–`IMG_9123.heic` end at
`child-ep0-evaluate-context-failed` after 0x1c events. The earlier interpretation
mistook the parent hub's Device Descriptor for the child's and incorrectly
concluded an external hub was connected. The child reports speed ID 3; its
64-byte EP0 already has the fixed High-Speed size. The code nevertheless called
a helper restricted to Full-Speed (speed ID 1), which rejected preparation
before issuing Evaluate Context. The ROOT-SS-1 candidate preserves fixed-size
EP0 contexts and publishes a No-op at command index 254 to keep the command
ring contiguous; a changed Full-Speed MPS still uses Evaluate Context.

**SuperSpeed device missed by root selection (corrected reading, 2026-10-25):**
photos `IMG_9124.heic`–`IMG_9125.heic` actually show
`hub-no-connected-child` at event count `0x12`, rather than a hub Configure
Endpoint failure. The USB 2 hub has no connected child, but the controller's
root-port snapshot shows **root port 3 `PORTSC=0x00081203`**: connection,
enable, power, and speed ID 4 (SuperSpeed). The probe selected the first
connected root, port 1 (the parent USB 2 hub), and never enumerated root 3.
Changing physical USB sockets could therefore move the stick outside the
only path the probe scanned. This is a driver selection gap; the previous
advice to keep re-seating or moving the drive did not address it.

**ROOT-SS-1 candidate (2026-10-25):** prefer a powered, connected SuperSpeed
root over the USB 2 hub, take a fresh inventory of every root port, and use a
VL805 warm reset with WRC completion for that link (following the local emu68
driver's `xhci_roothub_warm_reset_port`). Address Device uses EP0 MPS 512;
Device Descriptor validation accepts the USB 3 exponent encoding 9. The
direct-root path reads and validates the complete configuration chain,
including SuperSpeed companion descriptors, selects a BOT configuration,
checks GET_CONFIGURATION, and probes GET_MAX_LUN. It does not issue bulk
storage transfers. The USB 2 hub fallback is retained with the fixed-speed
MPS correction above. A compact `USB route ROOT-SS-1` / `USB direct` report
keeps root selection, device identity, and each control-transfer result
together.

`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. Kernel SHA-256 is
`fa29c6e752ad6ad741cc8c127d5517fb2e2dce5775e629ae2ee6daa4466282a2`;
the booter, demo, Limine config and EFI payloads retain their hashes listed
above. Host tests use the literal root-port snapshot from `IMG_9124.heic`, verify
selection of root 3 over root 1, warm-reset control/change masks, SuperSpeed
512-byte EP0 contexts, exponent-9 descriptors, BOT parsing across SS endpoint
companions, UAS exclusion, separate DMA cache lines, and fixed-speed MPS
preservation. Hardware acceptance is pending. Repeat the **same bottom-right
USB port** test; expected routing is selected root 3 / speed 4, followed by
the drive's own VID/PID and configuration/BOT result. No filesystem format
change is required for this control-transfer milestone.

**ROOT-SS-1 card staging (2026-10-25):** all five previous active payloads,
including kernel `bc079310…f7ae828`, were copied and SHA-256 verified under
`ZEALOS-BACKUP-20261025-ROOT-SS-1/` before replacement. `SHA256SUMS` records the
backup and `NEW-SHA256SUMS` records the new payload. The five active files were
read back and match `build/pi-esp/` byte-for-byte; the new kernel is
`fa29c6e752ad6ad741cc8c127d5517fb2e2dce5775e629ae2ee6daa4466282a2`.
After sync, `diskutil eject /dev/disk17` succeeded normally. Physical
ROOT-SS-1 routing/descriptor/BOT acceptance remains pending.

**ROOT-SS-1 physical result (2026-10-25):** photos `IMG_9127.heic` and
`IMG_9128.heic` confirm the new build marker, but the one-shot early inventory
still selected root 1. At that instant root 3 was `PORTSC=0x000002b1`:
connected and powered, reset in progress, speed field zero. The later
controller snapshot shows the same root at `0x00081203` (SuperSpeed). Thus the
USB 3 link was still training when selection ran. The probe again reached
`hub-no-connected-child`; the direct-root control sequence was not attempted.
The card's active kernel was verified as `fa29c6e7…66282a2` on reconnection.

**ROOT-SS-2 candidate (2026-10-25):** resolve each root's USB 2/USB 3 ownership
from the Supported Protocol capabilities before selection. A powered,
connected USB 3 candidate retains priority even with speed zero or reset busy.
Resample root status at 10 ms intervals for at most one counter-second (and
at most 101 scans), stopping early when a SuperSpeed link has a defined speed
and no reset in progress. If a USB 3 candidate remains busy at the bound, it
is reported as that selected route rather than silently redirecting the probe
to the hub. Warm-reset completion is selected from protocol ownership even
if the speed field was temporarily zero. Existing warm reset, direct EP0 and
USB 2 fallback paths otherwise continue as in ROOT-SS-1.

The compact report now identifies `ROOT-SS-2`, scan count/wait milliseconds,
and each root's protocol plus initial/final discovery PORTSC. Regression tests
cover the exact early `0x2b1` snapshot followed by `0x81203`, retaining the USB
3 selection while reset is busy, waiting for the fixed speed, and falling back
to USB 2 when no USB 3 connection exists. Repeat the same bottom-right-port
setup for physical validation.

`make check-pi4-platform`, `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass for ROOT-SS-2. The kernel SHA-256 is
`956d2e4178aa34034cf323f981778016c97b3b46c95fa2fa8de4ae92959f749f`;
the other four payload hashes remain unchanged. The build marker and new
discovery fields are present in the staged ELF. Physical validation is pending.

**ROOT-SS-2 staged (2026-10-25):** the five prior active payloads were copied
and SHA-256 verified in `ZEALOS-BACKUP-20261025-ROOT-SS-2/` (previous kernel
`fa29c6e7…66282a2`). The new five-file payload was read back and matches
`build/pi-esp/` byte-for-byte; `SHA256SUMS` and `NEW-SHA256SUMS` retain both
manifests. After syncing, normal `diskutil eject /dev/disk17` succeeded.
Boot this card in the same bottom-right-port setup and capture the short
`USB route ROOT-SS-2`, discovery, and `USB direct` result lines.

**ROOT-SS-2 physical result (2026-10-25): accepted.** Photos
`IMG_9130.heic` and `IMG_9131.heic` show selected root **3**, speed **4**,
and six discovery scans / `0x32` (50) ms. The initial root-3 status was
`0x000002b1` (connected USB 3 link still resetting); bounded discovery kept
that route until it trained. The drive's own Device Descriptor reports
VID/PID **`0781:5580`**, class 0. Its 44-byte configuration has one
class/subclass/protocol **8/6/0x50** BOT interface. Full configuration read,
SET_CONFIGURATION, GET_CONFIGURATION, and GET_MAX_LUN each return `rc=1`,
completion `0x01000000`, checks `0x1f`; selected configuration is **1**,
Max LUN **0**. The final state is **`direct-root-bot-ep0-pass`**, 12 events.
This confirms direct SuperSpeed enumeration, not just the USB 2 parent hub.
The reconnected card's kernel matches the staged ROOT-SS-2 SHA-256
`956d2e4178aa34034cf323f981778016c97b3b46c95fa2fa8de4ae92959f749f`.
Bulk endpoint operation, media access, and HID input are not proved by this run.

**BOT-INQUIRY-1 candidate (2026-10-25):** preserve that accepted root/link/EP0
path, then configure the selected BOT interface's bulk IN and OUT endpoints.
The configuration parser associates each SuperSpeed companion immediately
with its endpoint, checks descriptor size, burst bound, stream capability and
reserved interval bytes, and records the zero-based burst sizes. The direct
probe requires both companions and ordinary non-stream BOT endpoints with
1024-byte MPS. Configure Endpoint uses those burst sizes, supports 32/64-byte
contexts, preserves the root Slot Context, and adds only Slot plus the two
endpoint DCI flags. No-op 250 and Configure Endpoint 251 continue the
cycle-1 command sequence without crossing the Link TRB. Output Slot state,
endpoint types/states, MPS/burst, dequeue pointers/DCS, no-stream mode and
Average TRB Length must read back correctly before any bulk traffic.

The only SCSI command sent is standard **INQUIRY** on LUN 0: a 31-byte CBW
(tag `0x5a504901`), a 36-byte IN data phase, and a 13-byte CSW. Each phase uses
one Normal TRB with IOC; IN also has ISP. Matching Transfer Events must name
the expected slot, endpoint DCI and TRB. Only the INQUIRY data phase permits
Short Packet completion; its actual byte count must reconcile with CSW
residue. CSW signature, exact length, echoed tag and success status are
checked, then the complete standard direct-access identity is validated and
printable vendor/product/revision are reported. This is not a media-read or
filesystem test; **no media writes are issued**. Failure stops the one-shot
probe, with no retry/recovery traffic, and the existing halt/BME/BAR-restore
cleanup remains common to every exit.

The already-parsed configuration DMA space is reused only after EP0 has
finished: CBW at offset 512, data at 576, CSW at 640, each on its own 64-byte
cache line. The compact HDMI/UART report names `BOT-INQUIRY-1`, the endpoint
addresses/MPS/bursts, guard/configure completion, phase-specific event checks
and byte counts, CSW fields and INQUIRY identity. Configure readback failures
retain Output Context words; transfer failures retain event control/pointer.
The USB 2 fallback retains its existing context-only milestone.

Protocol references: [USB-IF BOT 1.0, sections 5.1–5.2 and 6.3](https://www.usb.org/sites/default/files/usbmassbulk_10.pdf)
for the wrapper/status rules; the local `emu68-xhci-driver` endpoint/context
implementation confirms MaxBurst field placement and ordinary bulk TRB
IOC/ISP handling. No code was copied from that driver.

Host validation covers the literal CBW wire bytes, >4-GiB Normal TRB pointers,
IOC/ISP/cycle and 64-KiB boundary rules, success/short/stall events, wrong
slot/DCI/pointer and Event Data rejection, CSW signature/tag/length/residue/
status, complete identity length and text sanitization, SS companion bounds,
and SS context seeding/readback for both context sizes. Existing Full-Speed/
High-Speed and topology/control tests remain in the same suite. Physical
validation is pending: keep the stick in the **same bottom-right USB port**,
boot the next card image, and capture `USB bulk`, `USB BOT`, `USB INQUIRY`
and the final xHCI result. Expected success is **`direct-root-inquiry-pass`**.

**BOT-INQUIRY-1 build validation (2026-10-25):** `make check-pi4-platform`
passes with AddressSanitizer and UndefinedBehaviorSanitizer; `make pi-sd`,
`bash -n scripts/pi-sd-stage.sh`, and `git diff --check` pass. The Pi kernel
SHA-256 is `cb4c8772af99b4200ba72e32f003133bd977fa2d543803cea8f1e2ec5d3bd01a`.
The booter, demo, Limine config and EFI payload retain the four hashes listed
above. The built ELF contains the candidate marker, INQUIRY identity report
and final success string. Existing compiler warnings in the HolyC/virtio code
remain; the new host tests compile with `-Werror`. Host checks do not establish
physical bulk acceptance; that remains the next board run.

**BOT-INQUIRY-1 card staging (2026-10-25):** all five prior active payloads
were copied and SHA-256 verified under
`ZEALOS-BACKUP-20261025-BOT-INQUIRY-1/`, including the accepted ROOT-SS-2
kernel `956d2e41…59f749f`. `SHA256SUMS` records that backup and
`NEW-SHA256SUMS` records the new payload. All five replacements were verified
byte-for-byte against `build/pi-esp/`; active kernel SHA-256 is
`cb4c8772af99b4200ba72e32f003133bd977fa2d543803cea8f1e2ec5d3bd01a`.
After syncing, normal `diskutil eject /dev/disk17` succeeded. The card is
ready for the same bottom-right-port physical INQUIRY test.

**BOT-INQUIRY-1 physical result (2026-10-25): accepted.** Photos
`IMG_9133.heic` and `IMG_9134.heic` show **`direct-root-inquiry-pass`**,
17 events and four command completions. Root 3 / speed 4 remains selected
after six scans / 50 ms. The drive remains VID/PID `0781:5580`, configuration
1, Max LUN 0. Bulk IN `0x81` and OUT `0x02` each advertise MPS `0x400`
(1024), MaxBurst `0xf` (15); guard and Configure Endpoint both complete with
`rc=1`, `0x01000000`. Output Context verification passes. CBW, INQUIRY data,
and CSW each complete successfully with check mask `0x1f` and byte counts
31 / 36 / 13. CSW signature is `0x53425355`, tag `0x5a504901`, residue 0,
status 0. The standard inquiry identifies **SanDisk / Extreme / 0001**
(type 0, format `0x12`, additional length `0x5b`). The reconnected card's
kernel matches `cb4c8772af99b4200ba72e32f003133bd977fa2d543803cea8f1e2ec5d3bd01a`.
This proves the bounded physical bulk OUT/IN command/data/status path.
It does not yet prove sector reads, a usable filesystem, or USB HID input.

**BOT-READ-1 candidate (2026-10-25):** retain the accepted INQUIRY report
separately, then send READ CAPACITY(10) to LUN 0. Parse the eight-byte
big-endian response as last LBA and logical block size, calculate total bytes
with 64-bit arithmetic, and stop on the `0xffffffff` READ CAPACITY(16)
sentinel or a block size outside the reserved 512/1024/2048-byte range.
The next two commands are one-block READ(10) at LBA 0. Save the first block,
then compare **every byte** of the independent second DMA transfer. Report
both standard CRC32 values, the first four bytes, and bytes 510/511 as a
possible boot signature; that signature is diagnostic, not an acceptance gate.
No partition or filesystem format is assumed and **no media writes are sent**.

Each later CBW receives a unique tag and its own OUT TRB; each data/CSW phase
receives a distinct sequential IN TRB following the INQUIRY's entries. All
remain before the Link TRB. The data buffer is at offset 1024, at most 2048
bytes, separated from CBW at 512 and CSW at 640. Cache clean/invalidate and
slot/DCI/TRB/event, byte-count, echoed tag, residue and CSW checks apply to
every phase. The command log preserves each attempt independently of INQUIRY.
A successful capacity/read command must return its entire requested payload.

A valid CSW command failure (status 1) is distinguished from a malformed
transport or phase error. The former triggers a bounded read-only REQUEST
SENSE; fixed and descriptor sense are decoded. Only Unit Attention (key 6)
or becoming-ready (key 2 / ASC 4 / ASCQ 1) permits a retry after 100 ms, with
at most three attempts per operation and at most 16 later commands overall.
Transport failures, other sense conditions, unsupported capacity or exhausted
attempts stop and retain diagnostics. No BOT reset, endpoint recovery or
unbounded readiness loop is added; common controller halt/BME/BAR restoration
still runs on every exit.

Host tests check exact READ(10) CBW/CDB wire bytes (LE wrapper vs BE LBA),
capacity length/endianness and last-LBA-plus-one arithmetic, the 64-bit size
boundary and CAPACITY(16) sentinel, block/buffer bounds, WRITE(10) exclusion,
REQUEST SENSE encoding and fixed/descriptor bounds, retryable versus terminal
sense, status-1 versus status-2 CSW handling, the standard CRC32 reference
vector, and DMA/ring capacity bounds alongside the existing INQUIRY/SS tests.
The command and response layouts follow [T10 SBC-3, READ(10) and READ CAPACITY(10)](https://t10.org/ftp/t10/document.05/05-344r0.pdf),
with the existing USB-IF BOT wrapper rules. This candidate's physical capacity
and sector-read acceptance is pending. Keep the stick in the **same
bottom-right USB port** and photograph `USB storage`, `USB capacity`,
`USB SCSI`, `USB sense` (if present), `USB sector0`, and the final xHCI result.
Expected success: **`direct-root-block-read-pass`**, two reads, repeat flag 1.

**BOT-READ-1 build validation (2026-10-25):** `make check-pi4-platform`
passes with AddressSanitizer/UndefinedBehaviorSanitizer and `-Werror` on the
DMA tests. `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. Kernel SHA-256:
`c91ec61a85e58a47bc9f0d02182a3d0de8ad0021c3115a0cc331899c1214c381`.
The other four payload hashes are unchanged. Physical capacity and repeated
sector-read acceptance remains pending; host tests are not a substitute for
that next board run.

**BOT-READ-1 card staging (2026-10-25):** the accepted INQUIRY build and
all four companion payloads were copied and SHA-256 verified under
`ZEALOS-BACKUP-20261025-BOT-READ-1/` before replacement. `SHA256SUMS` retains
the old files (kernel `cb4c8772…d3bd01a`), and `NEW-SHA256SUMS` the new files.
All five active payloads were read back and match `build/pi-esp/` byte-for-byte;
new kernel SHA-256 is
`c91ec61a85e58a47bc9f0d02182a3d0de8ad0021c3115a0cc331899c1214c381`.
After sync, normal `diskutil eject /dev/disk17` succeeded. The card is ready
for the same-port capacity/sector0/repeat test; physical acceptance is pending.

**BOT-READ-1 physical result (2026-10-25): accepted.** Photos
`IMG_9135.heic` and `IMG_9136.heic` show **`direct-root-block-read-pass`**.
Root 3 / SuperSpeed and the SanDisk Extreme INQUIRY result remain accepted.
Storage step 4, three later commands, two sector reads, and repeat flag 1
confirm the full byte comparison. READ CAPACITY(10) returns last LBA
`0x074de183`, logical block size `0x200` (512), total bytes
`0x0000000e9bc30800` = **62,742,792,192 bytes** (58.43 GiB). The capacity
command (tag `0x5a504902`) returns 8 bytes; both READ(10) commands (tags
`0x5a504903`/`0x5a504904`) return 512 bytes, success status 0 and valid CSW.
No readiness retry was needed. Both sector-0 CRC32 values are **`e03a2431`**,
boot signature **`aa55`**, first word 0. Final count is 26 events and four
command completions. The reconnected card kernel matches
`c91ec61a85e58a47bc9f0d02182a3d0de8ad0021c3115a0cc331899c1214c381`.
This accepts bounded physical capacity and repeatable raw sector reads, not
a filesystem mount or native USB keyboard/mouse input.

**FAT-ROOT-1 candidate (2026-10-25):** add `src/pi4_fat32.h`, a read-only,
allocation-free root-directory reader using a sector-read callback. The Pi
callback invokes the accepted READ(10)/CSW path and copies the verified
512-byte block into the reader's buffer. The accepted INQUIRY and two
sector-0 comparison results remain in separate reports. The new probe starts
only after those pass and uses the saved sector 0 for initial discovery.

The reader accepts a FAT32 BPB at LBA 0 or the first bounded MBR FAT32
partition (`0b`/`0c` and hidden counterparts). It checks partition extent,
boot signature/jump, 512-byte logical sectors, power-of-two cluster geometry,
FAT32-only BPB fields/version, total extent, FAT capacity and cluster count
(FAT32 classification), active FAT selection, and root-cluster bounds.
GPT and other filesystems are explicitly unsupported. No partition is changed.
The report retains all four MBR type bytes for unsupported-layout diagnosis.

Directory traversal reads the root cluster's active FAT entry and data sectors,
validates next-cluster/EOC/bad/free/reserved markers, detects loops, and stops
at **eight directory sectors, eight clusters, or eight live short entries**.
Deleted entries, long-name records, volume labels and dot entries are skipped;
8.3 aliases, file sizes, attributes and starting clusters are reported, with
printable character sanitization and short-name case flags. A directory end
marker or end-of-chain is a complete result. A budget limit is explicitly
**bounded/partial**, never mislabeled complete. Read failures, corrupt BPB/FAT/
directory entries and unsupported layouts have separate results. No file
contents are read, and **no media writes are issued**. This is a filesystem
discovery milestone, not a writable mount or source-store integration.

The total later BOT-command bound is raised from 16 to 32 so the metadata scan
fits without crossing either transfer-ring Link TRB or exhausting the Event
Ring. Unique tags/TRBs, cache synchronization, strict byte counts, CSW checking,
and limited REQUEST SENSE/retry handling continue unchanged. All exits still
use the common halt/BME/BAR restoration. The compact HDMI/UART report names
`FAT-ROOT-1`, `USB FAT32` geometry/result/completeness/read count and `USB file`
entries. Expected complete result is **`direct-root-fat32-root-pass`**;
**`direct-root-fat32-root-bounded`** is an explicitly partial directory scan.

The parser follows [Microsoft FAT specification v1.03](https://www.pcjs.org/documents/papers/microsoft/MS_FAT_OVERVIEW_103-2000-12-06.pdf).
Host tests execute the same callback-based reader over synthetic disk layouts:
partition offsets, active second FAT, root cluster 130 crossing a FAT-sector
boundary, multi-cluster directories, deleted/LFN records, bad/free/cyclic FAT
chains, I/O faults, invalid live-entry clusters, entry/sector/cluster budgets,
partition overflow, GPT and non-512-byte rejection, BPB geometry/version/type
and undersized FAT rejection. A separate real 64 MiB `mtools` FAT32 image with
`HELLO.TXT` passes root enumeration through a file-backed read callback.
Physical filesystem acceptance remains pending on the same bottom-right-port
setup. Capture `USB FAT32`, `USB file`, and the final xHCI result.

**FAT-ROOT-1 build validation (2026-10-25):** `make check-pi4-platform`
passes, including the new FAT32 suite compiled with `-Werror` and AddressSanitizer/
UndefinedBehaviorSanitizer. The same test binary passes against a real 64 MiB
FAT32 image created with `mformat` and populated using `mcopy`; it finds
`HELLO.TXT`. `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`, and
`git diff --check` pass. Kernel SHA-256:
`e1c4be5ab002b51705c122a699c276939d498a441d167016dbf7ea44bb142f5a`.
The booter, demo, Limine configuration and EFI payload hashes remain unchanged.
The hardware filesystem check remains pending; these host results do not
establish the attached stick's partition or directory contents.

**FAT-ROOT-1 card staging (2026-10-25):** all five previous payloads
were copied and SHA-256 verified in `ZEALOS-BACKUP-20261025-FAT-ROOT-1/`,
including accepted sector-read kernel `c91ec61a…214c381`. `SHA256SUMS` and
`NEW-SHA256SUMS` preserve the old/new manifests. The five active replacements
were read back and match `build/pi-esp/` byte-for-byte; kernel SHA-256 is
`e1c4be5ab002b51705c122a699c276939d498a441d167016dbf7ea44bb142f5a`.
After syncing, normal `diskutil eject /dev/disk17` succeeded. The card is ready
for the same-port physical FAT32 root-discovery test; filesystem acceptance
remains pending.

**FAT-ROOT-1 physical result (2026-10-26): accepted.** Photos
`IMG_9139.heic` and `IMG_9140.heic` show **`direct-root-fat32-root-pass`**
and `root-directory-pass`, with 35 events / four command completions. The
existing SuperSpeed INQUIRY, capacity and repeated sector-0 checks remain
accepted. MBR type is `0b`, partition 1 starts at `0x800` and spans
`0x074dd800` sectors. The volume label is **UNTITLED**, sectors/cluster
`0x40` (64), active FAT 0, root cluster 2. FAT starts at `0x820`, data at
`0x7cf8`, cluster count `0x1d358c`. Three filesystem sector reads, one root
sector and a directory end marker produce two entries, complete=1, bounded=0:
`SPOTLI~1` (attributes `0x12`, cluster 3) and `FSEVEN~1` (attributes `0x12`,
cluster 6), both directories with size 0. The card's accepted kernel is
`e1c4be5ab002b51705c122a699c276939d498a441d167016dbf7ea44bb142f5a`.
This accepts physical FAT32 geometry, FAT entry and root-directory reads.
The original stick had no ordinary test file, so file-data verification is
still a separate milestone.

**FAT-FILE-1 candidate (2026-10-26):** look up only the short name
**ZEALTEST.TXT** in the validated root report (case-insensitive), excluding
directories/volume/reserved attributes. Require its declared length to be
**1025 bytes**, then read it twice through the existing READ(10)/CSW callback.
The allocation-free file reader supports up to 2048 bytes / four clusters,
validates each active-FAT entry and cluster extent, detects cycles and early
EOC, distinguishes FAT/data read faults, and copies only the file's declared
bytes from the final partial sector. It never reads arbitrary user file data.
The guest issues **no writes to the USB stick**.

Both file buffers begin with different poison values. Each completed read is
checked byte-for-byte against repetitions of the public ASCII pattern
`ZealOS Pi4B FAT32 read test.` followed by LF, truncated to exactly 1025 bytes.
Expected standard CRC32 is **`873b79ac`**, SHA-256 of the host fixture is
`c98121f80df98e69c3dfad6d0577b8d3707b0f77cfd4de0cec095b2d7470c305`.
The two completed buffers must also compare byte-for-byte. Missing/unseen,
wrong-sized, read-failed, pattern-mismatched and repeat-mismatched results
remain distinct; a root listing alone cannot pass file acceptance. Root and
raw-sector acceptance reports remain preserved separately. All existing BOT
and filesystem bounds and controller cleanup remain in force.

The HDMI/UART report identifies **FAT-FILE-1** and `USB test-file` name,
cluster/length, both read results and sector/cluster counts, both CRC32 values,
expected CRC32, pattern flags and repeat-match flag. Expected complete success
is **`direct-root-file-read-pass`**, bytes `0x401`, CRC32 `873b79ac` twice,
pattern1/pattern2/repeat-match all 1. `scripts/pi-sd-stage.sh` reproducibly
prepares the public sidecar `build/ZEALTEST.TXT` without writing removable
media. Physical file acceptance remains pending.

File-reader host tests exercise a deliberately fragmented chain 3 → 7 → 9,
three data sectors and 1025-byte EOF with a canary beyond output; case-insensitive
lookup, missing file, pattern corruption in the middle sector, output/file
size bounds, directory rejection, empty files, early EOC, cyclic/free file
chains, and FAT/data I/O failure reports. The real `mtools` image test now also
loads the independently generated ZEALTEST.TXT through the same file reader.

**USB fixture prepared (2026-10-26):** at the user's request the SanDisk Extreme
was connected to the Mac. Its whole-drive capacity (62,742,792,192 bytes),
media name **Extreme**, FAT32 volume **UNTITLED**, UUID
`071EF94B-9DD1-325D-A455-26875254B5DE`, and 1 MiB partition offset match the
physical drive report. A new root ZEALTEST.TXT was created without replacing
any existing file, synced and read-back verified: 1025 bytes, CRC32 `873b79ac`,
SHA-256 `c98121f80df98e69c3dfad6d0577b8d3707b0f77cfd4de0cec095b2d7470c305`.
Normal safe eject of its then-current `/dev/disk17` succeeded. The USB stick
is ready; reconnect ZEALPI separately to stage the kernel before the Pi test.

**FAT-FILE-1 build validation (2026-10-26):** `make check-pi4-platform`
passes, including directory/file tests with AddressSanitizer,
UndefinedBehaviorSanitizer and `-Werror`. A freshly created 64 MiB `mtools`
FAT32 image containing HELLO.TXT and the independently generated ZEALTEST.TXT
passes directory discovery and exact file-content reading through the same
callback-based implementation. `make pi-sd`, `bash -n scripts/pi-sd-stage.sh`,
and `git diff --check` pass. Kernel SHA-256:
`19f85059ea02117ee8f75ae3337786a8191440095dbdd94f85f19a9760b092c4`.
The other four payload hashes are unchanged; the generated sidecar remains
1025 bytes / CRC32 `873b79ac`. Physical file-data acceptance remains pending.

**FAT-FILE-1 card staging (2026-10-26):** the reconnected ZEALPI card was
identified by UUID `A5F20C1B-7A08-33D9-ABB7-0EDFC70E751E`, separately from
the already ejected SanDisk stick. All five old payloads were copied and
SHA-256 verified in `ZEALOS-BACKUP-20261026-FAT-FILE-1/` (prior accepted kernel
`e1c4be5a…b142f5a`), with old/new manifests. The five active replacements were
read back and match `build/pi-esp/` byte-for-byte; new kernel SHA-256 is
`19f85059ea02117ee8f75ae3337786a8191440095dbdd94f85f19a9760b092c4`.
After syncing, normal `diskutil eject /dev/disk17` succeeded. Both the USB
fixture and microSD image are ready and safely ejected. Boot the card with
the SanDisk in the same bottom-right port; physical file acceptance remains
pending.


**FAT-FILE-1 physical result (2026-10-26): accepted.** Photos
`IMG_9141.heic` and `IMG_9142.heic` show **`direct-root-file-read-pass`**,
59 events / four command completions, and both `file-read-pass` results.
ZEALTEST.TXT is found at cluster `0xe1`, length `0x401` (1025 bytes). Each
pass uses four sector reads / one cluster, ending at LBA `0xb4ba`. Both
CRC32 values equal the expected **`873b79ac`**, and
pattern1/pattern2/repeat-match are **1/1/1**. This accepts actual file-data
reads through FAT32 and USB BOT, including the partial final sector and an
independent repeat. The accepted kernel is
`19f85059ea02117ee8f75ae3337786a8191440095dbdd94f85f19a9760b092c4`.
It remains a one-shot diagnostic, not an always-running USB/input driver.

**LIVE-READ-1 candidate (2026-10-26; superseded by LIVE-READ-2 below):** after the accepted file checks,
retain the existing direct SuperSpeed slot, endpoint contexts, reserved DMA
arena and event cursor in a static session. No stack-backed object remains
referenced. Adopt the exact next bulk IN/OUT TRBs after the completed bootstrap
commands; use bounded, single-outstanding transfers with producer cycle
tracking and Link TRB publication. The command ring starts at 248, so the
never-visited prefix 0..247 must be invalidated for the next consumer cycle
before allowing its first wrap. Payloads are cleaned with an invalid cycle
before ownership is published, and the Link is published before the last
ordinary entry. This follows the producer/consumer ownership rules in
[Intel xHCI 1.2b, section 4.9.2](https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf).

The startup check performs **272 No-op + READ(10) transactions**, comparing
every sector-0 byte against the previously accepted copy. It requires at
least two command and IN-ring wraps, one OUT-ring wrap and four event-ring
laps. Then it rereads the known file twice after those wraps, with the same
exact-pattern/CRC/repeat checks. Only full success leaves xHCI RUN, PCI BME
and the inbound alias enabled, with the arena reserved for the session.
The shell services one No-op/read health check per second without requiring
serial input. After ten successful idle checks it prints a fresh summary;
it continues checking thereafter. `usbstatus` reports ownership/counters;
`usbstop` and shell `halt` stop the controller. A health fault stops further
I/O and attempts halt, BME-off and original inbound-alias restoration in that
order. Translation is never changed unless halt and BME-off were verified;
DMA memory is never freed or reused on a failed stop. Guest USB I/O remains
read-only. Automatic reconnect, HID input and desktop startup are not part
of this candidate.

The compact `USB live LIVE-READ-1` report comes before the older routing
report. Expected initial success is **`live-read-only-pass`**,
active/fault/stopped **1/0/0**, boot checks **`0x110`** (272), wraps
command/IN/OUT/event at least **2/2/1/4**, file-after-wrap **1**. After roughly
ten seconds at the shell, expect `10 idle checks passed; controller remains
online`, idle-checks **`0x0a`**, and active/fault/stopped still **1/0/0**.
Use the same SanDisk stick and bottom-right port, leaving it attached during
the test. Physical controller-lifetime acceptance remains pending.

Host ring tests model a consumer running at every cache publication, including
between payload and ownership stores. They prove the skipped-prefix failure
without preparation and the corrected sequence across 1000 commands, four
producer wraps and three event laps; old entries, malformed links, premature
ownership and invalid queue geometry fail the model. Existing BOT, descriptor,
filesystem and DMA bounds tests remain required.


**LIVE-READ-1 validation and card staging (2026-10-26):**
`make check-pi4-platform` passes with AddressSanitizer/UndefinedBehaviorSanitizer
and `-Werror`; the final live-ring suite also passes independently. The same
FAT32 test binary passes the real `mtools` image containing HELLO.TXT and
ZEALTEST.TXT. Both the Pi and ordinary QEMU/UTM kernel builds complete via
`make pi-sd`; `bash -n scripts/pi-sd-stage.sh` and `git diff --check` pass.
The staging script now prints the current lifetime check and ownership rules,
replacing its accumulated historical one-shot instructions. Kernel SHA-256:
`656cc27298cb37cc5aece08c61e8473f6b349859aca0a76b72cb0abf24fc1c85`.
The other four payload hashes and the 1025-byte USB fixture are unchanged.

The mounted ZEALPI card was identified by UUID
`A5F20C1B-7A08-33D9-ABB7-0EDFC70E751E`. All five previous payloads were
backed up and SHA-256 verified in `ZEALOS-BACKUP-20261026-LIVE-READ-1/`,
including accepted FAT-FILE-1 kernel `19f85059…b092c4`, with old/new manifests.
All five active replacements match `build/pi-esp/` byte-for-byte. After syncing,
normal `diskutil eject /dev/disk17` succeeded. The card is ready for the same
bottom-right-port test, with the existing SanDisk fixture left attached.
Physical LIVE-READ-1 acceptance remains pending: capture the initial USB live
summary and the fresh ten-idle-check summary before claiming persistent
controller ownership is accepted. Keyboard/mouse input remains unimplemented
on the Pi backend.


**LIVE-READ-2 correction and validation (2026-10-26):** an additional mixed
command/BOT/event-ring host model exposed a real defect in LIVE-READ-1 before
any lifetime result was accepted. The Command Completion identity validator
still required Cycle=1 even though its caller had already checked ownership
against the event consumer cursor. After the event ring toggled to zero,
valid No-op completions would therefore be rejected. With the accepted
bootstrap's 59 events, the isolated reproduction fails at transaction 50,
No-op phase, event index 3 / cycle 0. This is host evidence predicting a
LIVE-READ-1 failure, not a physical Pi result. The previously staged
`656cc272…fc1c85` image is superseded.

Command, MFINDEX and port-event identity validators now accept both cycle
values, as the Transfer Event validator already did. Every production caller
checks `pi4_xhci_dma_event_ready` against its current cursor before invoking
these validators; stale/unowned events remain excluded. Pointer, event type,
completion status, slot and endpoint identity checks remain in place.

The new mixed model adopts the actual command/OUT/IN positions 252/15/30,
then executes 1000 No-op → 31-byte CBW → 512-byte data → 13-byte CSW
transactions. A simulated consumer runs at every cache publication; completions
are written to an independent 256-entry event ring and checked through the
production validators before ERDP advances. It verifies 4000 completions,
command/OUT/IN producer wraps 4/3/7, fifteen event-ring laps, stale-cycle
rejection before publication, and wrong TRB-pointer/slot rejection. At the
272-transaction startup checkpoint it verifies wraps command/IN/OUT/event
2/2/1/4. An isolated build with the old Cycle=1 command gate restored fails at
the precise transaction above, demonstrating regression coverage. No hardware
acceptance is inferred from the model.

`make check-pi4-platform` passes with `-Werror`, AddressSanitizer and
UndefinedBehaviorSanitizer, including the mixed model and explicit zero-cycle
No-op, MFINDEX and port-event cases. The real `mtools` FAT32 image test passes.
`make pi-sd` builds both Pi and ordinary QEMU/UTM kernels; script syntax and
`git diff --check` pass. The current candidate/report marker is
**LIVE-READ-2**; startup and ten-idle-check acceptance criteria are unchanged.
New kernel SHA-256:
`d05078f21e680a7512565c871a1b47da3b101d50dcc3158dac5d1d4e1489dfc3`.
The other four payloads and USB fixture are unchanged. ZEALPI is still
unmounted after its previous safe eject: this corrected image is built but
**not yet staged**. Reconnect the card before the next physical test so the
superseded image can be backed up and replaced, verified and safely ejected.


**LIVE-READ-2 card staging (2026-10-26):** after the user's reconnection,
ZEALPI was identified by UUID `A5F20C1B-7A08-33D9-ABB7-0EDFC70E751E`.
The active kernel matched the superseded LIVE-READ-1 SHA-256
`656cc27298cb37cc5aece08c61e8473f6b349859aca0a76b72cb0abf24fc1c85`.
All five previous payloads were backed up and SHA-256 verified in
`ZEALOS-BACKUP-20261026-LIVE-READ-2/`, with `SHA256SUMS` and
`NEW-SHA256SUMS`. All five active replacements were synced and read back
byte-for-byte against `build/pi-esp/`; corrected kernel SHA-256 is
`d05078f21e680a7512565c871a1b47da3b101d50dcc3158dac5d1d4e1489dfc3`.
Normal `diskutil eject /dev/disk17` succeeded. The corrected image is now
staged and safely ejected, superseding the preceding unstaged status.

Next physical check: cold-boot the Pi with the existing SanDisk fixture in
the same bottom-right port and leave it attached. Capture the `USB live
LIVE-READ-2` summary and the fresh ten-idle-check summary. Expected
`live-read-only-pass`, active/fault/stopped 1/0/0, boot-checks `0x110`, wraps
command/IN/OUT/event at least 2/2/1/4 and file-after-wrap 1; then ten idle
checks with the controller still online. No LIVE-READ-2 physical result has
yet been received; hardware lifetime acceptance remains pending.


**LIVE-READ-2 physical result (2026-10-26): accepted.** Photos
`IMG_9144.heic`–`IMG_9146.heic` show `live-read-only-pass`, with xHCI
live and the DMA arena retained. Startup active/fault/stopped is **1/0/0**,
boot checks **0x110** (272), wraps command/IN/OUT/event **2/2/1/4**, and
file-after-wrap **1**. The exact file pattern/repeat flags remain 1/1/1 and
both CRC32 values remain `873b79ac`. The initial event count is `0x493`
(1171), command completions `0x114` (276), and event cursor `0x93/1`.
`IMG_9145.heic` also shows **ten idle checks passed; controller remains
online**, idle count `0x0a`, with active/fault/stopped still **1/0/0** and
wraps still 2/2/1/4. This accepts persistent ownership past bootstrap, multiple
ring laps and ongoing shell-side read-only I/O. It does not yet accept USB
input, reconnect handling or native filesystem mounting. The returning card
is ready for the next update; its expected kernel is
`d05078f21e680a7512565c871a1b47da3b101d50dcc3158dac5d1d4e1489dfc3`.


**KBD-BOOT-1 candidate (2026-10-26):** add a native polled boot-keyboard
session to the accepted USB2 hub/child backend. For this physical check,
remove the SanDisk stick and other USB devices; attach one keyboard to the
same bottom-right port. Otherwise the preferred direct SuperSpeed storage
route will continue to win. The existing hub power/reset, TT route, child
address, EP0 MPS, full descriptor and configuration checks are retained.
After structural validation, select an alternate-zero HID class 3 / subclass
1 / protocol 1 interface with one interrupt-IN endpoint and a valid HID
Report-descriptor declaration. Selection works when the keyboard interface
is not the first interface in a composite device; malformed or ambiguous
boot-keyboard ownership is rejected without retaining a partial selection.

After SET_CONFIGURATION/readback, send SET_PROTOCOL(boot), verify
GET_PROTOCOL returns zero, then send SET_IDLE(0), using consecutive EP0 TDs
26..32 and distinct recorded request results. These requests follow
[USB HID 1.11 section 7.2 and appendix B.1/F.5](https://www.usb.org/sites/default/files/hid1_11.pdf).
Build an interrupt-IN context with the descriptor's packet size and converted
LS/FS/HS interval, preserving the child's root/TT/route Slot Context. A No-op
and Configure Endpoint are published at command entries 0/1, cycle zero;
the unused prefix is cycle-invalid so xHC cannot fetch unsubmitted entries.
Verify the running Output Endpoint Context before queuing the first report.

The interrupt ring uses the accepted 8 KiB allocation/guard layout and
cycle-aware producer. One 8-byte report TD is outstanding at a time. Its
64-byte DMA buffer is isolated at control-buffer offset 3072, poisoned and
cleaned/invalidated before each submission. The input hook polls the event
ring without waiting for a quiet keyboard: a NAK is not a timeout. Owned
completions must match the exact TRB, child slot and endpoint and contain
exactly eight bytes. Key-down/up and modifier changes pass HID usages through
the existing OS key mapping/decoder to the shell. Reordered/duplicate usages
never duplicate events; rollover preserves held state. Releases precede
modifier releases; typematic uses 400 ms initial delay / 35 ms repeat.
Faults stop ownership, release held keys/modifiers and follow the accepted
halt/BME-off/alias restoration order. The reserved arena is never recycled
while DMA might own it. Serial input remains available.

The compact keyboard report is repeated at the end of the HDMI diagnostics
and is available with `usbstatus`. It records interface/endpoint/MPS/interval,
protocol readback, class request and command results, reports/keys/ring wraps
and last event status/control. Expected `child-boot-keyboard-online`,
selected/active/stage/fault **1/1/7/0**, protocol **0**, three class request
rc/status pairs **1/0x01000000** and both command return codes **1**.
Type **help** then Enter, then **usbstatus** then Enter. Confirm text/help
output, increasing report/key counters and active=1/fault=0. Exercise Shift,
Backspace and a held key. `usbstop` and `halt` release keys and stop ownership.
The current scope is USB2 boot keyboards and the existing shell key layout;
mouse, report-protocol/NKRO devices, LED synchronization and automatic
reconnection are not implemented by this candidate. The accepted direct
SanDisk read-only/lifetime path is retained separately.

**KBD-BOOT-1 host validation (2026-10-26):** `make check-pi4-platform
check-usb-hid` passes, including the new keyboard suite with `-Werror`,
AddressSanitizer and UndefinedBehaviorSanitizer. Tests cover mouse-first
composite selection, wrong class/alternate/direction/type/MPS/interval,
missing report metadata, ambiguous owners, every truncated prefix and every
single-byte descriptor mutation. Context tests cover 32/64-byte contexts,
LS/FS/HS interval conversion, exact add flags and interrupt fields, root/TT
preservation, EP0 exclusion, output readback and size/alignment failures.
Wire-format tests exercise all three class requests. Report tests cover
modifier/key order, reserved byte, duplicates, reordering, rollover/truncation
state preservation, release and 1000 independent key press/release pairs.
The shared USB HID mapping tests pass. Both Pi and ordinary QEMU/UTM kernels
build with `make pi-sd`; staging-script syntax and `git diff --check` pass.
Kernel SHA-256:
`32713619f92dbbef5a93f04abc4bb56b1f52deea5826bfef9d6bfefb1101e917`.
The other four payloads and the USB file fixture remain unchanged.
Physical keyboard input has not yet been accepted.


**KBD-BOOT-1 card staging (2026-10-26):** the returning ZEALPI card was
identified by UUID `A5F20C1B-7A08-33D9-ABB7-0EDFC70E751E`; its active
kernel matched the physically accepted LIVE-READ-2 hash `d05078f2…489dfc3`.
All five prior payloads were backed up and SHA-256 verified in
`ZEALOS-BACKUP-20261026-KBD-BOOT-1/`, including old/new manifests. All five
active replacements were synced and read back byte-for-byte against
`build/pi-esp/`; kernel SHA-256 is
`32713619f92dbbef5a93f04abc4bb56b1f52deea5826bfef9d6bfefb1101e917`.
The final keyboard host suite also passes with explicit reserved-byte
suppression. Normal `diskutil eject /dev/disk17` succeeded. The card is safely
ejected and ready for the one-keyboard/no-storage-stick cold boot described
above. Physical typing, interrupt report receipt and key counters are still
pending; do not infer keyboard acceptance from endpoint setup or host tests.


**KBD-BOOT-1 physical result (2026-10-26): accepted.** The user reports
“it works.” Photos `IMG_9147.heic`–`IMG_9150.heic` show
`child-boot-keyboard-online`, selected/active/stage/fault **1/1/7/0**, boot
protocol **0**, successful class requests and guard/configure completions.
`IMG_9149.heic` shows the keyboard running **help**, producing the command
list, then **usbstatus**. Reports/keys are **0x86/0x82**; controller ownership
active/fault/stopped remains **1/0/0**. Interface 0, endpoint `0x81`, MPS 8,
interval 8 are selected. This accepts physical interrupt report receipt and
keyboard-to-shell input. The photos do not independently isolate every
modifier/repeat gesture; host regression coverage remains relevant. The
returning card's expected kernel is
`32713619f92dbbef5a93f04abc4bb56b1f52deea5826bfef9d6bfefb1101e917`.

**MOUSE-BOOT-1 candidate (2026-10-26):** extend the same selected USB2 child
session to a boot mouse when no boot keyboard is selected. The descriptor and
interrupt context builders are shared with keyboard selection, preserving its
8-byte minimum and existing tests; mouse selection requires alternate zero,
class/subclass/protocol **3/1/2**, valid HID report declaration and one
interrupt-IN endpoint with MPS **3..64**. Low-speed MPS is limited to 8.
SET_PROTOCOL(boot) and GET_PROTOCOL=0 precede endpoint configuration. SET_IDLE
is not issued to mice. Report format follows
[USB HID 1.11 appendix B.2](https://www.usb.org/sites/default/files/hid1_11.pdf):
buttons 1..3 and signed relative 8-bit X/Y; device-specific extension bytes
are ignored instead of interpreting them as a wheel.

One packet-sized TD remains outstanding, using the same 8 KiB allocation,
cycle-aware producer, isolated 64-byte report buffer and retained static event
cursor. Owned completions must match the pending TRB, slot and DCI, with
success or short-packet completion and at least three actual bytes. Quiet
NAKs do not create timeout faults, movement or releases. Malformed transfers
stop the shared controller session through the accepted halt/BME-off/alias
sequence; held mouse buttons are cleared. Signed movement is clipped to the
framebuffer and enters the common OS pointer sampling API. The existing Pi
shell cursor is visible and left-button dragging paints a stroke. The cursor
now checks sample success before using coordinates after an input fault.

No keyboard commands are needed for the mouse-only physical test. The shell
prints activity summaries at most once per three seconds after actual movement
or button changes (idle-only reports do not scroll the display),
with reports/moves/wraps, coordinates, current/seen buttons and press/release
counts. Disconnect the storage stick, keyboard and other USB devices. Attach
only a wired boot mouse to the same bottom-right port; cold-boot, move in both
axes, click/release left and right, then hold left and drag. Continue movement
for about 15 seconds and photograph the summary plus stroke. Expected
`child-boot-mouse-online`, selected/active/stage/fault **1/1/7/0**, both class
request rc/status pairs **1/0x01000000**, protocol 0, guard/configure rc 1,
increasing reports/moves, ring wrap(s), changed x/y, seen-buttons bits 1 and 2,
and nonzero presses/releases. Only one child/input role is retained; separate
keyboard+mouse together, report-protocol-only devices, wheels, reconnect and
filesystem integration remain outstanding. The accepted keyboard-only and
SanDisk storage paths remain available on separate cold boots. Mouse
hardware acceptance is pending; host tests cannot establish physical input.


**MOUSE-BOOT-1 host validation and staging (2026-10-26):**
`make check-pi4-platform` passes with AddressSanitizer and
UndefinedBehaviorSanitizer, including keyboard regressions and the new mouse
suite. Mouse tests cover composite selection, every truncated prefix and
single-byte descriptor mutation, malformed/ambiguous owners, LS/FS/HS and
32/64-byte contexts, TT/root preservation, MPS 3/4 and low-speed bounds,
signed-byte extremes, coordinate clipping, three-button changes and ignored
extension bytes. Transfer tests reject wrong pointer/slot/DCI, bad completion
codes and fewer than three bytes, accept complete and short reports on both
event cycles, and exercise 5120 movement/button reports over 20 event-ring
laps. Shared HID and USB2 route host tests pass. `make pi-sd` builds both Pi
and ordinary QEMU/UTM kernels; final Pi build includes activity-only automatic
summaries. Script syntax and `git diff --check` pass. The broader
`check-usb-input` QEMU regression is still running; its eventual result will
be recorded separately. Host tests do not establish physical mouse success.

The returning ZEALPI volume was verified by UUID
`A5F20C1B-7A08-33D9-ABB7-0EDFC70E751E`, writable FAT32 identity and the
physically accepted keyboard kernel SHA-256 `32713619…101e917`. All five old
payloads were backed up, synced and SHA-256 verified under
`ZEALOS-BACKUP-20261026-MOUSE-BOOT-1/`, including old/new manifests. All five
active replacements were synced and read back byte-for-byte against
`build/pi-esp/`. New kernel SHA-256:
`b4e7cdf10c0f55e07e111e83165b2ad625339fdfb646362117f5a3eed1ef9605`.
The other four payloads and firmware/DTB are unchanged. Initial normal eject
was refused by macOS `loginwindow`; after the user unlocked the Mac, normal
`diskutil eject /dev/disk17` succeeded. The card is safely ejected and ready
for the mouse-only cold boot above. Mouse movement, buttons, cursor and
painting on the physical Pi remain pending.


**MOUSE-BOOT-1 physical result (2026-10-26): basic pointer accepted.**
Photos `IMG_9151.heic`–`IMG_9154.heic` show
`child-boot-mouse-online`, selected/active/stage/fault **1/1/7/0**, both class
requests successful, protocol 0 and successful endpoint configuration.
The selected interface is 1, interrupt endpoint `0x83`, MPS 64, interval 1.
The live activity summary reaches **0x993 reports** (2451), **0x60e movement
reports** (1550), and **nine transfer-ring wraps**, with active/fault **1/0**.
Visible painted strokes prove coordinate delivery and left-button dragging.
The final visible summary includes button release and seen-buttons **1**.
The user identifies the input device as an Apple trackpad and confirms that
clicking works, but one- and two-finger clicks currently act alike. This
accepts basic pointer motion and primary-button painting, not physical
secondary/middle buttons, scrolling, multitouch or gesture behavior.

The driver used for this test is the generic USB boot-mouse path; it has not
ported the Apple report-mode/multitouch driver. The user's earlier driver was
located at
`~/github/templeos/ZealOS-yde-mac-mini-2014/src/Kernel/Usb/KUsbHid.ZC`:
`UsbTrackpadAttach`, `UsbTrackpadFeatureSend`, and `UsbTrackpadCB` contain the
Magic Trackpad 2 binding, report-protocol/feature setup and contact decoding.
That code is a reference for a future device-specific port; its presence is
not acceptance on ARM64/Pi. In particular, do not claim two-finger secondary
click merely from the existing YDE contact decoder. The returning ZEALPI
card matches the accepted kernel hash `b4e7cdf1…1ef9605`; it has not been
changed again following these photos.

**Remaining Pi work after basic input acceptance:**

1. Extend the retained USB backend beyond one selected child/input role.
   Keyboard, pointer and USB system storage must operate together. Add
   bounded multiple-device enumeration, completion dispatch, disconnect/error
   recovery and physical combined-device acceptance. Apple trackpad
   report-mode/secondary-click/scroll support is an additional input feature.
2. Integrate a writable system disk with the normal filesystem/source store;
   current physical FAT32 access is read-only validation. Verify file saves,
   settings and repeat cold-boot persistence. A USB system disk can be the
   first target; native SD block access is a separate driver task.
3. Run the full ARM64 compiler, source tasks, startup and graphical desktop on
   Pi hardware. The current Pi payload is still the separate diagnostic/
   bootstrap build, without the normal linked `zc` runtime. Validate desktop,
   editor, applications, input focus and graphics using physical Pi drivers.
4. Package and test Pi installation/boot recovery, then converge the normal
   ARM64 kernel and release payload across Pi and UTM/QEMU with platform
   discovery. A shared ARM64 UEFI installer is a target, not a currently
   proven universal ISO; the Pi still needs its board firmware/UEFI bootstrap.
5. Close remaining upstream behavior/graphics/application differences and
   run sustained input/storage/desktop and reboot tests on both platforms.
   Full upstream ZealOS equivalence is separate from basic Pi bring-up.
   Ethernet/Wi-Fi/audio and other Pi hardware require their own drivers and
   acceptance if included in the desired finished system.


**MOUSE-BOOT-1 final shared regression (2026-10-26): passed.**
`make check-pi4-platform check-usb-hid check-usb-input check-usb-route`
completed with exit status 0. All four QEMU/HVF USB configurations passed:
NEC xHCI desktop/edit/save/reopen workflow, QEMU xHCI, NEC relative mouse,
and NEC USB alongside VirtIO recovery input. Each reported transfer/event-ring
cycling without errors; typing, buttons, pointer, focus/window recovery and
menu tiling passed. This is automated VM regression evidence, separate from
the physical Pi trackpad result above. The ordinary kernel in the test disk
was extracted and matched the final built kernel byte-for-byte by SHA-256
`0410d34fce025f8b68226723312cbe0c7694d633434e66f9f91a8690565b4eaf`.
The accepted Pi kernel remains `b4e7cdf1…1ef9605`. Retained logs:
`build/check-pi4-mouse-boot-1.log` and
`build/check-pi4-mouse-boot-1-build.log`. `git diff --check` passes.

The current physical USB input milestone is accepted for keyboard typing and
basic trackpad pointer/primary-button painting, on separate cold boots.
This does not complete the full Pi desktop, writable storage, installer or
upstream ZealOS equivalence. The reconnected microSD still contains the
accepted MOUSE-BOOT-1 image; no new payload was written after IMG_9151–9154.


**MULTI-USB2-1 candidate (2026-10-26):** extend the physically accepted USB2
hub/child path to keyboard, pointer and read-only storage together. This
first combined gate uses the SanDisk fixture in a **black USB2 port**, with
keyboard and trackpad in two other ports (USB2 devices may use blue ports).
A SuperSpeed stick in a blue port still selects the separately accepted
direct-SS storage path. Two or more connected hub children enter the new bus
session after the accepted hub configuration; a single child retains the
previous path. Up to four hub children have independent contexts, EP0,
interrupt/bulk rings and DMA buffers, using six permitted controller slots
(root plus four children with one spare). The DCBAA still fits its original
64-byte reservation; existing arena offsets stay unchanged. Additional child
regions follow the existing layout, with 8 KiB ring allocations and explicit
arena bounds. Unsupported child endpoints are not queued.

A cycle-aware command producer adopts the command position after Configure
Hub (entry 252, cycle 1) and invalidates the unused post-wrap prefix. Each
child is reset, assigned a separate slot and route/TT context, validated by
8/18-byte device and complete configuration reads, configured with readback,
and given the same boot-HID class requests or BOT class/endpoint validation
used in the single-device paths. EP0 TDs publish their first TRB last; their
bounded boot-time control rings do not wrap. FS EP0 MPS updates and output
endpoint readbacks precede traffic. HID queues start only after the known
file test succeeds if storage is present. Exactly one keyboard, one pointer
and at most one storage target are selected; report-mode multitouch is not
added by this candidate.

The retained controller has one event cursor. During synchronous BOT and
command waits, unrelated keyboard/pointer events are dispatched by slot,
endpoint and exact pending TRB before the waiter examines its own completion.
Each input endpoint has one outstanding TD and an independent producer. A
malformed owned report faults the session instead of being mistaken for a
storage result. Repeated known-file reads run every five seconds with input
live; an interleaved counter records HID completions delivered during those
reads. Stop/fault handling releases keys/buttons, halts xHC, verifies BME off,
and only then restores the inbound alias. DMA memory is never recycled.

Physical gate: cold-boot all three devices, type **help**, move/drag-paint,
and continue input during the five-second file checks. After about 55 seconds
run **usbstatus** and capture the bus/child summary and stroke. Expect
`usb2-multi-device-online`, active/stage/fault **1/0x0f/0**, kbd/mouse/disk
**1/1/1**, both CRC values **873b79ac**, checks at least **0x0b** (boot plus ten
live checks), input counters/wraps increasing and preferably interleaved >0.
Role 1=keyboard, 2=pointer, 3=storage; a configured child is at stage 13.
On failure, preserve last-port/rc/status/control and child stages. The media
remains read-only; hotplug/reconnection, writable system storage and the full
Pi compiler/desktop remain later gates. This candidate is not yet physically
accepted.


**MULTI-USB2-1 host validation (2026-10-26): passed.**
`make check-pi4-platform` completes with exit status 0 under AddressSanitizer,
UndefinedBehaviorSanitizer and `-Werror`. The new bus suite checks four
nonoverlapping, aligned child regions and exhaustion/overflow bounds; an
eager simulated consumer at every cache publication of 90 complete EP0 TDs;
control-ring capacity and unsupported OUT-data rejection; and 8192 mixed
keyboard, pointer, bulk and command events spanning both event cycles and
32 event-ring laps. Shared endpoint numbers on different slots do not route
storage completions to HID. Wrong HID TRB/status or too-short reports are
rejected. Keyboard press/release and pointer signed movement/buttons are
decoded during the mixed sequence. Existing cache/DMA, command/BOT wrap,
FAT32, keyboard and mouse suites still pass with the six-slot plan.

`make pi-sd` builds both targets. The ordinary QEMU/UTM kernel is byte-identical
to the previously passed four-configuration USB regression image (SHA-256
`0410d34fce025f8b68226723312cbe0c7694d633434e66f9f91a8690565b4eaf`), so
that unchanged binary's VM tests were not repeated. New Pi kernel SHA-256:
`c2907a94866b4b7d397d1538a8b66bffc38a8a9e436205c323d73631e7e3faa8`.
Script syntax and `git diff --check` pass. Logs are retained at
`build/check-pi4-multi-usb2-1.log` and
`build/check-pi4-multi-usb2-1-build.log`. Physical combined-device enumeration,
USB2 BOT file reads with input live, and interleaved completions remain
pending. Pure host routing/publication tests do not establish those physical
results.


**MULTI-USB2-1 card staging (2026-10-26): completed.** ZEALPI was freshly
identified by UUID `A5F20C1B-7A08-33D9-ABB7-0EDFC70E751E`, writable FAT32
identity and the physically accepted MOUSE-BOOT-1 kernel hash `b4e7cdf1…1ef9605`.
All five existing payloads were backed up, synced and SHA-256 verified in
`ZEALOS-BACKUP-20261026-MULTI-USB2-1/`, including old/new manifests. All five
active replacements were synced and read back byte-for-byte against
`build/pi-esp/`; the new kernel is
`c2907a94866b4b7d397d1538a8b66bffc38a8a9e436205c323d73631e7e3faa8`.
The other four payloads and Pi firmware/DTB are unchanged. Normal
`diskutil eject /dev/disk17` succeeded. The card is safely ejected and ready
for the all-three-devices cold boot, with the SanDisk fixture in a black
USB2 port. Physical combined-device acceptance remains pending.

**MULTI-USB2-1 physical result (2026-10-26): failed; IMG_9157/9158.**
The returned card matches kernel `c2907a94…e3faa8`. The bus reports
active/stage/fault **0/12/12**, connected mask **7**, three children,
kbd/mouse/disk **1/1/0**, no input reports and no storage checks. Port 1 / slot
2 is the `046d:c52b` boot keyboard; port 2 / slot 3 is the `05ac:0265` boot
pointer. Both reached configured stage **13**. Port 3 / slot 4 is the
High-Speed `0781:5580` SanDisk BOT device, stopped at stage **12**. Its last
command returned **rc=1**, completion **01000000**, control **04018801**.
That was initially interpreted as Configure Endpoint success followed by bad
readback. **Correction from IMG_9159/9160:** it was the previous Address Device
completion. USB2 bulk context construction rejected absent-companion sentinel
values before Configure Endpoint was ever submitted (details below). No storage CBW
or HID report TD was issued by this failed run. The common cleanup halted
xHC and restored the alias; reconnecting devices cannot recover this stopped
session. The user's observation that keyboard/trackpad did nothing agrees
with the all-or-nothing startup gate. Combined input/storage remains unaccepted.

**MULTI-USB2-2 candidate and host validation (2026-10-26):** preserve every
bulk readback condition and report a bitmask identifying arguments (bit 0),
Slot entries/state (1), endpoint states (2), types (3), packet sizes (4),
bursts (5), streams (6), dequeue pointers/DCS (7), and Average TRB Length (8).
Snapshot Slot[0..3], IN[0..4], OUT[0..4], expected endpoint/MPS/burst/ring
addresses and Configure command result immediately after the command.
Snapshots precede any slot retirement and survive it. No check is relaxed.

If this specific readback failure recurs after a successful Configure command,
retire only that storage slot: all its EP0 completions have been received,
no bulk TD has been published and its new endpoints are idle. A bounded
Disable Slot command must succeed for the exact slot before its DCBAA entry
is cleared and cleaned. The context/rings/buffers remain reserved. This follows
[xHCI 1.2b section 4.6.4](https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf).
Timeouts, transport failures, invalid geometry, a failed Disable Slot, missing
HID roles or ambiguous multiple storage targets still use the existing
halt/BME-off/alias restoration path. This is bootstrap failure isolation,
not hotplug/re-enumeration or general BOT error recovery.

A successful retirement permits both configured HID queues to start, with
an explicit **usb2-input-online-storage-disabled** result, bus active/stage/
fault **1/15/0**, kbd/mouse/disk **1/1/0**, storage child stage **16**, disabled
**1**, checks **0** and no storage reads. This is an input-only result, not
combined-device acceptance. Normal all-three success still requires both
known-file CRCs **873b79ac**, at least eleven checks and live HID input.

`make check-pi4-platform` passes under ASan/UBSan and `-Werror`. New regression
coverage constructs independent FS/HS/SS output contexts at both sizes,
injects each readback error group into both endpoints and checks combined
fault masks; corrupted contexts remain rejected. Slot retirement tests verify
command-before-DCBAA-clear publication, preservation of every other slot,
and no clear on command failure or wrong returned slot. Existing 8192 mixed
completion, control publication, ring wrap, FAT32 and HID tests pass.
`make pi-sd`, script syntax and `git diff --check` pass. The ordinary kernel
is unchanged at `0410d34f…65b4eaf`, matching the previously passed four QEMU/HVF
input configurations; those long VM tests were not repeated for this unchanged
binary. New Pi kernel SHA-256:
`1988684c908cb6b8857b368324d1d94b01f606f1c6be2c2cd1481271bdc3bfb4`.
Logs: `build/check-pi4-multi-usb2-2.log` and
`build/check-pi4-multi-usb2-2-build.log`. Physical failure isolation, simultaneous
keyboard/pointer input, and the storage mismatch diagnosis remain pending.

Next physical check: keep the same three devices/ports, with the SanDisk
fixture in a black USB2 port. Cold boot with all three attached. Type `help`
and `usbstatus`; move and primary-button drag-paint with the trackpad. Capture
the compact bus/child summary and all **USB bulk** lines. If storage is disabled,
no 55-second file-check wait is needed. If storage starts, keep input active
for the eleven-check gate. Do not unplug/replug during the test.

**MULTI-USB2-2 card staging (2026-10-26): verified and safely ejected.**
ZEALPI was freshly identified by volume UUID
`A5F20C1B-7A08-33D9-ABB7-0EDFC70E751E`, writable external FAT32 identity,
volume size and old `c2907a94…e3faa8` kernel hash. All five existing payloads
were backed up, synced and byte/SHA-256 verified in
`ZEALOS-BACKUP-20261026-MULTI-USB2-2/`; old/new manifests are retained there.
All five active files match `build/pi-esp/` byte-for-byte after sync. The
new kernel is `1988684c…dc3bfb4`; booter/demo/config/EFI hashes are unchanged.
Pi firmware/DTB and the USB stick were not changed. The staging record is
`build/pi-multi-usb2-2-stage.json`.

The initial normal eject was refused because macOS `loginwindow` (PID 694)
dissented. After the user unlocked the Mac, ZEALPI's UUID, external FAT32
identity, size, current parent disk and all five payload hashes/bytes were
freshly reverified. Normal `diskutil eject /dev/disk17` then succeeded. No
force eject was used. The staging record now reports `ejected: true`.
The card is ready for the MULTI-USB2-2 physical check described above;
combined-device input/storage acceptance remains pending.

**MULTI-USB2-2 physical result and diagnosis (2026-10-26): failed;
IMG_9159/9160.** The returned kernel matches `1988684c…dc3bfb4`. The bus again
stopped at **0/12/12**, mask **7**, three children, kbd/mouse/disk **1/1/0**.
The two HID children are configured, with no report traffic. SanDisk's
expected IN/OUT endpoint/MPS values are **81/200** and **02/200**, but both
reported burst values are **ffffffff**. Configure status/control and output
snapshots are still zero: construction failed before the Configure command,
so the readback-only slot-retirement fallback did not run. This was not a
controller readback mismatch. The earlier interpretation of the last
`04018801` event was incorrect: it was Address Device completion. The user's
inability to type or move agrees with the common startup abort.

Root cause: the speed-independent configuration parser correctly leaves
`bot_bulk_{in,out}_{max_burst,streams}` at `UINT32_MAX` when no SuperSpeed
Endpoint Companion descriptor is present. The USB2 bus code incorrectly
passed these sentinel burst sizes to `pi4_xhci_dma_bulk_contexts_seed`, which
correctly rejects any nonzero burst at Full-/High-Speed. The separate legacy
USB2 context path already passed zero. The new multi-device path omitted
that translation. No physical Configure Endpoint, BOT transfer or HID report
was reached for this storage child; zeroed snapshots were not real readback.

**MULTI-USB2-3 fix and host validation (2026-10-26): passed; hardware pending.**
A speed-aware BOT endpoint plan translates absent USB2 companion fields to
zero input/output burst sizes, using that same plan for context construction,
readback and expected-value reporting. Parsed descriptors retain their raw
sentinels. FS/HS packet limits, rejection of unexpected SS companions on USB2,
and SS companion/burst bounds with no streams remain enforced. SS absence
is never silently accepted as zero. No storage write support is added.

Setup now reports explicit substeps: **1** construct/plan, **2** submit
Configure Endpoint, **3** completed command/readback, **4** verified. Output
words are printed only if a completed Configure command was actually captured.
Slot retirement is explicitly limited to substep 3, with all previous idle/
completion preconditions; unknown failures still halt safely.

New regression coverage feeds complete 32-byte USB2 BOT descriptor chains
through the real parser, reproduces the old failure using its sentinel burst
values, then verifies plan-to-context construction and configured-output
validation with zero bursts. This covers Full-/High-Speed and both 32/64-byte
context formats. A 44-byte SS chain retains distinct bursts **7/15**, while
missing companions, streams, FS/512-byte packets and inappropriate companions
are rejected. The previous independent context-corruption, ordered retirement,
8192 interleaved event, control publication, ring wrap, FAT32 and HID tests
still pass. `make check-pi4-platform`, `make pi-sd`, script syntax and
`git diff --check` pass. Ordinary kernel SHA-256 remains
`0410d34fce025f8b68226723312cbe0c7694d633434e66f9f91a8690565b4eaf`,
so its unchanged four-configuration QEMU/HVF regression was not repeated.
New Pi kernel SHA-256:
`aac1b2ea47fd013da81636d54a9442d73726d180535eeb69b367844054324cc3`.
Logs: `build/check-pi4-multi-usb2-3.log` and
`build/check-pi4-multi-usb2-3-build.log`. This fixes the reproduced software
construction bug; simultaneous physical HID/BOT operation is not yet accepted.

Next board check retains the same three devices and ports, SanDisk in a black
USB2 port. Cold boot, type `help`, move/drag-paint and then `usbstatus`.
Capture the **MULTI-USB2-3** bus/child and **USB bulk** summaries. If all three
start, continue input for about 55 seconds: both file CRCs must be **873b79ac**,
checks at least **0b**, active/fault **1/0**, and HID counters increasing.
Do not hot-unplug/replug as a recovery attempt.

**MULTI-USB2-3 card staging (2026-10-26): verified and safely ejected.**
ZEALPI was freshly identified by UUID
`A5F20C1B-7A08-33D9-ABB7-0EDFC70E751E`, external writable FAT32 identity,
volume size and old kernel `1988684c…dc3bfb4`. The five previous payloads were
backed up, synced and byte/SHA-256 verified in
`ZEALOS-BACKUP-20261026-MULTI-USB2-3/`, with old/new manifests. All five active
files were synced and read back byte-for-byte against `build/pi-esp/`;
new kernel `aac1b2ea…4324cc3`, other four payload hashes unchanged. Pi
firmware/DTB and the USB stick were not changed. Before eject, the card
identity and all five active hashes/bytes were checked again. Normal
`diskutil eject /dev/disk19` succeeded; no force eject was used. Record:
`build/pi-multi-usb2-3-stage.json` (`ejected: true`). The card is ready for
the same-port, all-three-device cold boot described above. Physical acceptance
of that corrected build remains pending.

**MULTI-USB2-3 physical result (2026-10-26): endpoint configuration
accepted; storage read probe failed. IMG_9161/9162.** The card matches kernel
`aac1b2ea…4324cc3`. The bus reports active/stage/fault **0/13/13**, mask **7**,
three children and configured kbd/mouse/disk **1/1/1**, but no input reports,
file checks or CRCs. All three children reached stage **13**. SanDisk reports
setup-step **4**, readback errors **0**, disabled **0**, Configure completion
**01000000** / **04018801**. Both High-Speed bulk endpoints have MPS **512**,
burst **0**, running state **1** and the expected retained ring pointers;
Slot state is configured. This physically validates the USB2 sentinel-to-zero
fix and endpoint setup. The subsequent stage-13 INQUIRY/capacity/filesystem
probe failed, invoking common controller cleanup before HID queues started.
The report did not retain that probe's phase, so do not claim which individual
storage transaction failed or that any file read passed. The user's inability
to type/move agrees with that startup ordering; combined input is unaccepted.

**MULTI-USB2-4 diagnostic startup split and validation (2026-10-26):**
after all device configurations pass, start the keyboard and pointer queues
and retain the controller immediately. The storage endpoints stay configured
and idle, with explicit **pending=1/ready=0/checks=0** and result
**usb2-input-online-storage-pending**. Polling must not send storage traffic
while pending. This is a deliberate diagnostic split, not completion of the
normal combined-device startup or a hidden storage pass. The two-HID physical
gate can now be checked without waiting for an unrelated read test.

The Pi shell's **usbread** command runs that read-only probe once, with the
shared HID dispatcher already installed. It is allowed only on an active,
nonbusy multi-device bus with an untested configured storage target. Clear
pending before entry; retain exact INQUIRY phase/status/checks/actual/control/
pointers plus received bytes, CSW fields, the saved probe-state name and the
last capacity/sector/filesystem command. INQUIRY validation and storage media
checks remain strict. A successful known-file double-read sets ready **1**,
checks **1**, both CRCs **873b79ac**, and enables the five-second file checks.
Those periodic reads update the retained probe state as well.

A failed probe retains all diagnostics, releases input, halts the controller,
verifies BME off and restores the alias using the existing stop path. Ready
and pending are cleared on stop. No further usbread attempt may run on the
stale queues; cold boot restores the pending-input startup. This version
adds no BOT reset/disconnect recovery and no media writes. The earlier narrow
idle-slot retirement remains available only for post-Configure readback
failure, separate from transport errors.

The retained INQUIRY transaction is exercised with keyboard/pointer events
interleaved during all three wire phases. Tests verify the real CBW bytes,
phase ordering, successful CSW/identity, buffer bounds, retained corrupt
replies and immediate termination on each transport failure. Wrong tags,
residue, failed CSW status, wrong inquiry identity, short data/CSW, oversized
reported data and short CBW are rejected. Existing descriptor-to-context,
strict readback fault injection, slot retirement, 8192 mixed events, ring,
FAT32 and HID suites pass. `make check-pi4-platform` passes with ASan/UBSan
and `-Werror`; `make pi-sd`, script syntax and `git diff --check` pass. The
ordinary QEMU kernel remains byte-identical at `0410d34f…65b4eaf`; its four
previously passed VM configurations were not rerun. New Pi kernel SHA-256:
`938c0f4e4e1e78cb80be55f400a45e8b485ba39eb878b1be302045ffad9f322c`.
Logs: `build/check-pi4-multi-usb2-4.log` and
`build/check-pi4-multi-usb2-4-build.log`. Host models establish software
contracts, not physical simultaneous input or USB2 media reads.

Next physical check: cold boot the same three devices/ports (SanDisk in a
black USB2 port). First type **help**, move and primary-button drag-paint,
then type **usbstatus** and capture the input counters/stroke. Only then run
**usbread**; keep moving the trackpad during the probe. Capture the final
summary and **USB INQUIRY / USB read** lines, whether it passes or stops.
If passed, keep input active for about 55 seconds and capture checks >= **0b**,
both CRCs **873b79ac**, increasing HID counters and active/fault **1/0**.
If it stops, the captured phase identifies the next storage fix; cold boot
recovers input. No port swapping or unplug/replug is part of this test.

**MULTI-USB2-4 card staging (2026-10-26): verified and safely ejected.**
Fresh ZEALPI UUID `A5F20C1B-7A08-33D9-ABB7-0EDFC70E751E`, external
writable FAT32 identity, size and old `aac1b2ea…4324cc3` kernel were verified.
All five prior payloads were backed up, synced and byte/SHA-256 verified in
`ZEALOS-BACKUP-20261026-MULTI-USB2-4/`, with old/new manifests. All five active
files were synced and read back byte-for-byte against `build/pi-esp/`.
Kernel is `938c0f4e…d9f322c`; the booter/demo/config/EFI hashes are unchanged.
Pi firmware/DTB and the USB stick were not changed. A second identity and
all-five-payload verification preceded successful normal
`diskutil eject /dev/disk19`; no force eject was used. Record:
`build/pi-multi-usb2-4-stage.json` (`ejected: true`). The card is ready for
the input-first test, then **usbread**, described above. Physical acceptance
of simultaneous input and the remaining storage fix are pending.

**MULTI-USB2-4 physical result (2026-10-26): simultaneous input and
USB2 INQUIRY accepted; READ CAPACITY data-IN stalled. IMG_9163–9166.**
The user confirms keyboard and trackpad now work together. Photos show typed
`help`, `usbstatus` and `usbread`, active/stage/fault **1/0f/0** before the
read, connected mask **7**, three configured children, and kbd/mouse/disk
**1/1/1**. Logitech receiver **046d:c52b** is port/slot **1/2**, Full-Speed;
Apple trackpad **05ac:0265** is **2/3**, Full-Speed; SanDisk **0781:5580** is
**3/4**, High-Speed. This accepts basic combined typing/pointer operation
with storage configured but idle. It does not accept multitouch/secondary
click, hotplug, or combined media access.

The explicit `usbread` reaches INQUIRY step **7**, CSW/identity-ok **1/1**.
Its CBW/data/CSW phases all return **1**, status **01000000**, checks **1f**,
and actual lengths **31/36/13**. CSW signature/tag/residue/status are
**53425355/5a504901/0/0**. This independently accepts the SanDisk's USB2
INQUIRY transport/identity on the shared event ring with HID online.

The next READ CAPACITY(10), opcode/tag **25/5a504902**, fails at phase **2**
(data-IN), rc **fffffffe**, status **06000008**: Stall Error completion **6**
with all **8** requested bytes remaining. Event slot **4**, DCI **3**, pointer
and ownership checks match; checks **0f** omit only successful completion.
No CSW was received and no capacity bytes may be interpreted. Saved state is
**61/direct-root-capacity-failed**. The common failure path stops the bus
(active/stage/fault **0/0d/0d**), releases input, halts xHC, clears BME and
restores the inbound alias. This is a specific device DATA-IN stall, not an
event identity failure or another endpoint-construction failure. Its SCSI
cause is still unknown until a valid CSW and REQUEST SENSE are obtained.

**MULTI-USB2-5 implementation and validation (2026-10-26): bounded
High-Speed data-IN stall recovery.** The BOT transfer helper still requires
an owned, exact matching Transfer Event with Stall Error, valid residual,
IN direction, data phase, selected storage slot/DCI/ring and sane next
producer position. Only the active High-Speed USB2 storage child is eligible;
Full-Speed targets behind a transaction translator are excluded. This hook
is absent on the earlier standalone routes and is cleared on controller stop.
Timeouts, corrupt identities, CBW/CSW stalls and phase errors are not retried.

Recovery requires the output endpoint to be **Halted (2)**, then runs:

1. **Reset Endpoint**, TSP **0**, with exact slot/DCI command completion;
   verify **Stopped (3)**.
2. Device **CLEAR_FEATURE(ENDPOINT_HALT)** for the bulk-IN endpoint via EP0;
   verify the control completion and that bulk-IN remains stopped.
3. **Set TR Dequeue Pointer** beyond the failed one-TRB TD, with exact DCS
   and no stream; verify command completion, stopped state and pointer.
4. Publish/ring the **CSW** TD. The failed CBW/data are not reissued. Retain
   the original stall and partial byte count; a valid failed-command CSW can
   enter the existing bounded REQUEST SENSE/read-only retry path. No partial
   capacity bytes are accepted as a successful result.

This follows [xHCI 1.2b section 4.6.8](https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf)
and the [USB Mass Storage BOT data-IN stall rules](https://www.usb.org/sites/default/files/usbmassbulk_10.pdf).
At most **four** recoveries are allowed across a session. Failure at any
step, invalid endpoint state/readback or exhausted bound invokes common
controller cleanup; no later recovery action or CSW is submitted. HID
completion dispatch stays enabled throughout. The initial input-first,
manual **usbread** flow remains so physical input stays independently
testable. New diagnostics retain attempts/completions/stage/endpoint-state,
original stall/pointer/next/DCS, each reset/clear-halt/set-dequeue result and
parsed REQUEST SENSE fields. A modeled Unit Attention response is covered
by host tests, but is not yet claimed as this physical device's cause.

Host tests exercise the real recovery sequencer and command/control encoders
with the captured slot-4/DCI-3/residual-8 stall shape; abort each action in
turn; interleave keyboard/pointer completions during every action; verify
host/device toggle reset and no premature CSW; cover all **255** transfer
positions with both DCS values, including wrap, and reject corrupted identity,
residual, direction, pointer and producer state. Real CSW/SENSE/capacity
parsers and read-only CBWs cover failed-command status/residue, retryable
sense, new tags and phase-error rejection. Existing 8192 mixed-completion,
descriptor/context/readback, retirement, ring, FAT32 and HID suites pass.
`make check-pi4-platform` passes with ASan/UBSan and `-Werror`; `make pi-sd`
and script syntax pass. Pi kernel SHA-256:
`84864502da9af94cb5d65cc636cd33a08208360da5f2db1ec3533033cade15f4`.
Logs: `build/check-pi4-multi-usb2-5.log` and
`build/check-pi4-multi-usb2-5-build.log`. Ordinary kernel remains unchanged
at `0410d34fce025f8b68226723312cbe0c7694d633434e66f9f91a8690565b4eaf`;
its four previously passed QEMU/HVF configurations were not repeated.
Host models establish the software contract, not physical recovery acceptance.

Next board check: retain the same three devices and ports, SanDisk in a
**black USB2 port**. Cold boot, confirm **MULTI-USB2-5**, type **help**,
move/primary-drag-paint and run **usbstatus**. Then type **usbread**, moving
the trackpad while it runs. Capture **USB BOT stall/reset/clear-halt/
set-dequeue**, any **USB recovered sense**, and **USB read** lines. If the
known-file double read passes, keep input active for about **55 seconds**;
run **usbstatus** and capture checks >= **0b**, CRC1/CRC2 both **873b79ac**,
increasing HID counters and active/fault **1/0**. On failure, photograph the
retained results; cold boot restores input. No port swapping or unplug/replug
is part of this check. General disconnect/recovery, writable persistence and
the full Pi desktop remain separate work.

**MULTI-USB2-5 card staging (2026-10-26): verified and safely ejected.**
Fresh ZEALPI UUID `A5F20C1B-7A08-33D9-ABB7-0EDFC70E751E`, external
writable FAT32 identity, volume size and old `938c0f4e…d9f322c` kernel were
verified. The five old payloads were backed up, synced and byte/SHA-256
verified in `ZEALOS-BACKUP-20261026-MULTI-USB2-5/`, with old/new manifests.
All five new active files were synced and read back byte-for-byte against
`build/pi-esp/`; kernel `84864502…cade15f4`, other four hashes unchanged.
Pi firmware/DTB and the USB stick were not changed. A second identity and
all-five-payload verification preceded successful normal
`diskutil eject /dev/disk19`; no force eject was used. Record:
`build/pi-multi-usb2-5-stage.json` (`ejected: true`). The card is ready for
the same-port input check followed by **usbread**. Physical DATA-IN stall
recovery, USB2 capacity/file reads and sustained input alongside those reads
remain pending; simultaneous basic keyboard/pointer and USB2 INQUIRY are
accepted from the preceding M4 photos.

**MULTI-USB2-5 physical acceptance (2026-10-26): combined USB2 input,
stall recovery, capacity/FAT32/file reads and sustained checks passed.
IMG_9169/9170/9171/9172/9174/9175.** The returned card matches staged kernel
`84864502da9af94cb5d65cc636cd33a08208360da5f2db1ec3533033cade15f4`.
The same Logitech receiver **046d:c52b**, Apple trackpad **05ac:0265** and
SanDisk **0781:5580** occupy ports/slots **1/2**, **2/3**, **3/4**. All three
are configured; the user types `usbread` and `usbstatus`, input counters
increase, and the shared dispatcher records interleaved input during reads.

The captured READ CAPACITY stall is recovered exactly once: attempts/
completed/stage/EP-state **1/1/4/3**, actual/requested **0/8**, original
status **06000008**, control **04038001**. The failed IN TD ends at low
pointer **0444a020**, next dequeue **0444a030**, high word **4**, DCS **1**.
Reset Endpoint, clear-halt and Set TR Dequeue each return **1** with
completion status **01000000** and the expected command/control slot.
A valid failed-command CSW then reaches REQUEST SENSE; the actual parsed
sense key/ASC/ASCQ are **2/4/1** (NOT READY / logical unit becoming ready).
This establishes the physical cause; the prior Unit Attention host fixture
was only a model. The existing bounded delay/retry completes successfully.
No full BOT reset or media write was needed.

The explicit probe reports **direct-root-file-read-pass (66)**, INQUIRY
step **7**, pending/ready **0/1**, initial checks **1**, CRC1/CRC2 both
**873b79ac**. Capacity is **512-byte blocks**, last LBA **074de183**, matching
the fixture. Sector/FAT32/root discovery and the strict known **1025-byte**
`ZEALTEST.TXT` double-read/pattern/CRC checks pass. The session remains
active/stage/fault **1/0f/0**, connected mask **7**, kbd/mouse/disk **1/1/1**.
Later photos show checks **2**, **8**, then **0b** with the same two CRCs,
no fault, and continuing input. IMG_9174 includes the automatic message
**ten live file checks passed with input online**; IMG_9175 confirms the
final summary. Eleven checks mean the initial double-read plus ten periodic
checks, each scheduled five seconds apart. That meets the sustained gate
without relying on the user's stopwatch; the requested 55 seconds allowed
slack. It is not necessary to repeat this manual test just to time it again.

This accepts the manual combined USB2 startup/read gate and the narrow
High-Speed DATA-IN recovery. It does not accept hotplug/re-enumeration,
general BOT recovery, Apple multitouch/secondary click, writable persistence,
native SD storage or the full Pi graphical desktop.

**MULTI-USB2-6 startup promotion and validation (2026-10-26):** use the
same physically accepted probe automatically at the first healthy input
poll, after the controller/DMA arena are retained and both HID queues and
the shared event/stall dispatchers are installed. Drain existing HID events
and check controller health before entering the probe. Consume pending and
set busy/reading before the first wire phase; recursive polling cannot
resubmit INQUIRY. Success adopts the same live rings, sets ready and enables
the same five-second file checks. Any failure retains diagnostics and uses
common stop cleanup, without replaying bootstrap traffic on stale queues.
The initial pre-shell report may still show storage pending; the input loop
starts the automatic probe next. No manual `usbread` is required.

The shell retains `usbread` as a diagnostic entry for a genuinely untested
target. On an already verified active target it reports the accepted state
and existing periodic checks without a second bootstrap INQUIRY. New
**USB storage automatic/ready-seconds/checks/target** reports origin **1**
for automatic startup, time since the initial media check passed, check
count and target **0b**. Seconds use the architectural counter/frequency
and print in hex; the ten-check completion message remains the direct
sustained acceptance signal. No protocol/recovery limits or disk writes
were added by this startup promotion.

`make check-pi4-platform` passes with ASan/UBSan and `-Werror`, including
existing recovery failure ordering, both ring cycles/wrap, mixed HID/BOT,
FAT32 and input suites; `make pi-sd` and script syntax pass. The startup
policy reuses the accepted transport rather than changing its wire phases.
Its physical automatic-boot ordering remains to be checked. Pi kernel
SHA-256:
`6d39678f6a903951e43d665ff140b5f9295e561a6b802a5f443521e14f77da87`.
Logs: `build/check-pi4-multi-usb2-6.log` and
`build/check-pi4-multi-usb2-6-build.log`. Ordinary QEMU kernel remains
`0410d34fce025f8b68226723312cbe0c7694d633434e66f9f91a8690565b4eaf`;
its unchanged four previously passed QEMU/HVF configurations were not rerun.

Next physical check: cold boot the same three devices/ports, SanDisk in a
**black USB2 port**. Confirm **MULTI-USB2-6** and let the automatic storage
test finish, without typing `usbread`. Type **help**, move/drag-paint and run
**usbstatus**. Keep input active until **ten live file checks passed with
input online**, then capture **usbstatus**: automatic **1**, pending/ready
**0/1**, checks >= **0b**, both CRCs **873b79ac**, active/fault **1/0**.
If startup fails, photograph the saved probe/recovery/read lines. No port
swapping or unplug/replug is part of this acceptance check.

**MULTI-USB2-6 card staging (2026-10-26): verified and safely ejected.**
Fresh ZEALPI UUID `A5F20C1B-7A08-33D9-ABB7-0EDFC70E751E`, external
writable FAT32 identity, volume size and working M5 kernel `84864502…cade15f4`
were verified. All five previous payloads were backed up, synced and
byte/SHA-256 verified in `ZEALOS-BACKUP-20261026-MULTI-USB2-6/`, with
old/new manifests. All five new active payloads were synced and read back
byte-for-byte against `build/pi-esp/`; kernel `6d39678f…4f77da87`, other
four hashes unchanged. Pi firmware/DTB and the USB stick were not changed.
A second fresh card-identity and all-five-payload verification preceded
successful normal `diskutil eject /dev/disk19`; no force eject was used.
Record: `build/pi-multi-usb2-6-stage.json` (`ejected: true`). The card is
ready for the automatic-startup check above. The working manual M5 payload
is preserved in the backup; its combined input/read acceptance remains valid.

**MULTI-USB2-6 physical acceptance (2026-10-26): automatic combined
USB startup and sustained input/read operation passed. IMG_9176/9177.**
Returned card kernel SHA-256 is the staged
`6d39678f6a903951e43d665ff140b5f9295e561a6b802a5f443521e14f77da87`.
The photos show **MULTI-USB2-6**, storage automatic **1**, pending/ready
**0/1**, saved probe **66/direct-root-file-read-pass**, INQUIRY step **7**,
active/stage/fault **1/0f/0**, mask **7**, three configured children and
kbd/mouse/disk **1/1/1**. The same slot-4 bulk-IN stall recovery completes
**1/1/4/3**, with the same NOT READY/becoming-ready sense **2/4/1**.
Both CRCs remain **873b79ac**. The ten-live-file-checks message appears;
subsequent `usbstatus` reports reach checks **13** (19 decimal), with
nonzero interleaved input, increasing keyboard/pointer counters and no fault.
No manual `usbread` was needed. The architectural-counter elapsed field also
advances. This closes the automatic combined USB2 startup/sustained gate,
in addition to the earlier manual gate. Do not repeat the same USB soak as
another milestone; next work is integration with the native runtime.

**PI-DESKTOP-1 native integration candidate (2026-10-26):** add a distinct
`kernel-pi-desktop.elf` built with **ZEAL_FORCE_PI4**, retaining the native
ZealC/compiler/runtime objects that **ZEAL_PI_DIAG** deliberately excludes.
The accepted Pi framebuffer/UART, FDT/PCIe/DMA/xHCI, GICv2 and timer path
continues to initialize the hardware. After its GIC check, retain IRQs-off
polled operation, run the accepted automatic combined USB probe and require
working keyboard, pointer and verified storage before native desktop startup.
Missing source archive or a failed combined hardware gate leaves the native
shell available with an explicit message rather than entering graphics.

The ordinary runtime's existing source-reader/key-reader/boot-media,
`zc_init`, source graphics, WinMgr, graphical terminal and HomeSys/Once
startup are extracted into one inlined shared startup helper. Pi input
capability/poll/sample/key-reset paths now include **ZEAL_FORCE_PI4** as well
as the diagnostic macro, so source-backed tasks receive the same accepted
native input events. The full Pi shell retains **usbstatus/usbread/usbstop**.
USB diagnostics typed in a graphical terminal use the normal captured output
path; unsolicited framebuffer text respects desktop ownership. The USB wire
protocol, rings, timeout/recovery limits and known-file checks are unchanged.

`make pi-desktop-sd` prepares **build/pi-desktop-esp/** with seven files:
`boot/kernel.elf` (native), `boot/kernel-diag.elf` (M6 recovery),
`boot/zealbooter.elf`, `boot/demo.hcbc`, `boot/sources.tar`,
`EFI/BOOT/BOOTAA64.EFI`, and `limine.conf`. The source archive is supplied as
the third Limine module, using the existing booter/handoff support; sources
remain alive throughout the session. It does not depend on VirtIO or on
adding program files to the USB fixture. The first Limine entry defaults to
the desktop after **five seconds**; the second selects diagnostic recovery.
Pi UEFI/firmware/DTB stay unchanged. The Pi booter remains byte-identical at
`0b62747a…c4fdfc39`; the recovery kernel remains byte-identical to the
physically accepted M6 image. The native source-writer callback is disabled
on Pi; no writable storage or installer acceptance is implied by this gate.

The staged-image checker validates AArch64 PIE identity, executable entry,
load/file bounds and loader-supported relative relocations; distinguishes
the native compiler+Pi-input image from diagnostic recovery; checks both
Pi/DTB/ordered-module boot entries; and verifies required modules in the
**246-file** source archive. Negative checks reject a diagnostic image
substituted for the desktop kernel, omitted third module and missing archive.
A new DMA reservation fixture covers a **256 MiB** kernel span occupying the
former arena address, a separate source module and page-table reservation:
the chosen DMA arena must remain disjoint from all three. Platform tests
pass with ASan/UBSan and `-Werror`; script syntax, image checks and the full
native/diagnostic build pass. Existing nonfatal compiler warnings remain.
The physical Pi native compiler/desktop is still unaccepted until board boot.

Native Pi kernel SHA-256:
`7359ad8cd604e5de8dee0739fd496f9c17a5c778075aae2e0a07f8faa983ee5d`.
Source archive SHA-256:
`9a24342ac7dab77b0e99b48bd91ff6c5b92692c56fd4d3961ba322f53e68bb8a`.
Boot config SHA-256:
`05902f996cf77bcad5ae2394814434136f83adc907c465fdf2f38cd633fa5bd7`.
The ordinary kernel changes to
`e9a2cdf6d16383029fb4952a0f308bd501fefc4b249c4740e4e9f4374854d572`
because of the shared startup extraction. Its QEMU/HVF PCI desktop workflow
regression passes: visible keyboard/cursor/history routing, compiler option
and error recovery, ExeFile/RunFile, task/server/popup lifetimes, source
exceptions, editor save/compile/run, cold reboot/reopen, live Lattice controls
and both demo returns to the terminal. This verifies the shared runtime;
it does not emulate the physical BCM2711/VL805 path or accept Pi graphics. Logs: `build/check-pi-desktop-1-build.log`,
`build/check-pi-desktop-1-platform.log`, `build/check-pi-desktop-1-image.log`,
`build/check-pi-desktop-1-image-rejections.log`, and
`build/check-pi-desktop-1-virt-regression.log`.

Next physical check is the **desktop**, not another diagnostic-only soak.
Keep the same three devices/ports, SanDisk in a black USB2 port. Cold boot
the default **PI-DESKTOP-1** entry and capture any progress/error text or the
resulting graphical terminals and centered animated Z. In a guest terminal,
run **40+2;** (expect **42**) and **Dir;**, move the pointer and click the
menus; **usbstatus** can confirm the retained USB session. If it stalls,
photograph the last **PI-DESKTOP-1/zc/USB** lines. The second Limine entry
boots the accepted M6 diagnostic recovery image. Files remain read-only;
persistence, disconnect recovery, native SD and Pi installation remain work.

**PI-DESKTOP-1 card staging (2026-10-26): verified and safely ejected.**
Fresh ZEALPI UUID `A5F20C1B-7A08-33D9-ABB7-0EDFC70E751E`, external
writable FAT32 identity/size and accepted M6 kernel/config hashes were
verified. Existing five payloads were backed up, synced and byte/SHA-256
verified in `ZEALOS-BACKUP-20261026-PI-DESKTOP-1/`; the manifest records
`boot/kernel-diag.elf` and `boot/sources.tar` as previously absent. Seven
new payloads were synced and read back byte-for-byte against
`build/pi-desktop-esp/`. The menu was published after both kernels and the
source module were present. Active native kernel is `7359ad8c…a983ee5d`;
recovery is the byte-identical accepted M6 image `6d39678f…4f77da87`.
The booter/demo/EFI hashes remain unchanged. `RPI_EFI.fd` and the Pi DTB
were hashed before/after and verified unchanged; the USB stick was not written.
The staged-image checker also passed directly against the seven card files.
A final fresh identity, all-seven-byte/hash and firmware/DTB verification
preceded successful normal `diskutil eject /dev/disk19`. No force eject.
Record: `build/pi-desktop-1-stage.json` (`ejected: true`); complete host
payload hashes/sizes: `build/pi-desktop-1-payload.json`. The card defaults to
the **PI-DESKTOP-1** desktop entry after five seconds, with **MULTI-USB2-6**
diagnostic recovery as its second entry. Pi desktop execution/input/rendering
remain pending the physical check above; automatic diagnostic USB startup is
already accepted and does not need another identical soak.

**PI-DESKTOP-1 physical result (2026-10-26): native source startup ran,
but the desktop/input gate failed. IMG_9180.** The photo shows terminal
welcome text, directory listings and HomeSys/Once Tips of the Day, so native
ZealC/source execution progressed beyond the diagnostic shell. Text overlaps
instead of showing the finished compositor, and the user reports no typing
or pointer response with the screen unchanged for more than two minutes.
The last visible queued startup command includes ACInit; this does **not**
establish that ACInit itself is the blocked operation. Native Pi desktop
rendering/input are not accepted by this result.

**PI-DESKTOP-2 framebuffer adaptation candidate (2026-10-26):** the source
compositor and its host frame buffers require a packed **800×600**, pitch
**3200** surface. `guest_task_frame_run` passes exactly those stride/row
bounds, while `zeal_fb_task_frame_begin` rejects a larger framebuffer; other
native display bridges also require that size/pitch. Requesting a mode in
Limine does not make an incompatible firmware framebuffer usable by those
fixed surfaces. This is a confirmed software gap and a probable explanation
for the unfinished display; the physical mode/pitch in IMG_9180 has not yet
been measured, and this is not proof of the sole hardware failure cause.

After the accepted automatic combined USB gate, the full Pi image now keeps
a packed **800×600** RAM desktop surface and separately retains the physical
scanout address, dimensions and pitch. A validated presenter centers and
integer-scales the logical surface into a supported 32-bit firmware mode,
preserves row padding, clears borders once, and cleans CPU-written scanout
cache lines. Source graphics and pointer coordinates use the logical bounds;
the initial pointer is centered there. Unsupported modes leave an explicit
shell message instead of entering native graphics. Startup and `usbstatus`
report physical width/height/pitch/scale/composed frames and logical bounds.
No ACInit, HID protocol, storage protocol or ring/recovery policy changed.

`make check-pi4-platform` passes with ASan/UBSan and `-Werror`, including
new tests that verify every pixel, border, padded scanline and allocation
guard for 800×600, 1024×768, 1920×1080, 1920×1200 and 3840×2160 modes
(scales 1/2/3), plus invalid-mode rejection. Existing DMA, xHCI, mixed HID/BOT,
FAT32 and input suites pass. `make pi-desktop-sd`, staged-image validation,
shell-script syntax and `git diff --check` pass. Logs:
`build/check-pi-desktop-2-platform.log`, `build/check-pi-desktop-2-build.log`
and `build/check-pi-desktop-2-image.log`. The ordinary QEMU kernel remains
byte-identical `e9a2cdf6…854d572` and the accepted diagnostic recovery remains
byte-identical `6d39678f…4f77da87`; their previous tests/physical evidence
remain valid, and the unchanged QEMU workflow was not rerun for this Pi-only
presenter. Physical presentation and desktop/input acceptance remain pending.

Native Pi kernel SHA-256:
`b12497c8e4df1998966b2a46d4f1ed7c31363090c3a1363df0a6183993d4930b`.
Boot config SHA-256:
`a8981abc3cda174982db0e510a1bce180b2cefa74973789cd49c481ef92b5bc4`.
The source archive, booter, demo and EFI payload are unchanged from
PI-DESKTOP-1. USB media and native source writes remain disabled/read-only.

Next physical check: cold boot the default **PI-DESKTOP-2** entry with the
same three devices in the same ports, SanDisk in a black USB2 port. Expect
a centered/scaled desktop with borders on larger modes, graphical terminals
and animated Z. Check pointer movement/menu clicks and type **40+2;** and
**Dir;**. Capture **usbstatus**, especially the physical mode/pitch/scale and
nonzero composed-frame count. If it stops again, photograph the final
startup screen rather than waiting indefinitely. The second Limine entry
continues to boot accepted **MULTI-USB2-6** diagnostic recovery.

**PI-DESKTOP-2 card staging (2026-10-26): verified and safely ejected.**
Fresh ZEALPI UUID `A5F20C1B-7A08-33D9-ABB7-0EDFC70E751E`, external
writable FAT32 identity/size and returned PI-DESKTOP-1 kernel/config hashes
were verified. All seven prior payloads were backed up, synced and read back
in `ZEALOS-BACKUP-20261026-PI-DESKTOP-2/`, with old/new manifests. All seven
active payloads were synced and byte/SHA-256 verified against
`build/pi-desktop-esp/`; the boot menu was published last. Native kernel is
`b12497c8…93d4930b`, config `a8981abc…92b5bc4`; accepted M6 recovery and the
other five payload hashes are unchanged. `RPI_EFI.fd` and the Pi DTB were
hashed before/after and verified unchanged; the USB stick was not written.
The image checker passed directly against the card. A final fresh identity,
all-seven-byte/hash, backup and firmware verification preceded successful
normal `diskutil eject /dev/disk19`; no force eject was used. Record:
`build/pi-desktop-2-stage.json` (`final_verified: true`, `ejected: true`);
payload sizes/hashes: `build/pi-desktop-2-payload.json`. Physical desktop
presentation, responsiveness and the cause of the PI-DESKTOP-1 stall remain
to be established by the board test above.

**PI-DESKTOP-2 physical presentation / terminal milestone (2026-10-26):
IMG_9182, user reports “it seems to work.”** The photograph shows coherent
native graphical terminal borders/text and executed `usbstatus`, with another
terminal visible to the right. The on-screen PI-DESKTOP-2 status reports
physical width/height/pitch/scale/frames **0x780/0x500/0x1e00/2/0x660**:
**1920×1280**, pitch **7680**, scale **2**, **1632 composed frames**. Logical
surface remains **800×600**, pitch **3200**. This confirms that the board
actually supplies a larger firmware mode and that the packed-surface adapter
is presenting native compositor output; the earlier mode mismatch was real.
This result does not isolate every possible cause of the PI-DESKTOP-1 stall.

The interactive status command is visible, and the USB session reports
active/fault/stopped **1/0/0**, successful INQUIRY/read CSWs, bulk errors and
disabled **0/0**, and checks **1f** (31 decimal). Logitech keyboard and Apple
pointer reports are nonzero; the pointer movement counter is nonzero. Thus
the retained USB input/storage path continues operating alongside the native
compositor. CRC lines and the full desktop/splash are outside this photo;
no new CRC, splash-animation or persistence claim is made from it.

This closes physical framebuffer adaptation and an interactive native
terminal/status command. The user’s broad success report is encouraging,
but explicit confirmation of visible cursor movement/menu clicks and
**40+2;** returning **42** is still needed before closing that desktop/input
gate. No new image or identical diagnostic-only USB soak is required for
those checks. Writes, disconnect recovery, native SD and installation on Pi
remain separate future work. Candidate identity is the visible PI-DESKTOP-2
marker and the previously verified staged image; no returned-card hash was
read during this live photo check.

**PI-DESKTOP-2 native desktop / basic input accepted (2026-10-26).**
After IMG_9182, the user explicitly answered **yes** to all three requested
checks: visible cursor movement, working menu clicks, and **40+2;** returning
**42** in the native terminal. Together with the photograph's coherent
native terminal/compositor, typed `usbstatus`, 1632 composed frames and
retained USB active/fault/stopped **1/0/0**, this closes the stalled-startup
follow-up and the basic physical Pi native desktop/input milestone. The
accepted native image is staged kernel SHA-256
`b12497c8e4df1998966b2a46d4f1ed7c31363090c3a1363df0a6183993d4930b`,
with the accepted M6 diagnostic recovery kernel preserved as the second boot
entry. Cursor/menu/arithmetic acceptance is user-observed; the supplied
photo independently captures compositor/status output, not those actions.

Do not require another identical flash or diagnostic soak to re-prove this
gate. No code or boot payload changed for this acceptance update. Remaining
work includes broader source-program/editor workflows on physical Pi,
explicit full-desktop/splash-animation checks, writable source storage and
save/reboot/reopen persistence, native SD support, safe disconnect/reconnect
recovery, and a Pi-compatible installation path. This milestone does not
claim complete ZealOS parity, Apple multitouch/secondary click, or a single
ISO that installs on both UTM and Pi.

9. **Storage / display** — after serial input and native USB input are usable.

## Non-goals (for now)

- Apple Silicon / SystemReady PC (decide after Pi serial).
- Replacing Limine on Pi with bare `kernel8.img`.
- Claiming full ZealOS parity on either target.
