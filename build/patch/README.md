# The staged kernel patch set

Status: STAGED, 2026-11-11. These patches are written against
the allocator centers of Linux 6.12.116 exactly as enumerated
in `../paper-3-patch-points-DRAFT.md` (file, function, line)
and disposed in `../data/reconciliation-2026-11-10.md`. The
6.12.116 source tree is not on this machine, so the hunks are
staged in form: each is anchored on its center's definition
at the line the patch list records, and the inserted code is
the design's content, but the surrounding context lines are
schematic. On telesart, against the real tree, each patch is
re-based (context regenerated with git diff) before it is
applied; the bench run list in `../BUILD-STATUS.md` carries
that step. Nothing here has been applied to any tree, and
nothing here claims a compile.

## The set

- `0001-slab-centers.patch`: mm/slub.c. The redirect at 1.1
  (`slab_alloc_node`), the oversize door 1.3
  (`__kmalloc_large_noprof`) patched as its own center, the
  free doors 1.4 (`kfree`, `kmem_cache_free`,
  `kmem_cache_free_bulk`), and the watch-point
  instrumentation at `alloc_slab_page` (a counter only: SLUB
  is bypassed outright for redirected caches by Andrew's
  ruling of 2026-11-11, so the counter exists to prove the
  bypass, slab page allocations attributable to redirected
  caches at zero after the early-boot prefix).
- `0002-cache-setup.patch`: mm/slab_common.c. Cache setup
  1.5 (`__kmem_cache_create_args`, `kmem_cache_destroy`):
  creation registers the cache with the layer under the cap;
  destruction deregisters it. Center 1.6
  (`krealloc_noprof`) has no hunk: it follows by composition
  (see MANIFEST.md).
- `0003-vmalloc-backing.patch`: mm/vmalloc.c. The backing
  step 2.3 (`vm_area_alloc_pages`) replaced with one
  exact-fit bridge run; the free door 2.4 (`vfree`). The
  reservation half 2.2 is not patched, by design (see
  NOT-PATCHED.md).
- `0004-second-ring.patch`: mm/percpu.c center 4.2
  (`pcpu_alloc_noprof`) and lib/genalloc.c center 4.3
  (`gen_pool_alloc_algo_owner`, gated on pool registration:
  the reconciliation records gen_pool's served set as open,
  and the gate is where the build names it). Center 4.1
  (mempool) has no hunk of its own: it is served through its
  backings, exactly as the reconciliation disposes it.
- `0005-bridge-core.patch`: the kernel side of the bridge:
  a new core (mm/iscandar_bridge.c) plus its glue header.
  The core owns the channel (the region reserved in the
  compile-time map, its address handed in as a boot
  parameter), the synchronous request path, the arena's
  kernel-space mapping of the reserved range (so a bridge
  offset becomes a kernel pointer by addition, the address
  form of the map's cube correspondence), the cache pointer
  to registry id table filled at 1.5, the early-boot prefix
  machinery (before the layer raises, centers pass stock and
  are counted and logged, ruling 4), and /proc/iscandar_bridge
  carrying the counters, including the watch counter.

## Rules the hunks obey

- No silent fallback. A bridge refusal at a patched center
  is the verdict: the door returns its failure shape (NULL
  for the allocation doors). Nothing patched serves stock
  quietly after the prefix. The only stock path after the
  prefix is the free doors' range decision: a pointer
  outside the arena was born outside it (the prefix's
  objects, the substrate's own) and frees through the stock
  body, which is the design's named seam, not a fallback.
- The early-boot prefix is bounded by the layer's raise:
  the layer publishes the channel control block and raises
  the ready flag through /proc/iscandar_bridge; before that
  moment every center is stock, counted per center in the
  control block and logged into the prefix ring.
- The shared header is copied, never retyped: application
  copies `build/bridge/iscandar_bridge.h` into the tree as
  include/iscandar_bridge.h (the bench run list carries the
  command). No bridge constant is defined in any patch.
