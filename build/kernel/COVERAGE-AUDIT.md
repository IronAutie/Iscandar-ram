# Coverage audit

Under the design's R10: every allocation entry point
in the source enumerated, each marked door'd or
deliberately stock, with its free twin beside it, and
the bulk, node, and lru variants named explicitly.
The enumeration was carried from the bridge round's
reconciliation (build/patch/MANIFEST.md and
NOT-PATCHED.md, function line numbers against
6.12.116) and is now closed against the kernel tree
itself: the closure reads were run at the bench on
2026-11-20, following BENCH-PACKET-AUDIT-CLOSURE.md,
and their results are folded in below. This document
supersedes COVERAGE-AUDIT-DRAFT.md. With this
re-issue the audit is closed and verification step 3
is done. Two items carry runtime halves that were
always guest work (the gen_pool tests T1 to T6 and
the kvmalloc tests K1 to K3); they ride into
verification step 5 as this audit's recorded
expectations, and their items close in the guest, as
their own text states.

Status: STETTED by Andrew, 2026-11-20. This audit is
the coverage record of the kernel round.

## Door'd, with their free twins

| Alloc entry | Free twin | Form |
|---|---|---|
| `slab_alloc_node` (1.1) | `kfree`, `kmem_cache_free`, `kmem_cache_free_bulk` (1.4) | door at the center; the bulk free is named here, not assumed |
| `__do_kmalloc_node` (1.2) | (same 1.4 set) | covered by 1.1, which it calls; the node variant |
| `__kmalloc_large_noprof` (1.3) | (same 1.4 set) | its own center, no exemption |
| `__vmalloc_node_range_noprof` (2.1), backing at `vm_area_alloc_pages` (2.3) | `vfree`, `vfree_atomic` (2.4) | backing replaced with one exact-fit run; the reservation half stays stock (below) |
| `mempool_alloc_noprof` (4.1) | `mempool_free` | by backing: over slab it is redirected at 1.1 with its elements; over pages it is substrate |
| `pcpu_alloc_noprof` (4.2) | `free_percpu` (door added in the bridge round, patch 0004) | door at the center under the uniprocessor precondition |
| `gen_pool_alloc_algo_owner` (4.3) | `gen_pool_free_owner` | gated on pool registration; the served set is settled by the census below |
| `__kvmalloc_node_noprof` (cross) | (falls to the 1.4 set or 2.4 by path) | covered if both its centers are door'd, as they are; its free composition is verified below |
| `krealloc_noprof` (1.6) | (by composition) | follows if 1.1 and 1.4 hold; listed so its coverage is verified, not assumed |
| `alloc_dr` (cross) | (same) | lands on 1.2 unchanged; no separate center exists or is needed |

Control path, not a door pair: `__kmem_cache_create_args`
and `kmem_cache_destroy` (1.5), the registry's writers,
under the cap, in process context.

Instrumentation only, not a door: `alloc_slab_page`
(the bypass counter, Andrew's ruling 2026-11-11).

## Deliberately stock, named

- Page-direct: `__alloc_pages_noprof`, `__free_pages`,
  `free_pages` (3.1, 3.2). The substrate: the page
  allocator stays underneath as the bulk supplier and
  parcels nothing for callers. The build's principal
  declared exception, twins named together.
- DMA-coherent: `__dma_direct_alloc_pages`, served
  through the substrate path.
- vmalloc reservation half: `__get_vm_area_node`,
  `alloc_vmap_area` (2.2). Address-space bookkeeping,
  never part of the replacement; only the backing is
  Iscandar's.
- memblock: `memblock_alloc_internal`. Pre-slab, out of
  scope; in this build the engine's own storage never
  passes through it at all (the marked mass, R8).
- `vmap`, `vm_map_ram`: address space only, no memory
  allocated.
- `cma_alloc`: absent from the build (CONFIG_CMA not
  set, measured in the bridge round).
- The early-boot prefix, across all families: stock,
  counted by the prefix counters, closed by the ready
  flag (R11). A named, bounded prefix, not an
  exception.

## The uniprocessor note (Andrew, 2026-11-19)

There should only be one CPU in the per-CPU freelist.
Under the build's uniprocessor precondition (SMP
disabled, the config byte-checked in the bridge round),
the per-CPU freelist array has exactly one live entry:
CPU 0's freelist is the only one that can ever be
populated or drained. Consequences for this audit: the
alloc_bulk drain and the door'd center walk one and
the same freelist, so the funnel loses nothing but
loop overhead; and no cross-CPU freelist exists that
could serve an allocation behind the center's back,
which bounds the closure enumeration to this CPU's
machinery. The note stands or falls with the
precondition, which is already stated in the build and
the manuscript's scope.

## The closure enumeration (run 2026-11-20)

The fast-path class was closed by enumeration, as the
draft's method specified: grep mm/slub.c for the
per-CPU freelist accesses (`c->freelist`,
`get_freepointer`, `->cpu_slab`). The count was taken
first (96 matching lines; the first page of output
filled its 80-line cap exactly, and the remainder was
paged, so the set below is the whole set). Every site
lands in one of these families:

- The accessor pair: `get_freepointer` (503) and
  `get_freepointer_safe` (532). The primitive the
  other sites use, not a path of its own.
- The fast path: the freelist exchange at 3660 and
  its slowpath at 4037 to 4064, with the tid
  bookkeeping at 3068 and 3109.
- The free paths: the slab freelist writes at 2820
  and 2852, and the locked free and bulk-free region
  at 3790 to 3870.
- The partial and flush drains: 3253 to 4011.
- The alloc_bulk drain: 5056 to 5108, inside
  `__kmem_cache_alloc_bulk`. The funnel target
  (item 2 below).
- The control path: `cpu_slab` allocated at cache
  creation (5329 to 5332) and freed at cache
  destruction (5407 to 5410), item 1.5.
- The early-boot prefix: the node cache's freelist
  at 5378.
- Debug and validation walks: 813, 1083, 1415.
  Readers, not serving paths.
- Inspection: 6522, the partial-list readers at
  6753 and 6768, and the statistics readers at 6965
  and 6991. Witnesses, not doors.

No site falls outside these families. A name that
landed nowhere would have been the finding the
enumeration exists to catch; there is none.

## Items closed at the bench, 2026-11-20

1. `kmem_cache_alloc_lru`: CLOSED. Read in the tree
   at mm/slub.c:4277: the body is one call,
   `slab_alloc_node(s, lru, gfpflags, NUMA_NO_NODE,
   _RET_IP_, s->object_size)`, plus a trace line. The
   lru travels as the center's own parameter and the
   function touches no freelist of its own, so the
   1.1 door covers it. The desk reading is confirmed
   line for line.
2. `kmem_cache_alloc_bulk`: READ AND SETTLED; the
   funnel hunk itself is kernel-round work.
   Two corrections to the draft's desk reading,
   from the tree. First, there is no
   `alloc_from_pcs` helper in 6.12.116: the drain is
   inlined in `__kmem_cache_alloc_bulk`. Second, the
   tree carries two bodies under CONFIG_SLUB_TINY,
   and the build's config settles which is live:
   CONFIG_SLUB=y with SLUB_TINY not set (read from
   the build's .config at the bench), so the
   full-SLUB body (mm/slub.c 5044 to 5110) is the
   live one. In that body the drain is private, as
   charged: under the cpu_slab lock with IRQs off it
   pops `c->freelist` through `get_freepointer`,
   falling back to `___slab_alloc` only when the
   list runs dry, with kfence checked first per
   iteration. The SLUB_TINY body (5114) is already a
   plain loop over `__slab_alloc_node`, a funnel by
   construction, but it is not compiled here.
   The contract the hunk must preserve is also
   sharper than the draft stated: the bulk call is
   all-or-nothing. On any failure the error path
   frees the partial fill through
   `__kmem_cache_free_bulk` (the door'd 1.4 twin)
   and returns 0. Treatment stands as RULED
   (Andrew, 2026-11-19): funnel it. The private
   drain is replaced by a loop over the door'd
   center, kfence first, the all-or-nothing error
   path unchanged.
3. The gen_pool served set (4.3): the census is
   RECORDED; the item closes when T1 to T6 pass in
   the guest, as ruled. The tree-wide census
   (`gen_pool_create` and `gen_pool_add`, excluding
   lib/genalloc itself) totals 150 lines. Sorted
   against the build, the creators are: other
   architectures' SRAM and TCM pools (powerpc, sh,
   csky, arm), not compiled; test mocks under
   tools/testing; build artifacts (.vmlinux.export.c
   and .mod.c symbol tables); and driver pools for
   hardware the guest does not carry (habanalabs,
   ivpu, cxgb4, Freescale QE/QBMan and FMan, s390
   CIO, Stratix10, QCOM, TI K3 and DaVinci, Mellanox,
   CAIF, TEE, PCI P2P), not compiled. One compiled
   pool is device-backed and stays stock by the
   backing rule: the USB host-controller localmem
   pool (drivers/usb/core/hcd.c). The compiled,
   RAM-backed creators, each confirmed by its object
   file in the build, are exactly four:
   - kernel/dma/pool.c, the DMA atomic pool, backed
     by allocated pages;
   - arch/x86/kernel/cpu/mce/genpool.c, the MCE
     pool, backed by a static buffer;
   - drivers/acpi/apei/ghes.c, the GHES estatus
     pool, backed by an allocated buffer;
   - drivers/edac/igen6_edac.c, the igen6 ecclog
     pool, backed by a static buffer.
   These four are the served set's candidates under
   the backing rule (Andrew, 2026-11-19: a pool is
   served only if its memory is ordinary system RAM,
   decided by the gate's test on each chunk as it is
   added, whole-chunk conservatism; Andrew's
   condition: more runtime tests to verify the
   choice). The tests, to run in the guest during
   verification step 5: T1 a test pool over ordinary
   RAM is decided served; T2 a test pool over a
   reserved range (the channel span is the standing
   example) is decided stock; T3 a chunk straddling
   the boundary is decided stock; T4 allocations
   from a served test pool land in the arena, free
   clean, and move the engine's books by exactly the
   granted amount; T5 allocations from a stock test
   pool land outside the arena and the books do not
   move; T6 the state read lists the pools and their
   decisions, and the bench compares the list
   against this census, any disagreement a finding.
4. The kvmalloc fallback path's free semantics: the
   record half is VERIFIED IN THE TREE; the runtime
   half rides to verification step 5. Read at the
   bench: `__kvmalloc_node_noprof` at mm/util.c:650,
   and `kvfree` at 696, whose body is the partition
   itself, `is_vmalloc_addr(addr)` sending the
   address to `vfree` and everything else to `kfree`
   (`kvfree_sensitive` at 714 rides the same
   partition). The two door populations are
   separated by address range (the arena's mapping
   never falls in the vmalloc VA range), so the test
   partitions them exactly; and a misroute would be
   loud, not silent, since the vfree door's arena
   test rejects a direct-map pointer. The
   address-range half of the verification runs on
   the kernel round's linked vmlinux, the arena
   section's addresses read against the vmalloc
   range, before the first boot. Runtime tests, in
   the guest: K1 a small kvmalloc (the kmalloc path)
   freed through kvfree returns exactly once; K2 a
   large kvmalloc (the vmalloc path) likewise; K3
   the books after both: grants and frees balance,
   leaked frees 0, violations 0. Held in reserve if
   any test misroutes: door kvfree itself, the free
   door applying the arena test before kvfree's
   branch. Treatment as RULED (Andrew, 2026-11-19):
   verify the composition. The item closes when K1
   to K3 pass.
5. The `free_percpu` door: CLOSED. Re-read in the
   tree alongside its alloc twin. `pcpu_alloc_noprof`
   at mm/percpu.c:1744 carries the door at its head,
   gated on `iscandar_active() && !reserved`, with
   the uniprocessor ruling's comment, the mirrored
   rejection set, atomic callers refused, and the
   prefix note below it on the stock path.
   `free_percpu` at mm/percpu.c:2273 carries its
   door above the chunk search: on
   `iscandar_ptr_in_arena(ptr)` it sends
   ISCANDAR_OP_PERCPU_FREE through the bridge and
   returns, so a bridge-born token never reaches
   `pcpu_chunk_addr_search`; stock pointers fall
   through to the chunk machinery unchanged. Both
   doors sit where patch 0004 placed them.

## What remains open after this re-issue

Named, and none of it is audit work: the kernel
round's patch set itself, including the alloc_bulk
funnel hunk (item 2), written against the stetted
design; the kvmalloc address-range read on the
round's linked vmlinux, before the first boot
(item 4); and the runtime tests T1 to T6 and K1 to
K3 in the guest, at verification step 5 (items 3
and 4). The audit is closed; verification step 3 is
done.
