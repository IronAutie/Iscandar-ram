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
0x20000000, channel 512 KiB at 0x1FF80000 (corrected
2026-11-18 late: the draft's 0x1FFE0000 was an arithmetic
error, the channel overran the arena base by 96 pages;
see the findings section at the end of this sheet);
every settled figure stands; efficiency deferred by
Andrew's word.
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

## Task 7: the canary, built 2026-11-18 evening

Ruled the same evening, all three as drafted in
`build/bridge/TASK7-CANARY-DRAFT.md`: D1, a separate
`iscandar-canary.service` between the layer and the
raise; D2, a `canary` verb on /proc/iscandar_bridge (the
control block is untouched); D3, the negative bench boot
is part of the acceptance round.

Built and checked here, not yet at the bench:

- The shared header carries the two pattern constants
  (`ISCANDAR_CANARY_FROM_LAYER`, `ISCANDAR_CANARY_FROM_KERNEL`)
  and the offset, `ISCANDAR_CANARY_OFFSET`.
- Patch `0007-canary.patch`: the verb, the recorded
  result, and the `canary:` line in the proc read.
  Verified as 0006 was: applied on top of the core
  reconstructed from 0005 and 0006, it reproduces the
  edited core byte for byte (505 to 551 lines).
- The layer's `--canary` mode: writes its pattern
  through its own /dev/iscandar arena mapping, asks the
  kernel by the verb, polls for the kernel's pattern,
  and restores both words to 0 before exiting, success
  or failure. The words are ledger bins for cube 0 pages
  2048 and 2049, so the restore is part of the protocol,
  not tidiness. The self-test stands a simulated kernel
  in the kernel's place: 847 checks, 0 failures (the
  canary adds 5 to the 842).
- The raise unit now carries `Requires=iscandar-canary.service`
  and orders after it; the ELBE XML carries the canary
  unit's inline body, its manifest line, and its wants
  symlink. Link-check is 11/11 here with the canary pair
  inside xml-copies and the canary's ordering asserted
  inside boot-order.

Next bench round, in order: pull; refresh the tree's
`include/iscandar_bridge.h` from `build/bridge/iscandar_bridge.h`
(never retype a constant); apply 0007 to the tree;
rebuild the kernel with `make -j1`; link-check at the
bench; rebuild the layer and restage the payload; copy
the updated XML and rebuild the ELBE image. Then the
acceptance round: first the negative boot, with a
deliberately wrong `iscandar_arena=` on the append line
and everything else as usual, expecting the canary to
FAIL in the journal, no raise, no exerciser, and a
living login; then the true boot, expecting `canary:
PASS` in the proc read, the raise, the exerciser, and
the finish, which is the evidence run.

## Bench findings, 2026-11-18 late evening (Boot N and Boot C)

### The channel address was wrong (found at the blackboard)

Designing the canary's negative boot forced a re-derivation
of the spans. The Task 6 draft's channel address,
0x1FFE0000, does not end where the arena begins:
0x1FFE0000 + 512 KiB = 0x20060000, which is 96 pages past
the arena base 0x20000000. The draft subtracted 128 KiB
for a 512 KiB channel. Every boot through 2026-11-18
evening ran with the channel's top 96 pages aliasing the
arena's first 96 pages: the response ring and the prefix
log shared pages with cube 0's ledger, maps, registry,
and first grantable pages, and the layer's attach-time
ring zeroing wrote through the alias onto arena pages
0 to 32 after its own raise had built them. Latent in
prefix posture (nothing served), fatal at first traffic.
The corrected address is 0x1FF80000, the true top of the
low half, ending exactly at the arena base as designed.
Boot line of record from here on:
iscandar_channel=0x1FF80000 iscandar_arena=0x20000000.
No code changes; the address lives only on the append
line. The Task 6 draft carries a correction note.

The canary's D3 negative boot was redesigned the same
evening: both sides take every span from the same boot
parameters through the kernel's variables, so a wrong
arena parameter moves both beliefs together and the
canary would pass on a wrong span. The bench negative
became the by-hand refusal (below).

### Boot N (negative, all six units masked, corrected spans)

Fast login. Bridge read: state prefix, arena self-check
PASS, canary: not run, counters counting. The by-hand
write `echo canary > /proc/iscandar_bridge` was refused
with EIO, and the next read showed `canary: FAIL`, with
userspace alive and the counters still climbing. The
witness refuses on the kernel's side; the layer's side
was already covered by the self-test's simulated silence
and wrong answer. Banked.

### Boot C (the true boot, image elbe-build-20261118-2057)

The chain ran: payload installed, layer raised (131072
pages, fixtures 84, net served 130988) and serving,
**canary PASS** in the journal. The guest then collapsed
in a starvation cascade: socket and kzalloc failures,
NULL dereferences in create_empty_buffers killing dd
(the finish unit's early harvest) and systemd-journald,
and bridge timeouts repeating. First reading (from the
console alone) blamed an unanswered first registration;
the forensic boot below corrected that: the bridge had
in fact been serving. QEMU was exited from the monitor
at Andrew's hand; the image is intact and was mounted
read-only (noload) on telesart for the journal, which
held only the synced head of the boot.

Two unit-web defects, independent of the timeout:
systemd found an ordering cycle (finish against
exerciser against multi-user.target) and deleted the
exerciser's start job, so the exerciser never ran; and
the finish unit, pulled in by the target itself, started
alongside the layer instead of after the exerciser. Both
trace to the 2026-11-18 decision to leave finish ungated.

OPEN, not settled: why the first registration went
unanswered. The layer was in its serve loop half a
second before and left no crash line. Decisive test,
parked for Andrew's go: an interactive boot in the
layer-only posture at the corrected address, canary by
hand, then `echo raise` by hand, reading req_tail,
resp_tail, and requests_served from the monitor to learn
which side of the counter failed.

Fix round, to be designed after the forensics:
confirm-then-commit registration (no cache marked, no
ready state, until the layer answers; a timed-out bridge
degrades to stock, never to NULL); the unit web rebuilt
so the exerciser cannot be deleted by a cycle and the
finish is pulled by the exerciser, not the target; and
a link-check assertion on the span arithmetic itself,
channel base + channel bytes == arena base, so this
class of error is caught by the entry check forever.

### Forensic boot, 2026-11-18 ~21:37 (layer-only posture)

Same image, corrected spans, canary/raise/exerciser/
finish masked. Layer serving (state S, hrtimer_nanosleep,
its poll posture), baseline bridge read clean, canary by
hand PASS. The raise was then written by hand and the
guest collapsed as in Boot C. From the QEMU monitor, the
control block at 0x1FF80000 read: magic and version
standing, arena base the layer's mapping, req_head 32,
req_tail 33, resp_tail 32, requests_served 32, refusals
and invariant violations 0, prefix counters matching the
shell read. Request record 33 in the ring: op 4 (cache
create), object size 624, align 8, flags 0x40100, cookie
the cache's kernel pointer.

Findings: the bridge served 32 registrations cleanly.
Call 33, a plain registration indistinguishable from the
32 before it, was published and never answered;
requests_served froze at 32 with the serve condition
standing true in the shared block. The layer process
then quit (ps found no iscandar-layer): it died after
completing serve 32 and before serving 33. Serve 33's
own work is registry arithmetic on pages the raise had
already touched, with no I/O in the serve path, so the
layer's death is not explained by its serving code; the
guest's own log on the image (a segfault line or an
OOM-kill line) names the killer, recoverable by
replaying the filesystem journal on a copy of the
image. That read was attempted the same night: the
journal on the replayed copy persisted only to
21:37:12, the layer's first breath; everything after
the raise never reached disk, because in the cascade
journald is itself a casualty and cannot write its own
witness statement. The coredump store is empty. The
killer therefore stays unnamed by the corpse; the way
to name it is a fix-round re-run with the layer in the
foreground on the console, where a segfault or an OOM
kill prints live instead of depending on the journal.
The fixes do not depend on the name: confirm-then-
commit registration and degrade-to-stock stand either
way.

The starvation cascade's mechanism stands as first
read, with the trigger corrected: with the bridge ready
and 32 caches registered, the layer's death left every
bridged allocation timing out at five seconds and
returning NULL to kernel paths that cannot survive
NULL. Two fix-round requirements follow: registration
must be confirm-then-commit, and the bridge must
degrade to stock when the layer dies, never to NULL:
a dead counter must close the shop, not poison it.

### The evening's lesson and the order of work (Andrew, 22:01-22:03)

The failure is documented here so it can be returned
to, in Andrew's words: "I dont think we should skip
any steps. We document our failures to, so we can get
back to them." The lesson, as he stated it: kernel
processes may not call userland handlers (his Solaris
form: kernel processes may never sleep; if they do,
they crash). The userland experiments were good for
design and have reached their limitations; Iscandar
goes back into the kernel, then is instrumented
thoroughly. Order of work: no steps skipped. The
bridge fix round runs first (confirm-then-commit
registration, degrade-to-stock on layer death, the
unit web rebuilt, the span-arithmetic link assertion,
and the foreground re-run that names the layer's
killer), and the in-kernel build follows on the basis
agreed at the blackboard: conditional compilation at
the platform seams only, one engine text byte-for-byte
in both builds, the same admission vectors replayed
against both.

### Fix round built, 2026-11-18 late evening

Ruled as drafted (Task 8, build/bridge/
TASK8-FIXROUND-DRAFT.md) and built the same night.
Patch 0008-fix-round.patch carries D1 and D2 (core
551 to 638 lines), verified by fresh application on a
reconstructed post-0007 core, byte-identical. The unit
web (D3) is edited in the finish unit, the exerciser's
file of record, and the ELBE XML, including the
wants-symlink farm, which proved to be the finish
unit's real pull-in to the target. Link-check carries
the D3 asserts inside boot-order and the span identity
as its twelfth check, and stands 12/12 here. Not yet
compiled or benched; the bench round runs D0 first
(the capturing launch, the foreground layer, the dmesg
read), then the negative, the true boot, and the
degrade proof. Uncommitted, awaiting Andrew's word.
