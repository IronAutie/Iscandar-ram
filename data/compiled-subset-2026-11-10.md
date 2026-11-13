# The compiled subset (measured)

The compiled-subset enumeration for Paper 3, run 2026-11-10.
Status: MEASURED. Points from: The Compiled Subset.

## Method and version statement

Each patch point in `../paper-3-patch-points-DRAFT.md` was mapped
to the object file that compiles it and the CONFIG symbol that
gates that object, using the Makefiles of Linux 6.12.116 (the
tree the patch list was verified against). The symbols were then
read from Fubuki's kernel configuration,
`fubuki-kernel-config-6.12.113-deb13-amd64.txt` in this directory
(the stock trixie config of linux-image 6.12.113+deb13-amd64).
The version difference is stated plainly, as the composition
ledger requires: the patch list and the gating evidence are
6.12.116; the configuration is 6.12.113.

A point is **compiled in** when its object is built into the
kernel (obj-y, or obj-$(CONFIG) with the symbol set to y),
**modular** when the symbol is m, and **absent** when the symbol
is unset.

## Configuration facts relied on

From Fubuki's config: CONFIG_SLUB=y; CONFIG_SLUB_TINY is not
set; CONFIG_MMU=y; CONFIG_NUMA=y; CONFIG_SMP=y;
CONFIG_GENERIC_ALLOCATOR=y; CONFIG_HAS_DMA=y; CONFIG_CMA is not
set. In the 6.12.116 mm/Makefile, slub.o, slab_common.o,
mempool.o, percpu.o, util.o, page_alloc.o and memblock.o are
obj-y (unconditional); vmalloc.o is built under CONFIG_MMU;
cma.o under CONFIG_CMA; lib/genalloc.o under
CONFIG_GENERIC_ALLOCATOR; kernel/dma/direct.o under
CONFIG_HAS_DMA; drivers/base/devres.o is in the base core list.

## The enumeration

| Point | Function | Gate | Verdict |
|---|---|---|---|
| 1.1 | `slab_alloc_node` (mm/slub.c) | slub.o, CONFIG_SLUB=y | compiled in |
| 1.2 | `__do_kmalloc_node` (mm/slub.c) | slub.o | compiled in |
| 1.3 | `__kmalloc_large_noprof` (mm/slub.c) | slub.o | compiled in |
| 1.4 | `kfree`, `kmem_cache_free` (mm/slub.c) | slub.o | compiled in |
| 1.5 | `__kmem_cache_create_args` (mm/slab_common.c) | slab_common.o, obj-y | compiled in |
| 1.6 | `krealloc_noprof` (mm/slab_common.c) | slab_common.o, obj-y | compiled in |
| watch | `alloc_slab_page` (mm/slub.c) | slub.o | compiled in |
| 2.1 | `__vmalloc_node_range_noprof` (mm/vmalloc.c) | vmalloc.o, CONFIG_MMU=y | compiled in |
| 2.2 | `__get_vm_area_node`, `alloc_vmap_area` (mm/vmalloc.c) | vmalloc.o | compiled in (no patch, by design) |
| 2.3 | `vm_area_alloc_pages` (mm/vmalloc.c) | vmalloc.o | compiled in |
| 2.4 | `vfree` (mm/vmalloc.c) | vmalloc.o | compiled in |
| 3.1 | `__alloc_pages_noprof` (mm/page_alloc.c) | page_alloc.o, obj-y | compiled in |
| 3.2 | `__free_pages`, `free_pages` (mm/page_alloc.c) | page_alloc.o, obj-y | compiled in |
| 4.1 | `mempool_alloc_noprof` (mm/mempool.c) | mempool.o, obj-y | compiled in |
| 4.2 | `pcpu_alloc_noprof` (mm/percpu.c) | percpu.o, obj-y | compiled in |
| 4.3 | `gen_pool_alloc_algo_owner` (lib/genalloc.c) | genalloc.o, CONFIG_GENERIC_ALLOCATOR=y | compiled in |
| 4.4 | `cma_alloc` (mm/cma.c) | cma.o, CONFIG_CMA unset | **absent** |
| cross | `__kvmalloc_node_noprof` (mm/util.c) | util.o, obj-y | compiled in |
| cross | `alloc_dr` (drivers/base/devres.c) | devres.o, base core | compiled in |
| cross | `__dma_direct_alloc_pages` (kernel/dma/direct.c) | direct.o, CONFIG_HAS_DMA=y | compiled in |
| scope | `memblock_alloc_internal` (mm/memblock.c) | memblock.o, obj-y | compiled in (pre-slab; out of runtime scope) |

## Findings

- Every first-ring center (Families 1 and 2) is compiled into
  Fubuki's kernel. Nothing in the patch set is modular: no gate
  in the set is a tristate set to m, so the compiled subset
  equals the patch list minus one point.
- The single absence is 4.4, `cma_alloc`: CONFIG_CMA is not set
  in the stock trixie amd64 configuration, so Fubuki's kernel
  contains no CMA allocator. The DMA-coherent path in this
  build therefore runs through the direct allocator (compiled
  in), and the design's CMA question is moot for Fubuki.
- CONFIG_NUMA=y means the node variants the patch list names
  (the `_node` doors into 1.1 and 1.2) exist in this build.
- The dye test's conditions transfer to Fubuki's build class:
  the dye was measured with CONFIG_SLUB=y and SLUB_TINY unset
  on 6.12.116 defconfig, and Fubuki's configuration has
  CONFIG_SLUB=y with SLUB_TINY unset, so `__fastpath_inline`
  resolves the same way in her kernel.

Correction 2026-11-12 (at Andrew's stet): this enumeration was
read against the stock trixie config of linux-image
6.12.113+deb13-amd64, fetched 2026-11-10 and taken at the time
as Fubuki's configuration. The byte-check run on 2026-11-12
established that Fubuki's kernel is 6.12.107+deb13-amd64
(vmlinuz version string, Debian 6.12.107-1) and that the
build's own exported config, `telezart-kconfig` in both ELBE
build directories on telesart, is the 6.12.107 config (11,466
lines, SHA-256
9b460f1003db4d417288e9c14caecfa14245dab07186df948fef0f3661e38ceb).
The version difference stated in this artifact therefore
reads 6.12.116 against 6.12.107. The load-bearing options are
verified on the build's own config: CONFIG_SLUB=y,
CONFIG_CMA not set. The enumeration's per-point verdicts stand
as read, pending the full gating-symbol re-reading against
the 6.12.107 config, which is the open follow-up; the method
section above is unchanged as the record of how the
enumeration was actually performed.
