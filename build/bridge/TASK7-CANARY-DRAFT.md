# Task 7 draft: the cross-boundary canary (for Andrew's ruling)

Prepared 2026-11-18 evening, after Boots A and B banked the
bisect. The paper carries the discipline (the macro-scale
caution closing Live Interposition, stetted 2026-11-18);
this draft is its implementation shape. Nothing here is
built until the decisions at the end are ruled.

RULED 2026-11-18, all three as drafted: D1, the separate
canary unit; D2, the `canary` verb on /proc/iscandar_bridge;
D3, the negative bench boot is part of the acceptance
round. Implementation follows the rulings the same evening.

## Why the arena premise, and only that premise, needs a witness

The channel's premise is already cross-witnessed in the
record: kernel-written prefix counters and layer-written
header fields coexist coherently in the one channel page,
read back from the QEMU monitor on 2026-11-17 and again on
2026-11-18. The arena's premise has no such witness. The
kernel believes its memblock reservation and the direct
map; the layer believes its mmap of /dev/iscandar at the
arena offset. If those two beliefs name different physical
memory, every check either side runs on its own belief
passes, and the first grant writes into whatever the layer
mapped while the kernel's callers dereference whatever the
kernel mapped. The zero-page signature of 2026-11-17 is
what a wrong premise looks like from inside.

## The protocol (as the paper states it)

One fixture page: arena offset one page, the ledger's
second page. The page itself is fixture space, but the
words written on it are ledger bins for grantable pages,
so the protocol restores both words to 0 before it ends
(see Ordering safety). Two words on it, two patterns,
both constants in the shared header (one home for every
constant):

- `ISCANDAR_CANARY_FROM_LAYER` (word 0), written by the
  layer through its own arena mapping.
- `ISCANDAR_CANARY_FROM_KERNEL` (word 1), written by the
  kernel through the direct map, only after it has read
  word 0 and found the layer's pattern there.

Steps:

1. The layer, in a new `--canary` mode, mmaps the arena via
   /dev/iscandar exactly as the serving attach does, zeroes
   word 1, writes the layer pattern at word 0, with a
   memory barrier between.
2. The layer writes `canary` to /proc/iscandar_bridge.
   The kernel's new verb reads word 0 through
   `iscandar_offset_to_ptr`. On a match it writes the
   kernel pattern at word 1 and returns success; on a
   mismatch it returns -EIO and records the failure.
3. The layer polls word 1 through its own mapping for the
   kernel pattern, bounded (about ten seconds). Match:
   exit 0. Timeout or wrong value: exit 1, loudly.
4. `/proc/iscandar_bridge` gains a show line:
   `canary: PASS`, `canary: FAIL`, or `canary: not run`,
   from the kernel's record of its half. The layer's half
   is the canary unit's exit status in the journal. Both
   halves are therefore on record, from opposite sides of
   the boundary.

Ordering safety: the canary runs after the layer's own
raise (the layer unit's process initializes the engine
fixtures at its start) and strictly before the kernel
raise. The words land in the ledger bins for cube 0
pages 2048 and 2049, which are grantable pages' bins, so
the canary restores both words to 0 before it exits,
success or failure: a leftover pattern would read as a
free run and hand a page out wrongly. No later
initialization overwrites the words; restoration is part
of the protocol, in the kernel self-check's own
scribble-then-restore manner. A stale kernel pattern
cannot produce a false pass: the layer zeroes word 1
before writing its own pattern, and the kernel writes
its pattern only on seeing the layer's.

## The boot chain

A new unit, `iscandar-canary.service`: After the layer,
Before the raise, Type=oneshot, ExecStart runs the layer
in `--canary` mode. The raise unit gains
`Requires=iscandar-canary.service` and
`After=iscandar-canary.service`, the same construction
the exerciser gained on 2026-11-18. A failed canary
therefore stops the chain the way a failed raise now
stops the exerciser: no raise, no evidence run, and the
journal says why. The ELBE XML gains the unit's inline
body and its wants symlink alongside the other five.

## Files touched

- `build/bridge/iscandar_bridge.h`: the two pattern
  constants. No struct change, no version bump; the
  control block is untouched.
- `build/patch/0007-canary.patch`: the `canary` verb in
  the bridge write handler, the recorded result, and the
  show line. Small, and confined to the bridge core.
- `build/layer/iscandar_layer.c`: the `--canary` mode.
- `build/units/iscandar-canary.service`: new.
- `build/units/iscandar-raise.service`: the Requires and
  After lines.
- `build/elbe/fubuki-iscandar.xml`: the new unit's inline
  body and wants symlink, regenerated to match.
- `build/link-check.sh`: the canary unit joins the
  xml-copies pairs; boot-order asserts the canary's
  ordering and the raise's Requires on it.

## Checks

1. Link-check, extended as above, 11/11 with the new
   assertions inside boot-order and xml-copies.
2. The layer self-test gains the canary in simulation:
   a responder that answers correctly passes; a responder
   that never answers fails on the timeout; a responder
   that answers with the wrong pattern fails at once.
   The witness must be able to say no, in both directions.
3. Bench, one negative boot before the evidence run:
   boot with a deliberately wrong `iscandar_arena=`
   parameter. The canary must FAIL, the raise must not
   run, and userspace must live. A witness that has never
   refused is not yet a witness.

## Decisions for Andrew

- D1. Placement: a separate canary unit between the
  layer and the raise (this draft), or the canary folded
  into the raise unit as an ExecStartPre. The separate
  unit carries its own journal line and mirrors the
  exerciser construction.
- D2. Kernel surface: a `canary` verb on
  /proc/iscandar_bridge (this draft), consistent with
  `raise` and `stop`, against any control-block field
  protocol. The control block stays untouched.
- D3. The negative bench boot (check 3): part of the
  acceptance round, or deferred. This draft says part of
  it, once, before the evidence run is attempted.
