# Bench run sheet, Paper 3 retrofit (prepared 2026-11-12)

The ordered session for telesart. Full detail lives in
BUILD-STATUS.md; this sheet is the at-the-bench checklist.

## Task 1. Push the stack: DONE 2026-11-12

Iscandar-ram pushed through 72c6a0c (build stack plus the
2026-11-12 corrections: net served 130,988; Fubuki version of
record 6.12.107). Overleaf sync of the same corrections the
same day (project 6af5061f18a2c07e6eff5e68).

## Task 2. Entry check on telesart

- Clone https://github.com/IronAutie/Iscandar-ram (or pull).
- In `build/`, run `./link-check.sh`. Eleven of eleven PASS
  is the gate; anything less stops the day here.

## Task 3. Prepare the Linux 6.12.116 tree

Artifact of record (kernel.org, released 2026-11-06):

- Tarball: linux-6.12.116.tar.xz, 148,733,020 bytes
- URL: https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.12.116.tar.xz
- Published SHA-256 (kernel.org sha256sums.asc):
  cb3ed3dfdd47e6ade49dc933db334fd86e24cadee920da612c262b0e9b6a511b

Download, verify the hash before unpacking, and record the
tree's origin and hash in BUILD-STATUS.md's successor note.
The Debian alternative (linux-source-6.12, 6.12.116-1, trixie
security) carries the Debian patch stack; the pristine
upstream tarball above is the tree of record, matching the
tree the sift, reconciliation, and dye were read against.

## Task 4. Rebase the patches

Apply 0005 first, then 0001 to 0004. Copy
`build/bridge/iscandar_bridge.h` into the tree as
`include/linux/iscandar.h` verbatim; never retype a constant.
Resolve each center at its patch-point line against the real
tree. The percpu chunk question and the vfree_atomic question
are answered here, by the tree.

## Task 5. Build the kernel

With the Fubuki config of record: `telezart-kconfig` in
~/telezart/elbe-build-20261103-204902/ on telesart,
SHA-256
9b460f1003db4d417288e9c14caecfa14245dab07186df948fef0f3661e38ceb
(the 6.12.107 build config, byte-checked 2026-11-12).

## Task 6. Bind the arena and the channel

Reserve both spans at boot per compile-time-map.md; set the
arena base to the arena span's physical start; bind the
layer's serve path to the reserved channel region; confirm
the raise registers the prefix-born caches from the prefix
counters.

## Task 7. Stage the payload, submit ELBE

`make` in `build/`; copy `build/bin/iscandar-layer` and
`build/bin/iscandar-exerciser` into
`~/telezart/share/iscandar-payload/`. Submit by absolute path:
`elbe initvm submit /home/andrew/telezart/fubuki-iscandar.xml`
(the patched XML placed at that path), and build the image.
The gen_pool served set is named here, per pool, as pools
register.

## Task 8. Boot the evidence run

QEMU at `-m 512`, patched kernel and initrd handed in by
`-kernel` and `-initrd`, trace armed (`trace_pipe_arm`),
`iscandar_channel=` and `iscandar_arena=` naming the reserved
regions, 9p share up for payload and exports, serial console
on ttyS0 as the console of record, exerciser running as the
boot application.

## Task 9. Read the evidence

Exported `iscandar-results` against the prediction sheets.
The Discussion's verdict item takes its answer from the
criterion fixed in advance: is this a direct replacement,
and does it run without a panic. Then the run-dependent
manuscript sections (Abstract, Measurements, Conclusion)
are written from the run's numbers.
