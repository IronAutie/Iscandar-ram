# The reconciliation (run 2026-11-10)

Paper 3, step 3, run as a formal step on 2026-11-10 at Andrew's
direction. Status: RUN. This artifact is a comparison of existing
measured artifacts only; no new measurement was taken for it.

## Method

Four instruments and one naming layer are read against each other:

- The sift (`census-2026-11-07.md`) reads the source: textual
  call-site counts by family in the Linux 6.12.116 tree (59,969
  C and header files, about 10.07M lines).
- The control trace (`control-trace-2026-11-03.md`) reads the
  running system: Fubuki's recorded boot, kmem family armed from
  the start, 1,650,424 entries, buffer overrun 0.
- The compiled subset (`compiled-subset-2026-11-10.md`) reads the
  build: each patch point's gating CONFIG read from the 6.12.116
  Makefiles against Fubuki's kernel configuration (the stock
  trixie config of linux-image 6.12.113+deb13-amd64).
- The dye test (`dye-test-2026-11-08.md`) reads the compiled
  artifact: compiled expansions of the slab funnel in mm/slub.o
  (6.12.116, x86_64 defconfig, CONFIG_SLUB=y).
- The patch-point list (`../paper-3-patch-points-DRAFT.md`) names
  the centers: the core functions, with file and line in
  6.12.116, that each family's spellings funnel into.

Version skew, stated plainly because the comparison inherits it:
the sift, the patch list, and the dye are 6.12.116; the compiled
subset reads that evidence against a 6.12.113 configuration, and
the trace was recorded on Fubuki's 6.12.113 kernel.

The comparison is made under Andrew's rulings of 2026-11-10:

1. Substrate: the page allocator stays underneath as the bulk
   supplier of the arena, raised by mmap from the user-space C
   translation layer. It parcels nothing for callers.
2. Oversize kmalloc: `__kmalloc_large_noprof` is patched as its
   own center and served from the arena. No exemption on that
   door.
3. Named caches cross the bridge as requests, size and alignment
   only; cache bookkeeping lives in the translation layer. This
   accommodation is an expansion of Iscandar, to be identified
   as such in a future paper.
4. Early boot: the earliest calls pass through stock and are
   logged as a named prefix. Logged is not missed.
5. Second-ring scope, the middle path: the side centers mempool,
   percpu, and gen_pool are in. The page allocator's own doors
   stay substrate. CMA is moot. Memblock and vmap/ioremap stay
   out of scope, as before.
6. Criterion: any family the live build will not serve is named
   here, and the paper's claim shrinks to fit the naming.

One refinement used below is Muse's, marked where it appears:
patching at the centers covers call sites that one recorded boot
never exercised, because the center serves the spelling wherever
it occurs, not only where the trace saw it.

A check the reconciliation itself ran on the trace's arithmetic:
812,106 allocations minus 754,350 matched frees minus 56,020
objects live at end equals 1,736, which is exactly the trace's
replaced-pointer figure; and 754,350 matched frees plus 83,968
unmatched frees equals the 838,318 total frees. The ledger is
internally consistent. The size histogram also sums exactly:
153,632 + 555,256 + 14,734 + 88,246 + 238 = 812,106.

## Family 1a: the kmalloc spellings (generic sizes)

(a) Sift: kmalloc 5,501; kmalloc_node 67; kmalloc_array 1,036;
kzalloc 21,808; kcalloc 4,806. These five spellings sum to
33,218 of the kmalloc family's 77,577 textual occurrences.
kzalloc is kmalloc with __GFP_ZERO, kcalloc is kmalloc_array
with __GFP_ZERO, and kmalloc_array overflow-checks and calls
kmalloc (patch list, slab.h account). The devm spelling
devm_kzalloc, 6,897 occurrences in the wider surface, also lands
in this family (see Cross-center doors).
(b) Trace: kmalloc allocation events 105,687 of 812,106 total
allocations. The tracepoint does not separate the five spellings
from each other; they arrive as one event stream.
(c) Compiled subset: centers 1.1 (`slab_alloc_node`) and 1.2
(`__do_kmalloc_node`) compiled in; CONFIG_NUMA=y, so the _node
doors exist in this build.
(d) Centers: 1.1, mm/slub.c `slab_alloc_node` (line 4205), the
redirect point; and 1.2, `__do_kmalloc_node` (line 4347), the
second route the slab.h dispatch takes into the funnel.
(e) Disposition: redirected at the centers.
(f) Gaps and disagreements: the dye test disagrees with the
source count at this family's center, and the disagreement is
accounted for exactly. `slab_alloc_node` has 6 source call
sites and no symbol in the compiled object; its dye appears 8
times in .text, once per compiled expansion, each inside a
named wrapper. Five sites sit directly in wrappers; the sixth
sits inside `__do_kmalloc_node`, itself always inlined, and was
expanded into three wrappers: 5 + 3 = 8. The source census
therefore undercounts the compiled copies of the funnel by
design of what it measures: it counts call sites, the dye
counts expansions. Control dye exactly 1, baseline 0. No dye
evidence exists for any other center; the dye instrument was
run on this funnel only.

## Family 1b: named caches (kmem_cache)

(a) Sift: kmem_cache_alloc 329; kmem_cache_alloc_node 24;
kmem_cache_create 464.
(b) Trace: kmem_cache_alloc allocation events 706,419 of
812,106, the dominant door of the recorded boot. The trace does
not split this figure between named caches and the kmalloc
size caches; both pass the same tracepoint, and no input
separates them.
(c) Compiled subset: 1.1 and 1.2 compiled in; 1.5,
`__kmem_cache_create_args` in mm/slab_common.c, compiled in.
(d) Centers: allocations at 1.1 (with 1.2 the second route);
cache creation at 1.5, mm/slab_common.c
`__kmem_cache_create_args` (line 297), the setup-time point
where cache geometries are stated.
(e) Disposition: redirected at the center, under ruling 3. A
cache allocation crosses as a request, size and alignment
only; the geometry table, constructor discipline, RCU
quarantine, per-cache reuse and caps are translation-layer
bookkeeping. The accommodation is an expansion of Iscandar and
is named here so it is not folded silently into the engine's
claims.
(f) Gaps and disagreements: source and runtime invert. In the
source, the generic kmalloc spellings (33,218 occurrences, see
1a) outweigh the cache doors (353 occurrences) by roughly two
orders of magnitude; in the recorded boot the cache door
carries 706,419 events against the kmalloc stream's 105,687.
Static mass is not runtime mass, and the reconciliation takes
the runtime figure as the load the center must serve. By
Muse's refinement, the centers also cover the cache call
sites this one boot never exercised.

## Family 1c: the free doors

(a) Sift: kfree 40,597; kmem_cache_free 963; and kvfree 1,982,
counted in the kmalloc family's 77,577 but a cross-center door
(see Cross-center doors). The free doors keep their plain
names in 6.12; they are the exception to the _noprof renaming.
(b) Trace: kfree events 166,390; kmem_cache_free events
671,928; total frees 838,318, against 812,106 allocations.
Unmatched frees: 83,968, objects allocated before the tracing
window. The excess of frees over allocations is therefore
already accounted for inside the trace ledger itself (see the
arithmetic check in Method).
(c) Compiled subset: 1.4, `kfree` and `kmem_cache_free` in
mm/slub.c, compiled in.
(d) Center: 1.4, `kfree` (line 4817) and `kmem_cache_free`
(line 4785), both funneling to `do_slab_free` / `__slab_free`;
bulk variant `kmem_cache_free_bulk` (line 4945).
(e) Disposition: redirected at the center. A free runs the
inverse translation and hands the run back.
(f) Gaps: none between the instruments. The 83,968 unmatched
frees are the measured shape of pre-window allocation, the
same class ruling 4 names as the logged early-boot prefix;
they are recorded here as the trace's own evidence that such
a prefix exists and has a size.

## Family 1d: krealloc

(a) Sift: the census file lists no krealloc spelling count.
That figure is absent from the inputs and is not supplied
here.
(b) Trace: not isolated. The trace records 1,736
replaced-pointer allocations but does not attribute them to
krealloc, and this artifact does not attribute them either.
(c) Compiled subset: 1.6, `krealloc_noprof` in
mm/slab_common.c, compiled in.
(d) Center: 1.6, `krealloc_noprof` (line 1308). It composes an
allocation, a copy, and a free.
(e) Disposition: redirected by composition. If 1.1 and 1.4
are patched, krealloc follows automatically; it is listed so
its coverage is verified rather than assumed (patch list).
(f) Gaps: this family rests on the patch list alone. Neither
quantitative instrument saw it: no sift count, no trace count.
Stated plainly, krealloc is the least evidenced family in
this reconciliation, and its disposition is a construction
argument, not a measured one.

## Family 1e: oversize kmalloc

(a) Sift: no separate spelling exists to count; oversize is a
routing outcome inside the kmalloc spellings, decided by the
slab.h dispatch when a size exceeds KMALLOC_MAX_CACHE_SIZE.
(b) Trace: not separately instrumented. The size histogram
records 238 requests over 8,192 bytes and a largest single
request of 996,600 bytes, inside the kmalloc family's stream.
The inputs do not state the value of KMALLOC_MAX_CACHE_SIZE
for this build, so the exact count routed to the oversize
door is not a figure present in the inputs and is not
computed here. What the trace does establish is that very
large requests occur in the boot stream the build must serve.
(c) Compiled subset: 1.3, `__kmalloc_large_noprof`, compiled
in.
(d) Center: 1.3, mm/slub.c `__kmalloc_large_noprof` (line
4326), reached via `___kmalloc_large_node`. These requests
never touch `slab_alloc_node`; the body calls the page
allocator directly.
(e) Disposition: redirected at its own center, under ruling
2, served from the arena as an exact-fit run and freed by
the inverse bijection at 1.4. No exemption on this door.
(f) Gaps: this is the one first-ring family whose runtime
weight is unmeasured. The patch list identified the door in
source on 2026-11-08 precisely because an unpatched funnel
would leave it as a hole exactly where the largest requests
pass; the reconciliation confirms the door exists in
Fubuki's build and is now ruled in, while stating plainly
that no instrument counted its traffic.

## Family 2: vmalloc

(a) Sift: vmalloc 924; vzalloc 976; vfree 3,436. Nothing in
this family is inlined into callers; every spelling funnels
into one center (patch list).
(b) Trace: not instrumented in the control trace. The trace
armed the kmem family only, and its own boundary statement
says the vmalloc and page-direct families are not seen.
(c) Compiled subset: 2.1 `__vmalloc_node_range_noprof`, 2.2
`__get_vm_area_node` and `alloc_vmap_area`, 2.3
`vm_area_alloc_pages`, and 2.4 `vfree`, all compiled in
(vmalloc.o under CONFIG_MMU=y).
(d) Centers: 2.1, mm/vmalloc.c
`__vmalloc_node_range_noprof` (line 3826), the center. Its
backing half is 2.3, `vm_area_alloc_pages` (line 3594), the
patch site: the page-by-page backing is replaced with one
exact-fit Iscandar run, at the standing calibration of one
sector to one page. Its reservation half is 2.2,
`__get_vm_area_node` (line 3168) via `alloc_vmap_area` (line
2008). Free door 2.4, `vfree` (line 3407), with `vfree_atomic`
(line 3373).
(e) Disposition: redirected at the center, split at the
family's own seam. The backing is Iscandar's; the virtual
range reservation (2.2) stays stock by design and is named
here as not redirected: address-space bookkeeping was never
part of the replacement. One vmalloc contains a hidden slab
allocation (the page-pointer array, allocated by a recursive
vmalloc-family call into Family 1 per the patch list), so
this family's traffic also touches Family 1, and the
translation layer's bookkeeping must expect that.
(f) Gaps: no runtime evidence exists for this family in the
inputs; its row rests on the sift, the patch list, and the
compiled subset. The patch list's remaining work already
names the remedy, a vmalloc-armed trace or the patch-center
logging of the live build, and neither has been run. By
Muse's refinement, the center covers vmalloc call sites the
recorded boot never exercised; the converse also holds and is
stated plainly: nothing in the inputs measures how much
vmalloc traffic Fubuki's boot actually carried.

## Family 3: page-direct (alloc_pages and kin)

(a) Sift: alloc_pages 581; __get_free_pages 175; free_pages
1,107.
(b) Trace: not instrumented in the control trace (same
boundary as Family 2).
(c) Compiled subset: 3.1 `__alloc_pages_noprof` and 3.2
`__free_pages` / `free_pages`, both compiled in
(page_alloc.o, obj-y).
(d) Centers as named in the patch list: 3.1, mm/page_alloc.c
`__alloc_pages_noprof` (line 4810); 3.2, `__free_pages` (line
4926) and `free_pages` (line 4942).
(e) Disposition: not redirected. Under ruling 1 the page
allocator stays substrate: it supplies the arena in bulk,
raised by mmap, and parcels nothing for any caller. Under
ruling 5 its own doors stay substrate as well. Page-direct
callers are therefore served by the stock allocator, and
this family is named here as the build's principal declared
exception on the allocation side. SLUB's own backing demand
(the watch point, `alloc_slab_page`, mm/slub.c line 2470,
compiled in) draws here too; under the redirect, that demand
must not be served twice, and no input settles whether SLUB
remains as bookkeeping over Iscandar grants or is bypassed
outright. That residue is carried as open, below.
(f) Gaps: no runtime evidence in the inputs, and the family's
traffic is known to be large in kind (it backs Families 1
and 2 internally as well as serving its own callers) but is
not counted anywhere in the inputs. The claim consequence is
the ruling's own: the substrate is named, and the claim
shrinks to fit the naming.

## Family 4a: mempool

(a) Sift: mempool_alloc 300.
(b) Trace: not instrumented; no side center was armed.
(c) Compiled subset: 4.1, `mempool_alloc_noprof` in
mm/mempool.c, compiled in (obj-y). Free door `mempool_free`
keeps its plain name.
(d) Center: 4.1, mm/mempool.c `mempool_alloc_noprof` (line
404); free door `mempool_free` (line 518). A mempool is a
guaranteed-reserve wrapper whose elements come from
`mempool_alloc_slab` (line 575, which is kmem_cache_alloc,
Family 1) or `mempool_alloc_pages` (line 624, which is
alloc_pages, Family 3).
(e) Disposition: in scope under ruling 5, served through its
backing. The patch list names no patch point of its own for
mempool: a mempool over slab is redirected at 1.1 with its
elements, and a mempool over pages draws on the substrate.
Both paths are accounted for by centers already named; the
family is in the claim on that basis, stated exactly.
(f) Gaps: no runtime evidence, and the inputs do not record
which backing any given mempool in Fubuki was created with.
The disposition covers both cases, but the split is
unmeasured.

## Family 4b: percpu

(a) Sift: alloc_percpu 291. The spelling is a macro in
include/linux/percpu.h (line 143) resolving to the center;
there is no function of the spelling's own name.
(b) Trace: not instrumented.
(c) Compiled subset: 4.2, `pcpu_alloc_noprof` in mm/percpu.c,
compiled in (obj-y).
(d) Center: 4.2, mm/percpu.c `pcpu_alloc_noprof` (line
1743). Chunk backing is created by `pcpu_create_chunk`
(declared line 1580); areas are allocated in chunks and
carved internally.
(e) Disposition: in scope under ruling 5, redirected at its
center. Its first chunks are built early in boot, so its
earliest traffic falls inside the logged stock prefix of
ruling 4 and is named there, not missed.
(f) Gaps: no runtime evidence in the inputs, and the patch
list records that redirecting this center is a separate
design question from the boot-load families, because the
chunk layer carves internally beneath the center. The
reconciliation names the center and the ruling; the chunk
internals are build-time design work that no input here
settles.

## Family 4c: gen_pool

(a) Sift: the census file lists no gen_pool spelling count.
The generic allocator's public spelling, gen_pool_alloc, is
a header inline (include/linux/genalloc.h line 152) over
`gen_pool_alloc_algo`; a textual count of the center's own
name is not among the census figures, and none is supplied
here.
(b) Trace: not instrumented.
(c) Compiled subset: 4.3, `gen_pool_alloc_algo_owner` in
lib/genalloc.c, compiled in (genalloc.o under
CONFIG_GENERIC_ALLOCATOR=y). Free door
`gen_pool_free_owner`.
(d) Center: 4.3, lib/genalloc.c
`gen_pool_alloc_algo_owner` (line 277); free door
`gen_pool_free_owner` (line 487).
(e) Disposition: in scope under ruling 5, redirected at its
center.
(f) Gaps and disagreements: the instruments and the patch
list pull in different directions here, and the difference
is stated, not smoothed. The patch list records that
genalloc manages device and special memory pools, not
general system RAM, and places it outside the arena's remit
unless a specific pool is implicated. Ruling 5 places the
center in scope. The reconciliation follows the ruling for
the center, and records the patch list's boundary as a
standing caveat: no input names any specific pool, no pool
inventory exists in the inputs, and which concrete pools
the redirect serves is therefore not determined by this
comparison. It is the one family whose served set is open.

## Family 4d: CMA

(a) Sift: the census file lists no cma_alloc spelling count;
the family's callers arrive through the DMA path (see
Cross-center doors).
(b) Trace: not instrumented.
(c) Compiled subset: 4.4, `cma_alloc` in mm/cma.c, ABSENT.
CONFIG_CMA is not set in Fubuki's configuration. This is the
single absence in the whole enumeration: every other patch
point is compiled in, and nothing in the set is modular.
(d) Center as named in the patch list: 4.4, mm/cma.c
`cma_alloc` (line 515); free door `cma_release` (line 562);
backing via `alloc_contig_range` (line 456).
(e) Disposition: moot for Fubuki, under ruling 5. There is
no CMA allocator in this kernel to redirect. Fubuki's
DMA-coherent path runs through the direct allocator (see
Cross-center doors), and the design's CMA question does not
arise for this build.
(f) Gaps: none for this build. The absence is a measured
fact of the compiled subset, and it closes the family
rather than leaving it open.

## Cross-center doors (no centers of their own)

- kv family: spellings kvmalloc, kvzalloc, kvcalloc; free
  door kvfree, 1,982 sift occurrences. Center of record:
  mm/util.c `__kvmalloc_node_noprof` (line 650), compiled
  in, which tries Family 1 first and falls back to Family
  2's center. Disposition: covered if both centers are
  patched, as they are under the rulings; the patch list's
  standing instruction, that the checklist verify the
  fallback path because a fallen-back kvmalloc frees
  through vfree semantics, is carried into the build as an
  open verification item. Trace: not separately
  instrumented; kv traffic is inside the Family 1 and
  (unarmed) Family 2 streams and cannot be separated from
  the inputs.
- devm family: devm_kzalloc, 6,897 sift occurrences, the
  largest single spelling in the wider surface. It lands on
  1.2 unchanged: drivers/base/devres.c `alloc_dr` (line
  110), compiled in, allocates its node through
  `kmalloc_node_track_caller`, one of the eight wrappers the
  dye test found the funnel expanded into. Disposition:
  redirected with Family 1; no separate center exists or is
  needed. Trace: inside the kmalloc event stream, not
  separable from the inputs.
- DMA coherent: dma_alloc_coherent, 1,872 sift occurrences,
  resolving through `dma_alloc_attrs` to
  `__dma_direct_alloc_pages` (kernel/dma/direct.c line 118),
  compiled in under CONFIG_HAS_DMA=y, which calls
  `alloc_pages_node`, Family 3; or, in CMA builds, to
  `cma_alloc`, which is absent here. Disposition: not
  redirected; DMA-coherent allocations in Fubuki are served
  through the substrate path, and are named here so the
  exception is explicit. This follows from rulings 1 and 5
  together with the measured CMA absence.

## Residuals (out of scope, named)

- memblock: the boot allocator, mm/memblock.c, funnel
  `memblock_alloc_internal` (line 1578, static). Compiled
  subset: compiled in (memblock.o, obj-y), marked there as
  pre-slab and out of runtime scope. Sift: the census file
  lists no memblock count. Trace: it is dead before the
  runtime test begins. Disposition: out of scope, named as
  the completeness boundary at the front of boot, as the
  patch list records. Its reservations are part of the
  boot's earlier chapter.
- vmap and ioremap: mm/vmalloc.c `vmap` (line 3496) maps
  pages the caller already owns; ioremap maps device
  memory. Neither allocates memory. `vm_map_ram` (line
  3038) uses the separate vmap_block sub-allocator over
  already-owned pages. Disposition: out of scope, address
  space only, flagged so the sift does not mistake them
  for missed centers. The vmalloc reservation half (2.2)
  stands in the same relation inside Family 2: stock by
  design, named in that family's row.

## Disagreements and open items, collected

1. Dye against sift at the funnel: 8 compiled expansions
   against 6 source call sites, accounted exactly (5 + 3).
   The instruments measure different quantities; neither is
   wrong, and no other center has dye evidence.
2. Sift against trace on Family 1: source mass and runtime
   mass invert (353 cache-door occurrences against 33,218
   generic occurrences in source; 706,419 cache events
   against 105,687 kmalloc events at runtime). The
   reconciliation follows the trace for load and the sift
   for coverage.
3. Unmeasured families: vmalloc, page-direct, oversize
   kmalloc (as a split), mempool's backing split, percpu,
   gen_pool, and the cross-center doors have no runtime
   counts in the inputs. Their rows rest on sift, patch
   list, and compiled subset only.
4. Absent figures: no sift count for krealloc or for
   gen_pool spellings; no trace split between named caches
   and kmalloc size caches inside the 706,419; no stated
   value of KMALLOC_MAX_CACHE_SIZE for this build.
5. Version skew: 6.12.116 evidence read against a 6.12.113
   configuration and kernel, as the compiled subset states.
6. The watch point: `alloc_slab_page` and whether SLUB
   remains as bookkeeping over Iscandar grants or is
   bypassed outright is settled by no input. It changes no
   family's center, and it is build-time design work still
   owed. CLOSED by Andrew's ruling of 2026-11-11: bypass
   outright. His ground: the pages serving translated
   requests are Iscandar allocations, so there is no second
   serving for SLUB to bookkeep. The evidence run's check:
   slab page allocations attributable to redirected caches
   are zero after the early-boot prefix; any that appear are
   a leak in the patch, a finding.
7. gen_pool's served set: in scope by ruling, unnamed by
   any input (see Family 4c).
8. The kv fallback verification, carried from the patch
   list into the build checklist.

## Verdict

Under the rulings, the live build will serve these families
at the named centers:

- The kmalloc spellings (1.1, 1.2), including the devm
  spellings that land there.
- Named caches (1.1, with 1.5 the setup point), crossing as
  requests under the named accommodation.
- The free doors (1.4), and krealloc (1.6) by composition.
- Oversize kmalloc (1.3), at its own center, no exemption.
- vmalloc (2.1, backing replaced at 2.3, free door 2.4),
  with its reservation half (2.2) stock by design.
- mempool (4.1), through its named backings.
- percpu (4.2).
- gen_pool (4.3), with its served set open as recorded.
- The kv family, through the Family 1 and Family 2 centers.

The live build will not serve these, each named as the
criterion requires:

- The page-direct family (3.1, 3.2): substrate by ruling,
  the build's principal declared exception.
- DMA-coherent allocations: served through the substrate
  path via the direct allocator.
- The early-boot prefix across all families: stock, logged,
  under ruling 4.
- memblock: out of scope, pre-slab.
- vmap and ioremap: out of scope, not allocations.
- CMA: absent from Fubuki's build; nothing exists to serve.

Count of the comparison: of the twelve family rows above,
nine end redirected at a named center (1a, 1b, 1c, 1d, 1e,
2, 4a, 4b, 4c), one ends substrate (3), one ends moot by
measured absence (4d), and the residuals stand out of scope
by design. Every family the sift or the trace brought
forward is accounted for in one of those endings; none is
left unmentioned.

Correction 2026-11-12 (at Andrew's stet): the version skew
stated in this artifact (item 5 of the recorded gaps, and the
inputs summary) names a 6.12.113 configuration and kernel.
The byte-check run on 2026-11-12 established that Fubuki's
kernel is 6.12.107+deb13-amd64 and its build configuration
the 6.12.107 config (see the correction note in
`compiled-subset-2026-11-10.md`). The skew therefore reads:
6.12.116 evidence read against a 6.12.107 configuration and
kernel, and the trace was recorded on Fubuki's 6.12.107
kernel. No family verdict in this artifact depends on the
patchlevel; the correction is to the label only. The text
above stands as the record of the run as performed.
