# DRAFT: Page exerciser, candidate standard-run pattern (Decision 6)

Status: SUPERSEDED 2026-11-11 by paper-3-exerciser-pattern.md, which
finishes the pattern (scale factor 16, anchors generalized to the
two largest classes) and is implemented by exerciser/exerciser.c.
Nothing here is in the manuscript, and no exerciser code has been written
from it. It answers Decision 6 in `paper-3-composition.md` (the exact
allocation pattern and size classes) with one concrete, fully counted
proposal, so the decision can be taken by editing numbers rather than
inventing them.

## Design constraints it was built to

- Deterministic: the program is fixed in advance, so the prediction sheet
  (Andrew, 2026-11-09) can be computed from the program alone, before any
  run, and never edited after it.
- Balanced: every allocation is released by the end, so the expected end
  state is zero exerciser-held pages.
- Auditable by class: allocations and frees are counted per size class,
  matching the audit's predicted versus actual table quantity by quantity.
- Modest: this is the standard run (menu option 1, the 10-second default).
  It produces page-family evidence for reconciliation; it is not a stress
  test. The extended run (menu option 2) is the same pattern with the
  churn phase repeated 9 times instead of 3.

## Size classes and Phase A counts

Page size is the guest's own (4 KiB in the Fubuki configuration used for
Paper 2). Classes are in pages.

| Class (pages) | Blocks allocated in Phase A | Pages |
|---|---|---|
| 1 | 64 | 64 |
| 2 | 32 | 64 |
| 4 | 16 | 64 |
| 8 | 8 | 64 |
| 16 | 4 | 64 |
| 32 | 2 | 64 |
| 64 | 1 | 64 |
| 128 | 1 | 128 |
| Total | 128 | 576 |

Each block is touched on every page it holds (one write per page) as soon
as it is allocated, in allocation order.

## Phases

- Phase A, fill: allocate the table above, in class order, touching every
  page. Peak live: 576 pages (2.25 MiB at 4 KiB pages).
- Phase B, halve: release every odd-indexed block in each class with an
  even count. The two singleton blocks (64 and 128 pages) are held as
  anchors. Released: 63 blocks, 192 pages. Live after Phase B: 65 blocks,
  384 pages. The free runs left between held blocks are the fragmentation
  the trace should show.
- Phase C, churn: 3 cycles. Each cycle re-allocates exactly the set freed
  in Phase B (63 blocks, 192 pages), touches every page, holds all of it
  to the end of the cycle, then releases all of it. Peak live returns to
  576 pages in each cycle.
- Phase D, drain: release everything still held (65 blocks, 384 pages).
  End state: 0 exerciser-held pages.

## Prediction sheet totals (computed from this pattern alone)

- Total allocations: 317 blocks (128 in Phase A, 189 in Phase C).
- Total frees: 317 blocks (63 in Phase B, 189 in Phase C, 65 in Phase D).
- Peak live: 576 pages.
- Live after Phase B: 384 pages.
- End state: 0 pages held.
- Allocations by class and frees by class follow directly from the table
  and the phase rules above; the exerciser prints them in full on its
  prediction sheet before it runs.

## Open points for Andrew

- Scale: 576 peak pages is deliberately small. Multiply every Phase A
  count by a single factor to scale the run without changing its shape;
  the prediction totals scale with it.
- Whether the singleton anchor blocks should also be halved in a later
  phase, or stay held to Phase D as written here.
