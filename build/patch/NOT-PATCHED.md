# Not patched, named

The families and doors the live build does not serve, each
with its ground, as the reconciliation's criterion requires:
any family the build will not serve is named, and the claim
shrinks to fit the naming. The link-check verifies this list
is present and complete against the reconciliation.

- Page-direct, centers 3.1 and 3.2: `__alloc_pages_noprof`
  (mm/page_alloc.c, 4810), `__free_pages` (4926),
  `free_pages` (4942). Substrate by ruling 1: the page
  allocator stays underneath as the bulk supplier of the
  arena and parcels nothing for callers; by ruling 5 its own
  doors stay substrate as well. The build's principal
  declared exception.
- DMA-coherent allocations: `__dma_direct_alloc_pages`
  (kernel/dma/direct.c, 118). Served through the substrate
  path via the direct allocator, following from rulings 1
  and 5 with the measured CMA absence.
- vmalloc reservation half, center 2.2: `__get_vm_area_node`
  (mm/vmalloc.c, 3168) and `alloc_vmap_area` (2008). Stock
  by design: address-space bookkeeping was never part of
  the replacement. Only the backing (2.3) is Iscandar's.
- memblock: `memblock_alloc_internal` (mm/memblock.c,
  1578). Out of scope, pre-slab: the boot allocator is dead
  before the runtime test begins.
- vmap and ioremap: `vmap` (mm/vmalloc.c, 3496),
  `vm_map_ram` (3038). Out of scope: address space only,
  no memory allocated.
- CMA, center 4.4: `cma_alloc` (mm/cma.c, 515). Absent
  from Fubuki's build: CONFIG_CMA is not set, measured by
  the compiled subset. Nothing exists to serve.
- The early-boot prefix, across all families: stock,
  logged, under ruling 4. Not an exception in the claim's
  sense: a named, counted, bounded prefix, closed by the
  layer's raise.
