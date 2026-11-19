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

DONE 2026-11-17: eleven of eleven PASS on telesart. Two
bench notes from the run: (1) telesart's engine checkout is
`~/iscandar`, so the check runs as
`ENGINE_REPO=$HOME/iscandar ./link-check.sh` (the Makefile
default names this VM's layout). (2) On a fresh clone the
first run fails conf-idempotent because
`constants/iscandar-build.conf` is generated and gitignored;
that first run generates it, and the immediate re-run passes.

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

DONE 2026-11-17: tarball SHA-256 verified against the
published sum and unpacked to ~/linux-6.12.116 on telesart.

## Task 4. Rebase the patches

Apply 0005 first, then 0001 to 0004. Copy
`build/bridge/iscandar_bridge.h` into the tree as
`include/iscandar_bridge.h` verbatim; never retype a constant.
(`include/linux/iscandar.h` is the glue header 0005 creates;
the shared header does not sit there. Ruled 2026-11-17.)
Resolve each center at its patch-point line against the real
tree. The percpu chunk question and the vfree_atomic question
are answered here, by the tree.

DONE 2026-11-17: 0005 applied first (its mm/Makefile hunk
failed and was hand-fixed; the bridge object line stands
exactly once), rebased 0001 to 0004 applied clean with
zero rejects, shared header copied to
include/iscandar_bridge.h per the header ruling, and
`#include <linux/iscandar.h>` stands exactly once in each
of the five target files. The percpu center is served
under the uniprocessor precondition (see Task 5) and the
vfree door was fixed as ruled; both are recorded in the
rebase report, section 9.

## Task 5. Build the kernel

DONE 2026-11-17: config of record byte-checked at the
bench (SHA-256 matches), copied to .config, SMP disabled
and verified (# CONFIG_SMP is not set), make olddefconfig
completed (one benign warning: .config:9970 symbol value
'm' invalid for ANDROID_BINDER_IPC). All six patched
objects compiled clean (mm/iscandar_bridge.o, mm/slub.o,
mm/slab_common.o, mm/vmalloc.o, mm/percpu.o,
lib/genalloc.o). Full build finished exit=0; bzImage
verified at the bench: 10,338,816 bytes, Nov 17 16:26.

With the Fubuki config of record: `telezart-kconfig` in
~/telezart/elbe-build-20261103-204902/ on telesart,
SHA-256
9b460f1003db4d417288e9c14caecfa14245dab07186df948fef0f3661e38ceb
(the 6.12.107 build config, byte-checked 2026-11-12).
One ruled derivative (2026-11-17): the build is
uniprocessor. Disable SMP on this config
(`scripts/config --disable SMP`): center 4.2 is served
under the build's uniprocessor precondition, and the
paper states that scope. The config of record remains
the byte-checked file above; the derivative is this
one switch.

## Task 6. Bind the arena and the channel

Reserve both spans at boot per compile-time-map.md; set the
arena base to the arena span's physical start; bind the
layer's serve path to the reserved channel region; confirm
the raise registers the prefix-born caches from the prefix
counters.

State found 2026-11-17, before any boot: this task is real
desk work, not a boot line. The bridge core as applied is
missing its binding body: nothing assigns iscandar_ctrl
(no mapping of the channel region), the arena kernel
mapping is never made, and /proc/iscandar_bridge exists
only as the closing comment's outline (the design always
said the proc body is written at application time). The
kernel links and boots without it because the prefix
posture is silent by construction: ctrl NULL, ready false,
every center stock-served. The layer likewise still serves
the rig's channel file; serving the reserved region (and
not zeroing the kernel's prefix counters and log when it
attaches) is unwritten. So the boot waits on: (1) the core
body: early mapping of both spans, the proc file with the
raise write and the state read, the raise registering the
prefix-born caches; (2) the layer's region mode; (3) the
span choices and memmap reservation for the boot line.
Then Task 8.

Drafted 2026-11-17 in build/bridge/TASK6-BINDING-DRAFT.md.
Decision 1 RULED the same day: (B), the guest goes to
-m 1024 and the arena span is the second 512 MiB at phys
0x20000000, channel 512 KiB at 0x1FFE0000; every settled
figure stands; efficiency deferred by Andrew's word.
Open point RULED the same day: measure the violations
first, fix next round from the count.

IMPLEMENTED 2026-11-17 (commit 448dec7): patch
0006-bridge-binding.patch carries the core body
(verified against a core reconstructed from 0005: the
patch reproduces the edited core byte for byte); the
layer's --serve-region is implemented and self-tested
here (842 checks PASS); the layer unit and the finish
script moved to region mode and the ELBE XML bodies
regenerated to match; link-check 11/11 here. Bench
round remaining: push, pull on telesart, apply 0006 in
the tree, rebuild the kernel, rebuild the layer there,
resubmit the image, restage the payload.

## Task 7. Stage the payload, submit ELBE

`make` in `build/`; copy `build/bin/iscandar-layer` and
`build/bin/iscandar-exerciser` into
`~/telezart/share/iscandar-payload/`. Submit by absolute path:
`elbe initvm submit /home/andrew/telezart/fubuki-iscandar.xml`
(the patched XML placed at that path), and build the image.
The gen_pool served set is named here, per pool, as pools
register.

## Task 8. Boot the evidence run

QEMU at `-m 1024` (Task 6 Decision 1(B)), patched kernel and initrd handed in by
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

## Bench findings, 2026-11-17 evening (first boot attempts)

Config delta, now part of the config of record: the boot
pairing is our kernel with ELBE's initrd, and ELBE's modules
cannot load in our build, so the boot-path drivers are
built in. Flipped from `=m` to `=y`: `VIRTIO_BLK`, `NET_9P`,
`NET_9P_VIRTIO`, `9P_FS`, `EXT4_FS`, `LIBCRC32C` (`JBD2`
followed `EXT4_FS` by select). Everything else unchanged.
Kernel rebuilt with `make -j1` (a `-j$(nproc)` build was
OOM-killed at the vmlinux link); bzImage 10,351,104 bytes
before the flips. Layer self-test at the bench: PASS
(842 checks, 0 failures, net served 130,988). ELBE image
built at `elbe-build-20261117-194225`.

Three boots. Boot 1: initrd gave up waiting for the root
device (no block drivers). Boot 2: root mount failed (no
ext4). Boot 3 reached systemd, then userspace collapsed:
sh, agetty, cron and a respawning unit segfaulted in the
dynamic loader executing zero-filled pages, the RCU stall
detector fired with the CPU idle, and the clocksource
reported a 316-second readout interval. No panic, no
Iscandar frames in any stack.

The channel control block, read from the QEMU monitor
after the hang, settles the engagement question: magic,
version, arena base, 4 cubes and 128 channel pages all
written by the layer; request tail 0, response tail 0,
requests served 0; prefix counters about 785,000 calls,
16,128 log entries, overflow set. The bridge never served
a single allocation. The engine was never exercised; it
stands untested, not failed.

Three faults, in order. (1) Setup: the config's boot-path
closure was never checked against the our-kernel /
ELBE-initrd pairing (fixed by the delta above). (2) Setup:
the raise never engaged. Correction to the first writing
of this finding (checked against the unit on disk
2026-11-18): the raise unit itself retries 50 times over
about ten seconds and exits 1 loudly in the journal; it
is not fail-open. The hole is downstream: the exerciser
unit carries no Requires on the raise, so a failed raise
still lets the exerciser and finish run unraised and
export evidence indistinguishable from a raised run.
Lead suspect remains the attach race (the raise's window
against the layer's control-block write); the fix is a
raise that waits on the control block's magic, and an
exerciser that Requires the raise.

Fix applied here, 2026-11-18 evening (reaches the bench
on the next pull). The raise's probe loop now runs 600
tries over about two minutes: the write is the probe,
because the kernel refuses with -EPROTO until the
control block carries the magic and the version, so the
loop is the synchronization with the layer's attach. On
exhaustion the unit dumps the final bridge state into
the journal before exiting 1. The exerciser unit now
carries Requires and After on the raise, so a failed
raise stops the evidence run instead of letting it run
unraised. Link-check's boot-order check asserts both
lines, the ELBE XML inline copies are regenerated to
match, and the set passes 11/11 here. The cross-boundary
canary and the bisect boots below follow the fix.
(3) Open: the zero-page deaths happened in the prefix
posture, so they are not the serving path. The corruption
is zero-shaped, the signature of a memset landing on live
memory; the new writers aimed by physical address are the
kernel's arena binding (patch 0006) and the layer's arena
attach. Suspect, not verdict.

Next round bisects by construction: boot the patched
kernel with the spans and the Iscandar units disabled and
confirm userspace lives; then start the layer by hand and
watch the attach. The guest image at
`elbe-build-20261117-194225` keeps its journal for
forensics.

## Bench findings, 2026-11-18 evening (bisect boots)

The raise fix above is in the image built this evening
(`elbe-build-20261118-195332`); link-check passed 11/11
on telesart before the build. Two bisect boots, same
kernel and spans as the failed runs:

Boot A (all five Iscandar units masked on the kernel
command line): fast login, shell healthy. `/proc` read:
state prefix, arena self-check PASS, prefix log written
16,128 with overflow set (the log saturates during every
boot; the hung guest's identical figure is a property of
the boot, not of the failure), center 11 at 742,670
calls. `dmesg` segfault count 0. The kernel binding and
the reserved spans stand next to a living userspace and
nothing dies: exonerated.

Boot B (raise, exerciser and finish masked; payload and
layer live): login, layer active, bridge state mirrors
Boot A (center 11 at 737,538), segfault count 0. The
control block, read from the QEMU monitor and decoded
against the shared header: magic, version, the layer's
arena base, 4 cubes, 128 channel pages; all ring indices
zero; requests served, refusals and invariant violations
zero; prefix counters live and matching the `/proc`
read; stop 0. The layer's attach is exonerated by the
same construction.

What remains untested is everything from the raise
onward, which is where the cross-boundary canary belongs
(and where the paper now carries the caution). Decision
of record, 2026-11-18: bank Boots A and B, build the
canary next; no manual raise probe ahead of it.
