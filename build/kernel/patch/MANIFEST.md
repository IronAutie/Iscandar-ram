# Kernel-round patch manifest

Every center the stetted coverage audit names, and the
form its coverage takes in this set. Anchors are
against pristine Linux 6.12.116, checked on this
machine at staging time (2026-11-20) against the
tarball of record (SHA-256 cb3ed3df...511b); the
bridge round's manifest numbers agree throughout,
which cross-checks both.

| Center | Function (6.12.116) | File | Form |
|---|---|---|---|
| core | (new) mm/iscandar/, include/linux/iscandar.h | 0001 | the glue: irqsave alloc/free over the seated engine, ledger, registry, prefix, latency ledger, canary init, state read |
| mass | (new) .iscandar_state, .iscandar_arena sections | 0001 | iscandar_mass.s copied verbatim; linker placement hunk in arch/x86/kernel/vmlinux.lds.S |
| 1.1 | `slab_alloc_node` (4205) | 0002 | hunk: door, direct call |
| 1.2 | `__do_kmalloc_node` (4347) | 0002 | covered by 1.1: it calls `slab_alloc_node` |
| 1.3 | `__kmalloc_large_noprof` (4326) | 0002 | hunk: its own center, no exemption |
| 1.4 | `kfree` (4817), `kmem_cache_free` (4785), `kmem_cache_free_bulk` (4945) | 0002 | hunk: range decision, then direct free |
| 1.5 | `__kmem_cache_create_args` (297), `kmem_cache_destroy` (506) | 0003 | hunk: register and deregister in the mass registry, control path |
| 1.6 | `krealloc_noprof` (1308) | 0003 | by composition: if 1.1 and 1.4 are patched, krealloc follows |
| watch | `alloc_slab_page` (2470) | 0002 | instrumentation only: the bypass counter (Andrew's ruling, 2026-11-11) |
| 1.x bulk | `__kmem_cache_alloc_bulk` (4965, the full-SLUB body) | 0005 | hunk: the funnel, per the audit's item 2 as ruled 2026-11-19; CONFIG_SLUB=y, SLUB_TINY not set, both read at the bench 2026-11-20 |
| 2.1 | `__vmalloc_node_range_noprof` (3826) | 0004 | the center; its halves are 2.2 (stock) and 2.3 (hunk) |
| 2.3 | `vm_area_alloc_pages` (3594) | 0004 | hunk: backing replaced with one exact-fit run |
| 2.4 | `vfree` (3407), `vfree_atomic` (3373) | 0004 | hunk: range decision, then direct free; the atomic door is taken in this build |
| 4.1 | `mempool_alloc_noprof` (404), `mempool_free` (518) | (none) | by backing: over slab, redirected at 1.1 with its elements; over pages, substrate |
| 4.2 | `pcpu_alloc_noprof` (1743), `free_percpu` (2234) | 0005 | hunks: doors at the center and above the chunk search, verified in the bridge tree at the bench 2026-11-20; the atomic refusal is removed in kernel form |
| 4.3 | `gen_pool_alloc_algo_owner` (277), `gen_pool_free_owner` (487); gate at `gen_pool_add_owner` (184) and `gen_pool_add_virt` | 0005 | hunks: doors gated on the recorded backing-rule decision; the gate's test runs at chunk-add |
| cross | `__kvmalloc_node_noprof` (650) | (none) | covered if both its centers are door'd, as they are; the composition verified at the bench 2026-11-20 (kvfree at mm/util.c:696 partitions on is_vmalloc_addr); K1 to K3 ride to verification step 5 |
| cross | `alloc_dr` (110) | (none) | lands on 1.2 unchanged; no separate center exists or is needed |

Not patched, each named: the audit's deliberately
stock list (../COVERAGE-AUDIT.md), which this set
adopts unchanged: page-direct (3.1, 3.2), DMA-coherent
through the substrate, the vmalloc reservation half
(2.2), memblock, vmap and vm_map_ram, cma_alloc
(absent from the build), and the early-boot prefix
across all families.
