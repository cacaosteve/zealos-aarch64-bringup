# Pi 4 writable storage plan

## Current state

- The Pi desktop build boots from the firmware-provided FAT boot volume and
  bundles the ZealOS source tree in `boot/sources.tar`.
- The Pi desktop has a manual `psdread` diagnostic and explicit `psdopen`
  command. `psdread` initializes the eMMC2 SDHCI controller, identifies the
  card, reads sector 0, and scans the primary partition table without writes.
  `psdopen` validates and binds the unique ZealOS data partition to the normal
  bounded block backend; it never runs at startup or formats storage. The PIO
  sector write and flush path has host simulation coverage but still needs
  physical Pi/card validation before routine use.
- SDHCI clock, command, paired-register, R2 response, and CSD-capacity helpers
  have host tests. The driver derives the identification-clock divider from
  the advertised SDHCI base clock. These tests validate calculations, not the
  electrical/controller behavior on a Pi.
- The `phdmi` diagnostic now also asks Pi firmware for the measured eMMC2
  clock rate. This is diagnostic groundwork only; it does not touch the
  controller or card.
- Pi desktop startup leaves the SD backend unbound. The source writer can use
  the SD partition only after the user explicitly runs `psdopen`.
- A platform-neutral block backend now owns RedSea geometry, absolute-sector
  I/O, source-bank ranges, and flush behavior. UTM binds its PCI VirtIO disk to
  that interface. Raw-disk read/write checks and a source-store cold-boot
  persistence check pass against disposable cloned UTM disks.
- A host-testable, bounds-checked MBR/GPT primary-partition scanner now
  validates partition extents, rejects overlaps and extended MBR layouts, and
  verifies GPT header and entry-table CRCs. Its fixture test is included in
  `make check-pi4-platform`. The manual `psdread` diagnostic now passes
  sector reads to this scanner; the result still needs validation on the
  physical Pi.
- The block backend can now discover the unique ZealOS data partition (MBR
  type `0xDA` or the ZealOS GPT type GUID) and expose it as a partition-relative
  disk. The planned layout reserves two 32 MiB append-only source banks at its
  end and maps the preceding sectors as the RedSea region. All generic block
  reads and writes are translated through the partition start and kept within
  its length. UTM continues to expose its existing whole-disk layout. The Pi
  kernel now binds to this backend only after an explicit `psdopen` command.
  A host test now exercises RedSea/source-bank reads and writes through the
  translated backend, rejects out-of-range writes, and verifies the MBR and
  FAT boot partition remain unchanged. This checks the memory-mapped block
  layer, not physical SD card behavior.
- The current bootstrap RedSea writer is not the full ZealOS filesystem: its
  formatter is limited to 128 sectors, its root-file writer is limited to
  64 KiB and eight root entries, and it does not provide stock directory,
  rename, or general file-write behavior. The partition mapping alone does not
  make this a normal writable ZealOS installation. Full RedSea integration is
  a separate requirement before claiming ordinary persistent file use.
- The bounded SDHCI backend uses conservative single-sector CMD17/CMD24 PIO
  operations with capacity checks and CMD13/DAT-busy flush confirmation.
  Multi-sector backend requests are split into single-sector commands. These
  paths have a fake-controller read/write/flush test but have not yet been
  validated against a physical Pi SD controller. `psdopen` is the only Pi
  command that binds this backend, and writes stay inside the MBR type `0xDA`
  or ZealOS GPT data partition.
- The inspected `ZEALPI` card uses an MBR with a single FAT32 partition
  spanning the card. There is no data partition or free tail to adopt as-is.

## Target layout and install flow

The current bring-up setup is an image-flash workflow, not the UTM ISO
installer. `scripts/build-pi-image.py` builds a Pi-specific whole-card raw
image on the host from a copied pftf boot-volume directory plus
`build/pi-desktop-esp`. Raspberry Pi Imager then writes that image to the
microSD. The builder does not touch a physical card.

The image has two MBR partitions:

1. A 512 MiB FAT32 partition (type `0xEF`) containing Pi firmware/UEFI boot
   files and the ZealOS boot payload.
2. A ZealOS data partition (type `0xDA`) occupying the rest of the image. Its
   first region is reserved for RedSea; two 32 MiB append-only source banks
   live at the end. The builder initializes an empty full-size RedSea volume
   and the source-bank headers. These are regions inside one data partition, not
   additional partitions. The current builder emits MBR, although the runtime
   scanner also recognizes the ZealOS GPT type GUID.

For this development image, boot the card and explicitly run `psdopen` to
initialize the SD controller and bind the unique ZealOS data partition. Only
then can generic disk/source commands reach it. The image builder formats the
RedSea area as an empty stock-format volume. Do not run `rspersist` on it:
`rspersist` is only for the 128-sector bring-up format. The runtime has bounded
RedSea reads and source-store persistence, but ordinary full-size RedSea file
create/replace/delete operations are not yet enabled. The physical Pi
write/readback/reboot persistence path still needs acceptance. The FAT boot
partition is outside the block-device write range.

The builder requires `mkfs.fat`, `mcopy`, and `mmd` (Homebrew `dosfstools` and
`mtools` on macOS). Example, after copying the current FAT boot volume to a
directory and staging the Pi payload:

```sh
make pi-image PI_FIRMWARE_ROOT=/path/to/copied-pftf-boot PI_IMAGE_SIZE_MIB=8192
```

It writes `build/zealos-pi4b.img`; it never writes to a card. Raspberry Pi
Imager can flash the resulting `.img` to a card at least as large as the image.
Flashing replaces the card's partition table and contents, so this is a fresh
install operation rather than an in-place upgrade. Keep a backup before
flashing a card with files you need.

Do not use the existing UTM ISO as if it were a Pi optical disc. Pi firmware
boots firmware-readable FAT partitions from SD or USB mass storage; it does
not boot an ISO as a CD device. A future USB installer could boot from a
mass-storage image and install to a separate target, but it needs a Pi
bootable installer environment. The first release is simpler: flash a raw
Pi `.img` with Raspberry Pi Imager. UTM continues to boot the ISO and run
`zinstall ERASE` against its VirtIO disk.

The image layout is not yet a finished end-user installer: it is a reproducible
bring-up image with manual `psdopen` and incomplete ordinary file-write support.
A later installer could offer either microSD or USB mass storage as the
destination, but the current native writable backend targets the Pi SD
controller and does not yet provide general writable USB-storage support.

Do not resize or repartition the user's existing card in place as part of
development. The current full-card FAT32 layout has no safe place for the
data partition. A future migration utility would need a verified full-card
backup and explicit confirmation before changing that layout.

## Implementation sequence

1. Define the bounded block-device interface, partition scanner, and
   partition-relative volume binding. The Pi SD backend now plugs into that
   interface only through explicit `psdopen`; UTM remains on VirtIO. Host
   checks cover both partition discovery and the partition-bounded block I/O.
2. Implement BCM2711 eMMC2/SDHCI initialization, identification, bounded PIO
   reads/writes, and flush status. Host tests cover helper logic and fake
   controller I/O. Run `psdread` and then `psdopen` on a Pi image with the new
   partition layout to validate physical controller behavior.
3. The image builder now initializes the empty full-size RedSea volume and
   both source-bank headers. Validate `zput`/`zload disk:` and cold-boot
   persistence on Pi. The boot partition remains outside every writable
   ZealOS range.
4. Port/connect stock RedSea formatting and normal filesystem operations.
   The raw-image builder now creates a valid empty full-size RedSea volume;
   the runtime still needs stock-compatible create, replace, delete, and
   rename operations. Keep bootstrap raw writes separate from normal
   filesystem APIs.
5. Keep the raw-image builder and source the supported Pi firmware package in
   a repeatable way. The builder lays out both partitions, builds the FAT boot
   volume from a copied firmware directory and staged payload, initializes the
   source banks, and formats an empty RedSea data region in the output image.
   It does not flash a card or initialize SD.
6. Validate on disposable images first, then on a backed-up microSD: boot,
   read files, write and reread a test source, cold-reboot, and verify it
   persists. Finally run representative apps and audio from the writable
   installation.

## Acceptance boundary

Writable storage is complete only when Pi ZealOS can discover the intended
data partition, use its RedSea/source files through the regular APIs, survive
a cold reboot with saved data intact, and leave the firmware FAT partition
unchanged. A successful SDHCI register probe or a host-built image alone does
not meet that bar.
