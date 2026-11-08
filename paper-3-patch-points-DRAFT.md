# Paper 3 patch points: DRAFT

**What this is.** The enumerated list of allocator centers to patch for Paper 3
("Debian's Voyage To Iscandar"), so that a live Debian (Fubuki, ELBE-built from
stock Debian trixie sources) runs its allocations through Iscandar under QEMU.

**Source.** Linux 6.12.116 (kernel.org tarball), the current point release of
the 6.12 series Fubuki's kernel came from. Every function name below was
verified by grep against that tree on 2026-11-08; line numbers are those of
6.12.116.

**Method (Andrew's formulation).** The recorded boot trace is all we need as
the control. Compare it against the source, and patch there: at the allocator
centers, the core functions all API spellings funnel into, not at individual
call sites.

**Trace evidence.** The Paper 2 control trace (Fubuki boot, kmem family armed
from t=0) recorded: kmalloc 105,687 events; kmem_cache_alloc 706,419; kfree
166,390; kmem_cache_free 671,928; total allocations 812,106. The vmalloc and
page-direct families were not armed in that trace, so for those families this
list rests on the source sift (census of 2026-11-07) until a second trace arms
them.

**Config scope.** The exact Fubuki kernel config is not in the workspace. The
ELBE build used the stock Debian trixie kernel package (the build sheet
records linux-image 6.12.113+deb13-amd64, with TRACING, EVENT_TRACING,
FTRACE and TRACEPOINTS all =y, checked from the .deb); the build exports the
config as a `telezart-kconfig` artifact, but that artifact lives with the
build output, not here. This list is therefore scoped to the core centers,
which are config-independent: the funnel functions exist in every 6.12 build.
Enumerating the compiled subset against the exact config is listed under
Remaining work. Debian's kernel uses SLUB, and 6.12 offers no other full
allocator (mm/Kconfig: SLUB, or SLUB_TINY for small systems; the old SLAB
allocator is gone from the tree).

**A naming fact that shapes the whole list.** In 6.12, allocation profiling
has renamed the doors: nearly every allocation function is a `_noprof` symbol
reached through an `alloc_hooks()` macro, so `kmalloc` in source text is
`kmalloc_noprof`, and the core is `__kmalloc_noprof`. The free doors are the
exception: `kfree`, `kmem_cache_free`, `vfree`, `mempool_free` and
`free_pages` keep their plain names. Patch references must use the names as
they appear in 6.12.116, below.

---

## Family 1: kmalloc / slab core (first ring; the traced family)

Spelling census (whole tree): kmalloc 5,501; kzalloc 21,808; kcalloc 4,806;
kmalloc_array 1,036; kmalloc_node 67; kmem_cache_alloc 329;
kmem_cache_alloc_node 24; kmem_cache_create 464; kfree 40,597; kvfree 1,982;
kmem_cache_free 963. Plus devm_kzalloc 6,897, which funnels into this family
(see Cross-center doors).

Trace: this is the family the control trace measured (counts in the header).
Note the shape: the cache door dominates the boot (kmem_cache_alloc 706,419
of 812,106 allocations), so the patch must serve named caches, not only the
generic kmalloc sizes.

Inline dispatch (include/linux/slab.h): `kmalloc_noprof` (line 885,
__always_inline) sends a compile-time-constant size to
`__kmalloc_cache_noprof` on a cache from `kmalloc_caches[kmalloc_type][index]`,
an oversize constant to `__kmalloc_large_noprof`, and any other size to
`__kmalloc_noprof`. `kmalloc_node_noprof` (line 1007) mirrors this with
`__kmalloc_cache_node_noprof`, `__kmalloc_large_node_noprof` and
`__kmalloc_node_noprof`. `kzalloc_noprof` (line 1127) is kmalloc with
__GFP_ZERO; `kcalloc` (line 1079) is a macro for kmalloc_array with
__GFP_ZERO; `kmalloc_array_noprof` (line 1030) overflow-checks and calls
kmalloc_noprof.

| # | Patch point (file, function) | What funnels into it | Patch action |
|---|---|---|---|
| 1.1 | mm/slub.c, `slab_alloc_node` (line 4205) | Every slab allocation: `kmem_cache_alloc_noprof` (4236), `kmem_cache_alloc_node_noprof` (4281), `kmem_cache_alloc_lru_noprof` (4247), `__kmalloc_cache_noprof` (4390), `__kmalloc_cache_node_noprof` (4402), and `__do_kmalloc_node` below. Slow path beneath it: `__slab_alloc` (3989) and `___slab_alloc` (3737). | The redirect point. Allocation requests are served through the Iscandar translation layer here. |
| 1.2 | mm/slub.c, `__do_kmalloc_node` (line 4347) | The variable-size kmalloc door: `__kmalloc_noprof` (4376), `__kmalloc_node_noprof` (4370), `__kmalloc_node_track_caller_noprof` (4382). devm allocations land here too (see Cross-center doors). | Covered by 1.1 (it calls slab_alloc_node), listed separately because the slab.h dispatch reaches the funnel by this second route; size and flags translation is checked here. |
| 1.3 | mm/slub.c, `__kmalloc_large_noprof` (line 4326), via `___kmalloc_large_node` | kmalloc spellings whose size exceeds KMALLOC_MAX_CACHE_SIZE (constant path in slab.h, and large sizes out of `__do_kmalloc_node`). **These never touch slab_alloc_node**: the body calls `alloc_pages_noprof` / `__alloc_pages_noprof` directly. | Patch separately, or the design records that oversize kmalloc keeps page-allocator backing. Unpatched, this is a hole in the redirect exactly where the largest requests pass. |
| 1.4 | mm/slub.c, `kfree` (line 4817) and `kmem_cache_free` (line 4785) | The two free doors for the whole family (trace: kfree 166,390; kmem_cache_free 671,928). Both funnel internally to `do_slab_free` / `__slab_free` (4487). Bulk variants: `kmem_cache_free_bulk` (4945). | The return door: frees run the inverse translation and hand the run back. |
| 1.5 | mm/slab_common.c, `__kmem_cache_create_args` (line 297) | Cache creation: the `kmem_cache_create` macro in slab.h (line 414) and its usercopy/rcu variants resolve here. Census: kmem_cache_create 464 sites. Teardown door: `kmem_cache_destroy` (506). The kmalloc size caches themselves are built at init by `create_kmalloc_caches` (983). | Setup-time point, not a runtime redirect: this is where the design states how cache geometries (object size, alignment, constructors) are represented when 1.1 serves a named cache. |
| 1.6 | mm/slab_common.c, `krealloc_noprof` (line 1308) | `krealloc` spelling (slab.h line 454). Composes an allocation, a copy and a free; `kfree_sensitive` (1336) is a free-side variant. | Follows automatically if 1.1 and 1.4 are patched; listed so the checklist can mark it verified rather than assumed. |

Watch point, not a patch point: mm/slub.c, `alloc_slab_page` (line 2470),
where SLUB itself draws backing pages from the page allocator
(`alloc_pages` / `__alloc_pages_node`). If the slab centers are redirected,
SLUB's own page demand must not be served twice; the design states whether
SLUB remains as bookkeeping over Iscandar grants or is bypassed outright.

## Family 2: vmalloc (first ring by the design's own attack plan)

Spelling census: vmalloc 924; vzalloc 976; vfree 3,436. Trace: not armed.

Unlike kmalloc, nothing here is inlined into callers: every spelling
(`vmalloc_noprof` line 4007, `vzalloc_noprof` line 4047, `__vmalloc_noprof`
line 3988, `__vmalloc_node_noprof` line 3973, all in mm/vmalloc.c, behind the
macros in include/linux/vmalloc.h lines 144-176) funnels into one center.

| # | Patch point (file, function) | What funnels into it | Patch action |
|---|---|---|---|
| 2.1 | mm/vmalloc.c, `__vmalloc_node_range_noprof` (line 3826) | All vmalloc spellings. | The center. The two steps below are its two halves. |
| 2.2 | mm/vmalloc.c, `__get_vm_area_node` (line 3168), via `alloc_vmap_area` (line 2008) | The virtual-range reservation step inside 2.1. | **No patch.** The reservation stays stock, per the design: Iscandar replaces the backing, not the kernel's address-space bookkeeping. |
| 2.3 | mm/vmalloc.c, `vm_area_alloc_pages` (line 3594), called from `__vmalloc_area_node` (line 3683) | The backing step inside 2.1: page-by-page allocation via `alloc_pages_bulk_array_mempolicy_noprof`, `alloc_pages_noprof`, `alloc_pages_node_noprof`, then mapping via `vmap_pages_range` (line 712). | The patch site: replace the page-by-page backing with one exact-fit Iscandar run and map the arena's frames into the reserved range. At the standing calibration (one sector = one page), an N-page request is an N-sector request. |
| 2.4 | mm/vmalloc.c, `vfree` (line 3407); `vfree_atomic` (line 3373) | The free door for the family. | Runs the inverse bijection and returns the run. |

Note: `__vmalloc_area_node` allocates the page-pointer array for an area by
recursively calling `__vmalloc_node_noprof`, so one vmalloc allocation
contains a small hidden slab allocation. The translation layer's bookkeeping
must expect vmalloc traffic to touch Family 1.

## Family 3: page allocator (second ring; design-scope decision)

Spelling census: alloc_pages 581; __get_free_pages 175; free_pages 1,107.
Trace: not armed.

| # | Patch point (file, function) | What funnels into it | Patch action |
|---|---|---|---|
| 3.1 | mm/page_alloc.c, `__alloc_pages_noprof` (line 4810); slow path `__alloc_pages_slowpath` (line 4276) | Every page allocation: the gfp.h inlines `alloc_pages_noprof` (include/linux/gfp.h line 315), `alloc_pages_node_noprof`, `__alloc_pages_node_noprof`, `get_free_pages_noprof` (mm/page_alloc.c line 4889), and the internal callers from Families 1 and 2 (points 1.3, 2.3, and SLUB's `alloc_slab_page`). | Second ring. Not in the first patch set: every first-ring center ultimately draws pages here, so patching here as well would serve the same memory twice. The design decision to record: the page allocator stays as the substrate under the redirected centers, or Iscandar's arena is carved out at boot and this center is left stock outside the arena. |
| 3.2 | mm/page_alloc.c, `__free_pages` (line 4926) and `free_pages` (line 4942) | The page free doors. | Follows the 3.1 decision. |

## Family 4: side centers (second ring)

Trace: none of these were armed. Each is a reserve or a special-purpose
allocator that stands on a Family 1 or Family 3 backing, so each follows its
backing unless the design names it separately.

| # | Patch point (file, function) | What funnels into it | Patch action |
|---|---|---|---|
| 4.1 | mm/mempool.c, `mempool_alloc_noprof` (line 404); free door `mempool_free` (line 518) | mempool_alloc spelling (census 300). A mempool is a guaranteed-reserve wrapper: elements come from `mempool_alloc_slab` (line 575, which is kmem_cache_alloc: Family 1) or `mempool_alloc_pages` (line 624, which is alloc_pages: Family 3). | No patch point of its own; it inherits whichever backing it was created with. Listed so a mempool over slab is verified under 1.1, not assumed. |
| 4.2 | mm/percpu.c, `pcpu_alloc_noprof` (line 1743) | The per-CPU allocator: `alloc_percpu` and `__alloc_percpu_gfp` are macros in include/linux/percpu.h (line 143) that resolve here, not to a function of their own name. Census: alloc_percpu 291. Chunk backing is created by `pcpu_create_chunk` (declared line 1580). | Second ring. Per-CPU areas are allocated in chunks and carved internally; redirecting here is a separate design question from the boot-load families. |
| 4.3 | lib/genalloc.c, `gen_pool_alloc_algo_owner` (line 277); free door `gen_pool_free_owner` (line 487) | The generic pool allocator: `gen_pool_alloc` is a header inline (include/linux/genalloc.h line 152) over `gen_pool_alloc_algo`. The exported centers carry the `_owner` suffixes in 6.12. | Out of the arena's remit unless a specific pool is implicated: genalloc manages device and special memory pools, not general system RAM. Listed for the completeness boundary of the paper. |
| 4.4 | mm/cma.c, `cma_alloc` (line 515); free door `cma_release` (line 562) | The contiguous memory area allocator, backing via `alloc_contig_range` (line 456). Reached in practice from the DMA path (below). | Second ring, decided with the DMA question. |

## Cross-center doors (no patch points of their own)

- **kv family.** mm/util.c, `__kvmalloc_node_noprof` (line 650): tries
  `__kmalloc_node_noprof` first (Family 1) and falls back to
  `__vmalloc_node_range_noprof` (Family 2). Spellings: kvmalloc, kvzalloc,
  kvcalloc; free door `kvfree` (census 1,982) chooses by address range. No
  patch needed if both centers are patched, but the checklist must verify
  the fallback path, because a kvmalloc that fell back frees through vfree
  semantics.
- **devm family.** drivers/base/devres.c, `alloc_dr` (line 110) allocates
  its bookkeeping node by calling `kmalloc_node_track_caller` (line 119),
  behind `devm_kmalloc` (line 829) and the devm_kzalloc spelling (census
  6,897). It lands on point 1.2 unchanged: no separate center exists.
- **DMA coherent.** dma_alloc_coherent (census 1,872) resolves through
  `dma_alloc_attrs` to `__dma_direct_alloc_pages` (kernel/dma/direct.c line
  118), which calls `alloc_pages_node` (Family 3), or to `cma_alloc`
  (point 4.4) for contiguous allocations. Whether DMA belongs in the
  redirected set is the same decision as 3.1 and 4.4.

## Out of runtime scope

- **memblock.** mm/memblock.c: the boot allocator. Its funnel is
  `memblock_alloc_internal` (line 1578, static), behind
  `memblock_alloc_try_nid_raw` (line 1653) and `memblock_alloc_try_nid`
  (line 1683). Pre-slab: it is dead before the runtime test begins, and its
  reservations are part of the boot's earlier chapter. Recorded here so the
  paper's completeness claim can name its boundary.
- **vmap and ioremap.** mm/vmalloc.c, `vmap` (line 3496) maps pages the
  caller already owns: address space only, no memory allocated. ioremap maps
  device memory. Neither is an allocation, and neither is redirected.
  `vm_map_ram` (line 3038) uses the separate vmap_block sub-allocator
  (`vb_alloc`, line 2818) over already-owned pages; same boundary, flagged
  so the sift does not mistake it for a missed center.

---

## Remaining work

1. **Compiled-subset enumeration.** Run the sift against the subset Fubuki's
   exact config compiles (the `telezart-kconfig` artifact from the ELBE
   build output, or the stock trixie config it came from) and mark each
   patch point above as compiled-in, modular, or absent for this build.
   Until then the list is scoped to the config-independent core centers.
2. **Inline-site audit.** The constant-size kmalloc spellings are inlined
   into callers as `__kmalloc_cache_noprof` calls on the kmalloc caches.
   The center patch covers them by construction; the audit (compiler
   inlining reports per translation unit, binary spot checks) is the
   evidence that no spelling bypasses the centers.
3. **A vmalloc-armed trace.** The control trace armed only the kmem family.
   If Family 2 is to carry trace evidence equal to Family 1's, a second
   Fubuki boot trace must arm the vmalloc path (or the patch-center logging
   proposed in the design must substitute for it).
4. **Design decisions this list waits on:** the page-allocator question at
   3.1 (substrate or bypass); the oversize-kmalloc ruling at 1.3; how cache
   geometries are represented at 1.5; the second-ring scope (4.1 to 4.4 and
   DMA); and the early-boot ordering for vmalloc (arena standing before the
   first redirected call, or the earliest calls passing through stock and
   logged as such).

*DRAFT, prepared 2026-11-08. Nothing patched; no repository created. The
list becomes final when Andrew rules on the decisions in item 4 above.*
