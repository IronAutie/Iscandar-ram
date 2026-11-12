# Page exerciser: the standard-run pattern (finished)

Status: FINISHED 2026-11-11 at Andrew's direction, resolving the
two open points of the candidate draft of 2026-11-09 (scale, and
the anchor treatment). The program is `exerciser/exerciser.c` in
this repo; it is installed in the guest as a boot-up application
(see `exerciser/README.md`). This document is the pattern of
record: the prediction sheet below is computed from it alone and
is never edited after a run.

## Rulings that finish it

- Scale factor 16 applied to every Phase A count of the draft.
  Peak live becomes 9,216 pages (36 MiB at the guest's 4 KiB
  page), a modest load beside the boot's own recorded peak
  (56,023 live objects, 13.38 MiB in the control trace) and well
  inside the 512 MiB arena, while every class is exercised at
  least 16 times.
- Anchors generalized: in the two largest classes (64 and 128
  pages) every block is an anchor, held through Phases B and C
  and released only in the drain. The draft's singletons were
  anchors by being single; at scale the anchor is the class.

## Size classes and Phase A counts (scale 16)

| Class (pages) | Blocks | Pages |
|---|---|---|
| 1 | 1024 | 1024 |
| 2 | 512 | 1024 |
| 4 | 256 | 1024 |
| 8 | 128 | 1024 |
| 16 | 64 | 1024 |
| 32 | 32 | 1024 |
| 64 | 16 | 1024 |
| 128 | 16 | 2048 |
| Total | 2048 | 9216 |

Each block is touched on every page it holds (one stamped write
per page) as soon as it is allocated, in allocation order. The
stamp on a page is the block's allocation serial and the page's
index within the block, so a later read can tell whether the
page still holds what was written to it.

## Phases

- Phase A, fill: allocate the table above, in class order,
  stamping every page. Peak live: 2,048 blocks, 9,216 pages.
- Phase B, halve: in each class from 1 to 32 pages, release
  every odd-indexed block. The 64 and 128 page classes are the
  anchors and are not halved. Released: 1,008 blocks, 3,072
  pages. Live after Phase B: 1,040 blocks, 6,144 pages. The
  free runs left between held blocks are the fragmentation the
  trace should show. Before the release, every held block's
  stamps are verified.
- Phase C, churn: 3 cycles in the standard run (menu option 1,
  the 10 second default); 9 cycles in the extended run (menu
  option 2). Each cycle re-allocates exactly the set freed in
  Phase B (1,008 blocks, 3,072 pages), stamps every page, then
  verifies the stamps of everything held, old blocks and cycle
  blocks together, which is where an overlay would show, and
  releases the cycle's blocks. Peak live returns to 9,216
  pages in each cycle.
- Phase D, drain: verify all stamps once more, then release
  everything still held (1,040 blocks, 6,144 pages). End state:
  0 exerciser-held pages.

A stamp that reads back wrong at any verification is an
invariant violation: the program records it in its ledger,
stops, and exits with a failure status. It does not retry and
it does not continue past the failure.

## Prediction sheet (standard run, computed from this pattern alone)

- Total allocations: 5,072 blocks (2,048 in Phase A, 3,024 in
  Phase C). Total frees: 5,072 blocks (1,008 in Phase B, 3,024
  in Phase C, 1,040 in Phase D).
- Peak live: 9,216 pages. Live after Phase B: 6,144 pages.
  End state: 0 pages held.
- Allocations by class (frees by class are the same numbers,
  the run is balanced): 1 page: 2,560; 2 pages: 1,280;
  4 pages: 640; 8 pages: 320; 16 pages: 160; 32 pages: 80;
  64 pages: 16; 128 pages: 16.
- Extended run totals: 11,120 allocations and 11,120 frees
  (2,048 in Phase A, 9,072 in Phase C; 1,008 in Phase B, 9,072
  in Phase C, 1,040 in Phase D); peak and Phase B figures
  unchanged.

The exerciser prints this sheet, from its own tables, before it
runs (and alone, allocating nothing, under menu option 3,
ledger check only).
