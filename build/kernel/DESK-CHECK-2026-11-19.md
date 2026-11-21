# Desk check of the marked mass, 2026-11-19

Step 1 of the design draft's verification order, run on
this VM. Generator: gen_mass.py. Layout: MASS-LAYOUT.md.
Checker: check_mass.c, linked against the admitted C
translation (the engine repository's gear bodies, the
k=4 Beam), the generated mass assembled as its own
object. All five claims in the layout document were
checked; all pass.

1. Self-consistency: ok. For each live cube the store's
   popcount equals count_overall, the row-counter sum,
   the plane-counter sum, and the mirror (27 in cube 0,
   19 in cubes 1 to 3, beam total 84). The marked set is
   exactly the fixture set. Slots above the cardinality,
   the ledger, and the registry are zero.
2. Identity: ok. The mass's Beam region is
   byte-identical to a reference Beam built the runtime
   way (beam_init, the fixture runs marked through the
   engine, the mirrors and the beam total set to match).
   The born-marked state is the state the old
   construction produces, with no construction at
   runtime.
3. Behavior: ok. An eight-request battery (1, 5, 8, 32,
   64, 300, 1024, 5000 sectors) against the mass-seated
   Beam and the reference Beam, in separate processes,
   granted identical pointers and freed clean; after the
   frees both stand byte-identical to their born state.
   First grant: cube 0, sector 31, in row 0's free tail
   past the fixture run (the Nano bullet scans its row
   word from the high end; the layout document's first
   statement of this check predicted sector 27 and was
   corrected to the measured placement).
4. Two-path witness: ok. Cube 0's store word 0 read by
   byte offset from the mass base equals the same word
   read through the struct path: 0x07FFFFFF.
5. Dumpable without booting: ok. The linked checker's
   section table shows .iscandar_state at 561,424 bytes
   and .iscandar_arena at 536,870,912 bytes.

The generator's constants all parse from their homes of
record; its internal cross-checks (geometry multiplying
to CUBE_PAGES, fixture runs summing to
FIXTURE_PAGES_TOTAL, net served arithmetic) pass. One
generator assertion failed on its first run (a wrong
formula in the check itself, planes counted twice) and
was corrected before the mass was emitted; the emitted
mass was never wrong.
