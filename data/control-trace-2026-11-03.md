# The control trace ledger (measured)

The recorded boot of Fubuki, captured 2026-11-03 with the kmem
tracepoints armed from the start and the ledger counted inside
the guest. Status: MEASURED. Points from: The Control Trace.

## The trace itself

The raw trace is not duplicated in this repository. It lives in
the iscandar-k4 repository as
`data/telezart-trace-boot-complete-2026-11-03.txt.gz`
(SHA-256 of the gzipped file:
8b2a07f7926585c5bdaf28e3ea2da456a36c16222729478f53bcef28122d06d2;
raw 290,751,268 bytes). A working copy also stands in the
telezart workspace as `telezart-trace-boot-complete-2026-11-03.txt`
(SHA-256
1ba9a4a725d0fa924de9ed97576d77a5dba001f545c1b77da06fef823dfa76a6).

## Completeness

Buffer counters: entries in buffer equal entries written,
overrun 0, commit overrun 0. The count is complete; nothing was
overwritten.

## Event counts

- Total trace entries: 1,650,424
- kmalloc allocation events: 105,687
- kmem_cache_alloc allocation events: 706,419
- Total allocations: 812,106
- kfree free events: 166,390
- kmem_cache_free free events: 671,928
- Total frees: 838,318

Arithmetic check: 105,687 + 706,419 = 812,106 allocations;
166,390 + 671,928 = 838,318 frees;
812,106 + 838,318 = 1,650,424 entries. The three totals agree.

## Sizes and lifetimes of record

- Sum requested: 402,331,163 bytes (383.69 MiB)
- Sum allocated: 422,590,176 bytes (403.01 MiB)
- Largest single request: 996,600 bytes
- Matched frees: 754,350, returning 382,010,843 requested bytes
  (364.31 MiB)
- Unmatched frees (objects allocated before the tracing window):
  83,968
- Replaced-pointer allocations: 1,736
- Peak live: 14,028,570 bytes (13.38 MiB) in 56,023 objects
- Live at end: 13,297,716 bytes (12.68 MiB) in 56,020 objects
- Size percentiles, requested bytes: P20 = 72, P50 = 80, P80 = 184
- Size histogram: 153,632 at 64 bytes or under; 555,256 at 65 to
  256; 14,734 at 257 to 1,024; 88,246 at 1,025 to 8,192; 238 over
  8,192

## Boundary

The trace armed the kmem family only. It does not see the
vmalloc or page-direct families; those are the province of the
automatic boot and the page exerciser later in the step order.
