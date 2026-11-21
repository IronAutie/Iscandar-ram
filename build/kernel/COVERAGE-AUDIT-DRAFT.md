# Coverage audit, draft (desk side)

SUPERSEDED 2026-11-20 by COVERAGE-AUDIT.md, re-issued
after the bench closure reads of that date. This file
is kept as the desk-side record the re-issue was
checked against.

Under the design draft's R10: every allocation entry
point in the source enumerated, each marked door'd or
deliberately stock, with its free twin beside it, and
the bulk, node, and lru variants named explicitly. The
enumeration below is carried from the bridge round's
reconciliation (build/patch/MANIFEST.md and
NOT-PATCHED.md, function line numbers against
6.12.116). Status: DRAFT. The audit closes only against
the kernel tree itself, at the bench, before the kernel
build's first boot (verification order, step 3). Items
whose coverage the desk record does not settle are
listed as open, by name, at the end.

## Door'd, with their free twins

| Alloc entry | Free twin | Form |
|---|---|---|
| `slab_alloc_node` (1.1) | `kfree`, `kmem_cache_free`, `kmem_cache_free_bulk` (1.4) | door at the center; the bulk free is named here, not assumed |
| `__do_kmalloc_node` (1.2) | (same 1.4 set) | covered by 1.1, which it calls; the node variant |
| `__kmalloc_large_noprof` (1.3) | (same 1.4 set) | its own center, no exemption |
| `__vmalloc_node_range_noprof` (2.1), backing at `vm_area_alloc_pages` (2.3) | `vfree`, `vfree_atomic` (2.4) | backing replaced with one exact-fit run; the reservation half stays stock (below) |
| `mempool_alloc_noprof` (4.1) | `mempool_free` | by backing: over slab it is redirected at 1.1 with its elements; over pages it is substrate |
| `pcpu_alloc_noprof` (4.2) | `free_percpu` (door added in the bridge round, patch 0004) | door at the center under the uniprocessor precondition |
| `gen_pool_alloc_algo_owner` (4.3) | `gen_pool_free_owner` | gated on pool registration; the served set is an open item below |
| `__kvmalloc_node_noprof` (cross) | (falls to the 1.4 set or 2.4 by path) | covered if both its centers are door'd, as they are; its fallback path's free semantics are an open item below |
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

## Open items, named for the tree-side closure

1. `kmem_cache_alloc_lru`: the lru variant. Desk-read
   from the public sources (the patch record and the
   common slab code): it is a thin wrapper over
   slab_alloc_node, the lru travelling as the center's
   own parameter, so the 1.1 door covers it. Status:
   read, not assumed; to be re-verified line for line
   against 6.12.116 at the bench before closure.
2. `kmem_cache_alloc_bulk`: the bulk alloc side.
   Desk-read: it carries its own drain over the
   per-CPU freelist (alloc_from_pcs_bulk under
   kmem_cache_alloc_bulk_noprof in this era of the
   code) and does not funnel into 1.1 for the objects
   it drains. Its free twin is already door'd (1.4).
   Treatment RULED (Andrew, 2026-11-19): funnel it.
   The private drain is replaced by a loop over the
   door'd center, one call per object, the bulk
   semantics (fill what can be filled, report the
   count) unchanged. What remains open on this item is
   execution only: re-verification against the tree
   and the hunk itself, which is kernel-round work.
3. The gen_pool served set (4.3). Treatment RULED
   (Andrew, 2026-11-19): the backing rule. A pool is
   served only if its memory is ordinary system RAM,
   decided by the gate's test on each chunk as it is
   added; a chunk not wholly in ordinary RAM is stock,
   whole-chunk conservatism. Andrew's condition on the
   ruling: more runtime tests to verify the choice.
   The tests, to run in the guest during the kernel
   round's bench verification (the rule's input, what
   is ordinary RAM, is a runtime fact of the guest's
   memory map and cannot be desk-checked):
   T1 a test pool over ordinary RAM is decided served;
   T2 a test pool over a reserved range (the channel
   span is the standing example) is decided stock;
   T3 a chunk straddling the boundary is decided
   stock; T4 allocations from a served test pool land
   in the arena, free clean, and move the engine's
   books by exactly the granted amount; T5 allocations
   from a stock test pool land outside the arena and
   the books do not move; T6 the state read lists the
   pools and their decisions, and the bench compares
   the list against this audit's expectation, any
   disagreement a finding. The audit's gen_pool item
   closes when T1 to T6 pass in the guest.
4. The kvmalloc fallback path's free semantics.
   Treatment RULED (Andrew, 2026-11-19, on the desk
   recommendation): verify the composition. Desk half:
   the two door populations are separated by address
   range (the arena's mapping never falls in the
   vmalloc VA range), so kvfree's is_vmalloc_addr test
   partitions them exactly; and a misroute would be
   loud, not silent, since the vfree door's arena test
   rejects a direct-map pointer. Runtime half, in the
   guest: K1 a small kvmalloc (the kmalloc path) freed
   through kvfree returns exactly once; K2 a large
   kvmalloc (the vmalloc path) likewise; K3 the books
   after both: grants and frees balance, leaked frees
   0, violations 0. Held in reserve if any test
   misroutes: door kvfree itself, the free door
   applying the arena test before kvfree's branch.
   The item closes when K1 to K3 pass.
5. The `free_percpu` door: carried from patch 0004 as
   built; to be re-verified against the tree alongside
   its alloc twin (item 4.2 above), since the audit
   trusts nothing it has not re-read.

Closure method for the fast-path class as a whole: a
private fast path exists only where code touches the
per-CPU freelist directly, so the set of carriers is
enumerable, not open-ended. Grep the tree for the
freelist accesses; every function returned is a
center, door'd, funneled, or named in this document.
The same enumeration applies per allocator family.
When that enumeration is run against the tree, items
1 to 5 are read and settled, and this table is
re-issued without the word draft, the audit is closed
and verification step 3 is done.
