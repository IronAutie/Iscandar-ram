# Patch manifest

Every center the reconciliation names, and the form its
coverage takes in this staged set. The link-check verifies
each name's presence here and, for the hunk rows, in a patch
hunk.

| Center | Function (6.12.116) | File | Form |
|---|---|---|---|
| 1.1 | `slab_alloc_node` (4205) | 0001 | hunk: redirect to the bridge |
| 1.2 | `__do_kmalloc_node` (4347) | 0001 | covered by 1.1: it calls `slab_alloc_node`; the size and flags translation is checked at 1.1's hunk |
| 1.3 | `__kmalloc_large_noprof` (4326) | 0001 | hunk: its own center, no exemption (ruling 2) |
| 1.4 | `kfree` (4817), `kmem_cache_free` (4785), `kmem_cache_free_bulk` (4945) | 0001 | hunk: range decision, then bridge free |
| 1.5 | `__kmem_cache_create_args` (297), `kmem_cache_destroy` (506) | 0002 | hunk: register and deregister under the cap |
| 1.6 | `krealloc_noprof` (1308) | 0002 | by composition: if 1.1 and 1.4 are patched, krealloc follows; listed so its coverage is verified, not assumed |
| watch | `alloc_slab_page` (2470) | 0001 | instrumentation only: the bypass counter (Andrew's ruling, 2026-11-11) |
| 2.1 | `__vmalloc_node_range_noprof` (3826) | 0003 | the center; its halves are 2.2 (stock) and 2.3 (hunk) |
| 2.3 | `vm_area_alloc_pages` (3594) | 0003 | hunk: backing replaced with one exact-fit bridge run |
| 2.4 | `vfree` (3407), `vfree_atomic` (3373) | 0003 | hunk: range decision, then bridge free |
| 4.1 | `mempool_alloc_noprof` (404), `mempool_free` (518) | 0004 | by backing: over slab, redirected at 1.1 with its elements; over pages, substrate (reconciliation, Family 4a) |
| 4.2 | `pcpu_alloc_noprof` (1743) | 0004 | hunk: redirect at the center; the chunk layer's internals are build-time design work the reconciliation names, carried in BUILD-STATUS.md |
| 4.3 | `gen_pool_alloc_algo_owner` (277), `gen_pool_free_owner` (487) | 0004 | hunk, gated on pool registration; the served set is open in the reconciliation and the gate is where the build names it |
| cross | `__kvmalloc_node_noprof` (650) | (none) | covered if both its centers are patched, as they are; the fallback path's free semantics are a build verification item, carried in BUILD-STATUS.md |
| cross | `alloc_dr` (110) | (none) | lands on 1.2 unchanged; no separate center exists or is needed |

Not patched, each named: see NOT-PATCHED.md.
