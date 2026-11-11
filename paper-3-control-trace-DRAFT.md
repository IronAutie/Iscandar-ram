# DRAFT: The Control Trace (Paper 3, next section)

Status: prepared 2026-11-08 for Andrew's stet. NOT inserted into
paper-3.tex and NOT pushed to Overleaf. Evidence label in the
composition ledger: MEASURED (the Fubuki boot trace).

## Why this section is next

The resumption point recorded in paper-3-composition.md, and told to
Andrew when he wrapped up on the evening of 2026-11-08: the manuscript
stands at 5 clean pages with The Sift, and its Products and The Census,
a Byproduct of Earlier Research in place, and the control trace section
is next in the step order of the Introduction's ten techniques.

## Section arc, in the shape Andrew settled for the Sift section

1. The question the step answers: what did this system actually ask
   for, and give back, in a complete boot?
2. The method: a recorded boot of Fubuki with the kmem tracepoints
   armed from the start, counted inside the guest, with the trace
   buffer's own counters as the completeness check.
3. The findings: the trace ledger itself, printed as the section's
   numbers, with the two facts that give the ledger its meaning:
   the count is complete (nothing overwritten), and the load is a
   swarm of small cache objects rather than a few large requests.
4. The boundary, stated in the section: the trace armed the kmem
   family only. It did not arm the vmalloc or page-direct families,
   which is exactly why the automatic boot and page exerciser exist
   later in the step order. Frees of objects allocated before the
   tracing window appear as unmatched frees, and are counted as
   such rather than explained away.
5. The hand-off: this ledger is the control the reconciliation
   audits against the census. Every event in it should have a
   sifted site behind it; an event with none is an undocumented
   path caught in the act.

## Candidate opening, itemized, mirroring the Sift opening

- It opens with the question the step answers: what did this system
  actually ask for, and give back, in one complete boot?
- Then the method, a recorded boot of Fubuki traced from the first
  instruction available to the tracer, the kmem family armed from
  the start, the ledger counted inside the guest.
- Then the findings, which the paper can print as the trace ledger
  itself, with the two facts that give it its meaning: the buffer
  counters prove the count complete, and the size distribution
  shows a swarm of small objects, not a storm of large ones.
- Then the boundary, stated in the section rather than apologized
  for later: the trace reads the kmem family only, and cannot see
  the vmalloc or page families, which is exactly why later steps
  exist.
- And it closes with the hand-off: this ledger is the control the
  reconciliation will now audit against the census.

## The numbers, checked against the two records in hand

From the composition ledger (paper-3-composition.md, MEASURED: the
Fubuki boot trace):

- Total trace entries: 1,650,424
- kmalloc allocation events: 105,687
- kmem_cache_alloc allocation events: 706,419
- Total allocations: 812,106
- kfree free events: 166,390
- kmem_cache_free free events: 671,928

Arithmetic check: 105,687 + 706,419 = 812,106 allocations;
166,390 + 671,928 = 838,318 frees; 812,106 + 838,318 = 1,650,424
entries. The three totals agree.

From Andrew's own trace ledger report (main chat, 2026-11-04):

- Buffer completeness: entries in buffer equal entries written,
  overrun 0, commit overrun 0
- Sum requested: 402,331,163 bytes (383.69 MiB)
- Sum allocated: 422,590,176 bytes (403.01 MiB)
- Largest single request: 996,600 bytes
- Matched frees: 754,350, returning 382,010,843 requested bytes
  (364.31 MiB)
- Unmatched frees (pre-window objects): 83,968
- Replaced-pointer allocations: 1,736
- Peak live: 14,028,570 bytes (13.38 MiB) in 56,023 objects
- Live at end: 13,297,716 bytes (12.68 MiB) in 56,020 objects
- Size percentiles, requested bytes: P20 = 72, P50 = 80, P80 = 184
- Size histogram: 153,632 at 64 bytes or under; 555,256 at 65 to
  256; 14,734 at 257 to 1,024; 88,246 at 1,025 to 8,192; 238 over
  8,192

## What remains for Andrew

- Statistics form: settled 2026-11-10 by the census precedent.
  Andrew removed the census math node in Overleaf (its indents
  differed from the rest of the paper) and the repo copy was
  synced to match. The trace statistics take a plain itemize,
  no math node.
- Dictate or stet the section's prose, in the pattern of the Sift
  and Census sections, before anything moves into the manuscript.
- The compiled-subset enumeration and the automatic-boot rehearsal
  still wait on his word, as recorded in the composition ledger.
  Neither is started by this draft.
