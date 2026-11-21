# The marked mass: layout of the kernel build's initial state

The kernel build's counterpart of the compile-time map
(build/compile-time-map.md), drawn 2026-11-19 under the
design draft's R8: the engine's own storage arrives
already marked as a solid mass in the image at link
time, there are no initial loops, and init verifies
rather than fills. This document states the mass's
layout and the arithmetic behind it; `gen_mass.py`
emits it from the constants of record, and
`check_mass.c` is the desk check that the emitted mass
is self-consistent and behaves as the state it claims
to be.

Every constant below has exactly one home, as before:
the build constants in build/bridge/iscandar_bridge.h
(read through build/constants/iscandar-build.conf and
the header itself), the engine geometry in the engine
repository's headers, the Beam's shape in the k=4 C
translation's Beam.h. The generator parses those homes;
it carries no constants of its own.

## What the mass is

One contiguous block in its own section,
`.iscandar_state`, holding three regions in order: the
Beam state, the extent ledger, the cache registry. The
layout mirrors the admitted C structs byte for byte, so
the kernel glue seats a `Beam *` over the first region
by cast, with no translation and no seating code: the
mass is not a serialization of the state, it is the
state, in the form the engine reads.

Beside it, a second section, `.iscandar_arena`,
declares the arena itself: ARENA_BYTES (536,870,912)
of reservation, no initialized content. The arena's
pages hold no state at birth; what is born marked is
the engine's books about the arena, in the mass.

## The Beam state (266,512 bytes)

The Beam struct as admitted: a 16-byte header
(cardinality, count_overall, sectors_supported, lead),
then K4_BEAM_MAX_CUBES (32) CubeEngine slots of 8,328
bytes each. A slot is the k=3 Engine (the cube store,
1,024 words at 4 B; count_overall; 32 plane counters;
1,024 row counters: 8,324 B) plus the CubeEngine's
mirror of count_overall (4 B).

Born values, from the build constants:

- Header: cardinality = ARENA_CUBES (4);
  count_overall = FIXTURE_PAGES_TOTAL (84);
  sectors_supported = ARENA_PAGES (131,072); lead = 0.
- Cube 0 carries the fixture run plus the registry
  charge: FIXTURE_PAGES_PER_CUBE (19) + REGISTRY_PAGES
  (8) = 27 sectors, 0 through 26. Its store's word 0 is
  therefore bits 0..26 set (0x07FFFFFF), row counter 0
  is 27, plane counter 0 is 27, count_overall and its
  mirror are 27.
- Cubes 1 to 3 carry the fixture run alone: 19
  sectors, 0 through 18. Word 0 is 0x0007FFFF; row and
  plane counters 0 are 19; the overalls are 19.
- Slots 4 to 31 (above the build cardinality) are
  entirely zero: present because the admitted struct
  carries them, inert because cardinality says 4.

Check the headline arithmetic: 27 + 19 + 19 + 19 = 84
= FIXTURE_PAGES_TOTAL, and ARENA_PAGES minus that is
130,988 = NET_SERVED_PAGES. The mass's books open
exactly where the bridge's books opened at the raise.

## The extent ledger (262,144 bytes)

One slice per arena cube: CUBE_PAGES (32,768) bins at
2 bytes, 65,536 bytes per cube, four cubes. Born all
zero: zero is Nothing, no grant starts here. The
ledger's bytes stand in the mass; its charge stands in
the arena's fixture run (16 of each cube's 19 fixture
pages), as the compile-time map already accounted: the
pages stand permanently live and are never granted.

## The cache registry (32,768 bytes)

REGISTRY_CAP (512) entries at REGISTRY_ENTRY_BYTES
(64): born all zero, no cache registered at birth.
Entries are written later by the control path, in
process context, confirm-then-commit. Its bytes stand
in the mass; its charge stands in cube 0's fixture run
(the 8 registry pages behind cube 0's 19), again as
the compile-time map accounted.

## Totals

| Region | Bytes |
|---|---|
| Beam state (16 + 32 x 8,328) | 266,512 |
| Extent ledger (4 x 65,536) | 262,144 |
| Cache registry (512 x 64) | 32,768 |
| Mass total | 561,424 |

## What the desk check must show

1. Self-consistency, from the emitted bytes alone:
   for each live cube, the popcount of the store
   equals count_overall, equals the sum of the row
   counters, equals the sum of the plane counters,
   equals the mirror; the marked set is exactly the
   fixture set and nothing else; slots above the
   cardinality, the ledger, and the registry are zero.
2. Identity with the constructed state: a reference
   Beam built the runtime way (beam_init, then the
   fixture runs marked through the engine) is
   byte-identical to the mass's Beam region.
3. Behavior: an identical probe battery against the
   mass-seated Beam and the reference Beam grants
   identical pointers (the first grant lands in cube
   0's row 0, in the free tail past the fixture run:
   sector 31 for a one-sector request, the Nano bullet
   scanning its row word from the high end), and after
   the battery's frees both stand byte-identical to
   their born state again.
4. The two-path witness, rehearsed at the desk: the
   witness word (cube 0, store word 0) read by byte
   offset from the mass base equals the same word read
   through the struct path. This is the init canary's
   shape, run here against the linked image.
5. The mass is dumpable from the linked binary without
   booting: the section table shows .iscandar_state at
   561,424 bytes and .iscandar_arena at 536,870,912.

Checks 2 and 3 run the two Beams in separate processes:
the gears keep their cursors in process-global statics
(the atomicity simulation's finding), so a shared
process would let one battery move the other's
cursors. In the kernel there is one Beam, and the
point does not arise.
