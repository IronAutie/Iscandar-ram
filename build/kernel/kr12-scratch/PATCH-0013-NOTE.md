# Patch 0013 (DRAFT): step counters for KR12 — design note

Status: DRAFT for the author's check. Nothing committed,
nothing applied to any tree of record. The diff is
`../patch/0013-step-counters.DRAFT.patch`; all work was
done in this scratch directory (`pristine/` = the staged
tree's mm/iscandar sources as they stand with patches
0001–0012 applied, `work/` = the patched copies).

Purpose: KR12 measures actual allocation step counts
in-kernel, so Paper 3 (practice) can be set against the
termination table (theory: KERNEL-BUILD-DESIGN-DRAFT.md
section 4; atomic-sim/REPORT.md). A **step** is one
iteration of a scanning loop in the search or commit
path (one row, plane, sector position, or cube
examined).

## 1. Site map: simulation sites -> C sites

The simulation counted with three counters (search,
commit, gate) inserted by `atomic-sim/instrument.py` at
anchored sites in copies of these same sources, and
reported their sum per call. The mapping is not
approximate: the staged kernel sources are
**byte-identical** to the simulation's originals
(verified by de-instrumenting the sim copies of Nano.c,
Beam.c, Engine.c, RowsCheck.c and diffing: empty), every
one of the sim's 36 loop-head line numbers lands on a
`for`/`while` head at the same line in the kernel
copies, and the sim's three pattern counts match
exactly (32/32/32). After insertion, the step lines sit
at identical line positions in the kernel copies and
the sim copies (spot-checked Nano, Beam, Engine,
RowsCheck, PlanesCheck).

Loop sites (STEP inserted as the loop body's first
statement), by file, line in the original, sim kind:

| file | lines | kind | what the loops scan |
|---|---|---|---|
| Nano.c | 40 | search | positions within one row word |
| Tiny.c | 55, 85 | search | sector sweeps, both directions |
| Small.c | 48, 60 | search | sector sweeps, both directions |
| Large.c | 28, 43, 83, 175, 179, 225, 277, 281, 321, 336 | search | sector, core-run, and plane sweeps |
| Huge.c | 74, 85, 134, 182, 244, 271, 302, 326, 361, 461, 525, 543 | search | row/sector/plane walks of the span gears |
| Commit.c | 133, 145, 178, 189 | commit | interior row and plane walks of mark and unmark |
| Engine.c | 214 | gate | the Large/Huge verification gate's plane loop |
| Beam.c | 83 | gate | the beam walk: one per cube offered |
| PairsRowDriver.c | 70, 130 | search | pivot and row sweeps of the pairs gear |
| Modes.c | 121 | search | CubeCheck's plane-count loop |

Pattern sites (the hand-unrolled gears; one step per
unrolled block, per the sim's stated rule):

| file | pattern | count | placement |
|---|---|---|---|
| RowsCheck.c | after each `row_NN:` label | 32 | per row block |
| PlanesCheck.c | before each `(*lane++)(...)` invocation | 32 | per lane invocation |
| PlaneSmallDriver.c | after each `/* Trial N:` comment | 32 | per trial |

Total counter sites: **132** (36 loop + 96 pattern).

Loops that exist in the engine but are **not** counted,
because the simulation does not count them (the sim is
the definition, not physical completeness): Cube.c's
row-construction loop at init; all coordinate, bijection,
and pointer arithmetic (no loops); Mark.c/Unmark.c have
no loops at all — both funnel into Commit.c's
`commit_sectors`, whose four walks are the whole
counted commit path. Nothing.c has no search and no
sites.

## 2. What the diff changes, file by file

Diffstat: 18 files, +277 −3. Every engine file is pure
addition (zero lines removed or changed); the only
removals are 3 lines in core.c (one comment sentence
extended, the /proc format string widened).

- **iscandar_steps.h (new, +43)**: the step definition
  and the counting discipline in a header comment; the
  extern counter declarations; `ISCANDAR_STEP()`, one
  increment of the global.
- **The 13 instrumented engine files**: one
  `#include "iscandar_steps.h"` before the first include
  (the sim's placement for its steps.h) plus the 132
  `ISCANDAR_STEP();` lines, inserted by
  `instrument-kr12.py` in this directory, anchored and
  counted exactly as the sim's script anchored them.
- **Engine.h (+5)**: `EngineResult` gains
  `uint64_t steps`.
- **Engine.c (+16)**: `engine_call` snapshots the
  counter at entry and sets `result->steps` to the
  delta at **all seven exits** (admission gate, Nothing,
  search failure, verification failure, coordinate
  failure, mark failure, success). Plus its 1 gate site.
- **CubeEngine.h (+3)**: `CubeEngineResult` gains
  `uint64_t steps`.
- **CubeEngine.c (+2)**: copies `result.steps` through;
  the negative-request shortcut (no engine call) sets
  `out->steps = 0`.
- **Beam.c (+30)**: defines the two globals;
  `beam_allocate` and `beam_free` snapshot the counter
  and publish the delta in `iscandar_beam_last_steps`
  at every exit; `beam_scan`'s room-check skip sets
  `out->steps = 0`, so a CubeEngineResult's steps field
  is defined on every path. Plus its 1 gate site.
  **Beam.h is untouched** (see judgment call 2).
- **core.c (+37 −3)**: includes the header; per-class
  `iscandar_total_steps[]` / `iscandar_worst_steps[]`
  ledgers and `iscandar_steps_note()` beside the
  latency ledger; the note is folded in at the same two
  seats as `iscandar_latency_note` (after
  `beam_allocate`, after `beam_free`); the /proc class
  line becomes
  `class %s calls %llu worst_cycles %llu total_steps %llu worst_steps %llu`.
  The latency comment's "the walk is untouched" gains a
  sentence naming this patch as the one exception.

## 3. Judgment calls (author to rule)

1. **R9 — the engine text is modified.** Every prior
   patch kept the admitted engine text byte-identical in
   the kernel tree; core.c's own header and the Makefile
   both state the rule ("the admitted C text,
   unmodified (R9)"). Counting steps at the sim's sites
   is impossible from outside the text: no header or
   macro seam can hook a loop iteration. This draft
   therefore does to the kernel copies exactly what the
   atomicity simulation did to its copies — counter
   lines and one include, nothing else, with the
   admission question answered by re-verification (the
   sim re-ran its admission diffs on the instrumented
   build; section 4 below is the desk equivalent, and
   KR12's exerciser run is the live one). But it is a
   departure from the staging terms as written, and
   whether KR12 flies as an instrumented build beside
   the build of record, or 0013 amends the terms, is
   the author's ruling, not this draft's.
2. **The count does not ride in the Beam struct.** The
   natural seat for "carried up through beam_allocate"
   would be a Beam field. It is barred: the struct's
   shape is part of the link-time mass (16-byte header,
   266,512-byte Beam region), byte-compared against a
   runtime-built Beam by the mass checks
   (MASS-LAYOUT.md). Adding a field changes the mass
   and the 561,424-byte total. The draft instead
   publishes the count in `iscandar_beam_last_steps`,
   an extern defined in Beam.c and set by beam_allocate
   and beam_free before they return — the count still
   travels up through EngineResult and CubeEngineResult
   per engine, and beam_allocate's published figure is
   its own snapshot delta (its gate steps included),
   which equals the sum of the per-cube counts plus its
   gate steps by construction. If the author prefers an
   out-parameter on beam_allocate/beam_free instead,
   that is a small rework of the Beam.c and core.c
   hunks; the engine hunks are unaffected.
3. **One counter, not three.** The sim kept search,
   commit, and gate apart and reported the sum. The
   draft keeps only the sum, since the sum is what the
   termination table states and what /proc can usefully
   print per class. Splitting again later is a
   three-counter widening of the same sites.
4. **Frees are counted, and folded into the same
   per-class ledgers as allocations.** The counter
   cannot help counting frees: the commit sites fire on
   the unmark path, and the theory itself prices frees
   as the commit walk (at most 2,104 steps; 12 in the
   trace). The draft folds beam_free's count into the
   same total/worst ledgers, at the seat beside the
   free's latency note — deliberately mirroring the
   cycle ledger, which has always counted allocation
   and free door calls together, so calls, cycles, and
   steps share one denominator. The cost of that
   choice: a class's `worst_steps` is the worst door
   call of that class, not the worst allocation
   strictly. For Nano and Tiny, the classes KR12
   compares, a free's walk over a granted block is a
   handful of steps at most (trace frees: at most 12),
   so the allocation worst is what the ledger will
   show in every case the trace exercises. For Huge
   and CubeCheck, a whole-cube free (2,104) can stand
   as the class worst; the paper should say which it
   is quoting. If the author wants allocation-only
   ledgers, deleting the second `iscandar_steps_note`
   call (the iscandar_free seat) is the whole change;
   the free counts remain available in
   `iscandar_beam_last_steps` after each beam_free.

## 4. Desk check (what was run, and the results)

1. **Apply check**: the draft applies cleanly
   (`patch -p1`, dry-run then real) to pristine copies
   of the staged sources, and the applied result is
   byte-identical to the work copies the diff was
   generated from.
2. **Userspace compile**: all 26 engine objects
   compiled from the patched copies with gcc
   (-std=gnu11, declspec/Gears.h force-includes as in
   the kernel Makefile; the kernel's stdint/string
   shims set aside for system headers, as the sim
   build does). No errors; the only warnings are the
   pre-existing classes the Makefile already tolerates
   (unused labels in RowsCheck, missing prototypes in
   the gear drivers).
3. **Kernel-flags compile**: a full copy of the staged
   tree was patched and the 15 patched objects
   (core.o included) were compiled with the exact
   command lines recorded in the tree's own .cmd
   files. All 15 compile; no new warnings. Two limits,
   stated plainly: objtool post-processing did not run
   (a full `make` on this VM tries to rebuild objtool
   and this VM has no gelf.h/libelf headers — an
   environment gap, not a patch finding), and no link
   was attempted. The bench build on telesart remains
   the full compile check, as with every patch in the
   set.
4. **Behavioral check**: a scratch harness ran one
   fixed 61-op script (allocations across every class,
   failures, frees, CubeCheck on full and empty beams)
   against a 4-cube Beam built from the pristine
   objects and from the patched objects. **Every
   decision is identical**: same statuses, same granted
   starts and counts, same final books. (The script's
   drain loop re-frees some slots; both builds answer
   those with the same FAIL_OVERLAY status and 0
   steps, identically.) The patched build's step
   counts cross-check against the termination table
   at its known points:
   - free of a whole-cube block: **2,104 steps** —
     the table's stated worst free, exactly;
   - CubeCheck allocation: 2,104 engine steps, 2,105–
     2,106 at the beam (the table's 2,104, plus one
     beam gate step per cube offered);
   - Huge (32,767) on an empty cube: 3,255 engine
     steps (the table's admission worst), 3,257 at
     the beam with its gate steps;
   - failed beam allocations (over-size, negative,
     zero): 4 steps, the four gate steps of the walk
     itself, no engine search;
   - Nano allocations on a fresh beam: 4–13 steps;
     Tiny: 27–38 — the right shape against trace
     worsts of 403 and 61 on a full boot's occupancy.

## 5. Risks and open points

- **Measured behavior**: the only behavioral effect of
  the counters is time. Each step now costs one global
  increment inside the section, so the section (the
  interrupt-suppression window the step count sizes)
  runs slightly longer per step, and a KR12 boot's
  `worst_cycles` will read slightly higher than
  KR11's for that reason alone. Step counts themselves
  cannot be affected: the counter feeds no decision.
  Cross-patch cycle comparisons should carry that
  caveat; the control comparison is unaffected (its
  harness times both versions' doors, and only the
  Iscandar side carries counters — which is itself
  worth one sentence in the paper, since the counters
  are part of the practice being measured).
- **Concurrency**: the counter is a plain global, not
  an atomic. Its correctness rests on the build's
  standing discipline — every engine entry inside
  core.c's spin_lock_irqsave section, init-time entries
  single-threaded — the same premise the simulation
  modeled with its atomic sections and violation
  counter. Caller audit of the staged tree:
  beam_allocate/beam_free are called only from core.c;
  engine_call only from CubeEngine.c; cube_engine_call
  only from Beam.c. On Fubuki's single CPU the count
  is exact. A future caller reaching the engine
  outside the section would lose steps to a race,
  never corrupt a decision; the sim's answer to that
  case was a violation counter, which this draft does
  not reproduce (judgment call 3's simplification
  extends here).
- **Attribution hygiene**: any engine activity outside
  beam_allocate/beam_free (init, the canary's
  arithmetic) increments the global but is never
  attributed, because every published count is a delta
  taken inside those two functions. The ledgers cannot
  be polluted by it.
- **Overflow**: u64 throughout; at the trace's ~34
  steps per call, the counter outlives the machine.
- **Name check**: Huge.c has local variables named
  `step`; the macro is `ISCANDAR_STEP()` and no site
  collides. `iscandar_steps.h` follows the directory's
  shim-header pattern and shadows nothing.
