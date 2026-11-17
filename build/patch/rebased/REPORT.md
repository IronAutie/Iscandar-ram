# Paper 3 patch rebase report, 2026-11-17

Rebase of the staged patch set (`~/workspace/Iscandar-ram/build/patch/`)
against the real Linux 6.12.116 tree (kernel.org tarball, SHA-256
`cb3ed3dfdd47e6ade49dc933db334fd86e24cadee920da612c262b0e9b6a511b`).
The staged insertions are the design's content, carried over verbatim
except for the logged adaptations in section 2, each forced by the real
tree. Nothing in `~/workspace/Iscandar-ram` or any git repo was modified;
all work is in `~/workspace/scratch-rebase/`.

Deliverables in this directory: `0001-slab-centers.rebased.patch`,
`0002-cache-setup.rebased.patch`, `0003-vmalloc-backing.rebased.patch`,
`0004-second-ring.rebased.patch`, `0005b-makefile-fix.patch`.

## 1. Per-patch anchor summary

Line numbers are the pristine 6.12.116 tree's. "Top of function" means
immediately after the function's opening brace, as the staged hunks
intend; the one exception is 0002's create hunk, whose staged form is
explicitly placed at the successful return (its own inserted comment
says so), and the rebase follows the staged placement.

### 0001, mm/slub.c

- `alloc_slab_page` (def. line 2470; watch point): the staged comment
  plus `atomic64_inc(&iscandar_slab_page_calls);`. Nothing else in the
  function is touched.
- `slab_alloc_node` (def. line 4205, center 1.1): the bridge redirect.
  If `iscandar_active()`: build a record (OP_CACHE_ALLOC, cache_id from
  `iscandar_cache_lookup(s)`, size `s->object_size`, align `s->align`);
  an unregistered cache (id 0) returns NULL as a finding; a failed call
  or non-OK status returns NULL; on success run `s->ctor` if present,
  zero under `__GFP_ZERO`, return `iscandar_offset_to_ptr(rec.result)`.
  Otherwise fall through after `iscandar_prefix_note(CENTER_SLAB_ALLOC,
  s->object_size)`. The real parameter names are `s` and `gfpflags`,
  exactly as staged.
- `__kmalloc_large_noprof` (def. line 4326, center 1.3): OP_ALLOC_LARGE
  with `rec.size = size`; failure returns NULL; success returns the
  offset as a pointer. Prefix note CENTER_KMALLOC_LARGE otherwise.
- `kmem_cache_free` (def. line 4785, center 1.4): if the pointer is in
  the arena, OP_FREE with the pointer's offset and the cache's id, then
  return; else the stock body. (Name adaptation, section 2.)
- `kfree` (def. line 4817, center 1.4): same door without a cache id.
  (Name adaptation, section 2.)
- `kmem_cache_free_bulk` (def. line 4945, center 1.4): the staged
  compaction loop verbatim: each arena pointer is freed through the
  bridge one by one and swapped to the tail; `size` is reduced to the
  stock remainder; an all-bridge bulk returns early. Real names
  (`s`, `size`, `p`) match the staged code unchanged.

### 0002, mm/slab_common.c

- `__kmem_cache_create_args` (def. line 297, center 1.5): inserted
  immediately before the function's final `return s;` (pristine
  line 379), i.e. on the success path only, after the `if (err)` tail.
  If active, `iscandar_cache_register(s, object_size, args->align,
  flags)`; a refusal fails the creation (see the failure-shape
  adaptation, section 2). Otherwise `iscandar_prefix_note(
  CENTER_CACHE_SETUP, object_size)`. (Name adaptations, section 2.)
- `kmem_cache_destroy` (def. line 506, center 1.5): top of function:
  if active, `iscandar_cache_unregister(s)`; the stock teardown then
  proceeds, as staged.
- Center 1.6 (`krealloc_noprof`) takes no hunk, by composition, as
  staged and as the MANIFEST records.

### 0003, mm/vmalloc.c

- `vm_area_alloc_pages` (def. lines 3593-3596, center 2.3): if active,
  one OP_VMALLOC_BACKING record for the whole area
  (`rec.size = nr_pages << PAGE_SHIFT`); on success fill
  `pages[i] = iscandar_offset_to_page(rec.result) + i` for all
  `nr_pages` and return `nr_pages`; on failure return 0. Otherwise the
  prefix note and the stock body. (Name and return-shape adaptations,
  section 2; the real function has no `area` parameter and returns a
  page count, not an error code.)
- `vfree` (def. line 3407, center 2.4): top of function: if the address
  is in the arena, OP_VFREE with its offset, then return; else the
  stock body. Real name `addr` matches the staged code.
- `vfree_atomic` (def. line 3373, center 2.4): the staged hunk inserts
  its design comment only, no code. The rebase carries exactly that.
  See section 5 for what the tree says about this door.

### 0004, mm/percpu.c and lib/genalloc.c

- `pcpu_alloc_noprof` (def. line 1743, center 4.2): the staged redirect
  verbatim: if active and not `reserved`, OP_PERCPU_ALLOC with
  `rec.size = size * num_possible_cpus()` and `rec.align = align`;
  failure returns NULL; success returns the arena pointer cast to
  `void __percpu *`. Otherwise the prefix note and the stock body.
  No name adaptations: the real parameters are `size`, `align`,
  `reserved` (plus `gfp`, unused by the insertion), and
  `num_possible_cpus()` is already used in this file (lines 2468,
  2903, 3234). See section 4 for the structural findings.
- `gen_pool_alloc_algo_owner` (def. line 277, center 4.3): verbatim.
  If active and `iscandar_gen_pool_is_arena(pool)`: OP_GENPOOL_ALLOC,
  failure returns 0, success returns the arena pointer as the address.
  If active but the pool is not an arena pool, the prefix note stands
  and the stock body serves it, per the staged gate.
- `gen_pool_free_owner` (def. line 487, center 4.3): verbatim. For an
  arena pool and an in-arena address, OP_GENPOOL_FREE with the offset,
  then return; else the stock body.

### 0005b, mm/Makefile

Adds `obj-y += iscandar_bridge.o` immediately after the real line 67
(`obj-y += page-alloc.o`), with true context. This replaces 0005's
staged Makefile hunk, which expected line 4 to be blank (the real file
has `#` at line 4, blank at line 5) and whose top-of-file placement
would in any case be discarded by the `obj-y :=` assignment at
line 51. On the bench this fix was already applied by hand via sed;
this file is the repo record of it. The rest of 0005 (the two new
files) applied cleanly at the bench and is not redone here.

## 2. Adaptations to the real tree (complete list)

Everything not listed here is byte-for-byte the staged code.

1. **0001, `kmem_cache_free`: staged `objp` -> real `x`** (2 uses).
   The real signature is `void kmem_cache_free(struct kmem_cache *s,
   void *x)` (line 4785). The staged code named the object pointer
   `objp`; the tree names it `x`.
2. **0001, `kfree`: staged `objp` -> real `object`** (2 uses).
   The real signature is `void kfree(const void *object)` (line 4817).
3. **0002, `__kmem_cache_create_args`: staged `args->object_size` ->
   the function's `object_size` parameter** (2 uses: the register
   call and the prefix note). `struct kmem_cache_args` in 6.12.116
   (include/linux/slab.h, line 265) has fields `align`, `useroffset`,
   `usersize`, `freeptr_offset`, `use_freeptr_offset`, `ctor` only;
   object size is a separate parameter of the function.
4. **0002, same site: staged `args->flags` -> the function's `flags`
   parameter.** For the same reason: there is no `flags` field in
   `struct kmem_cache_args`; flags arrive as the `slab_flags_t flags`
   parameter. Note the value at the insertion point is the sanitized
   flags (the body masks it with `CACHE_CREATE_MASK` before the
   success return), which is the geometry the cache was actually
   created with. `args->align` is kept as staged: the field exists,
   and on the create path the body has already replaced it with the
   calculated alignment before the success return.
5. **0002, same site, failure shape: staged `return
   ERR_PTR(-ENOSPC);` -> `return NULL;`.** This is the one deliberate
   deviation from the staged text beyond names, and it is flagged for
   Andrew's ruling. The real function's contract is NULL on failure:
   its kernel-doc says "a pointer to the cache on success, NULL on
   failure", and its own error tail returns NULL. Callers of
   `__kmem_cache_create_args` (the `kmem_cache_create()` wrappers)
   treat the result as a cache pointer or NULL; an ERR_PTR would be
   handed on as if it were a cache. The design's intent is preserved:
   a refused registration still fails the creation, loudly, and
   nothing falls back to stock. Only the shape is the function's own.
   If Andrew wants the ERR_PTR kept as a marker, it is a one-line
   change in the rebased patch.
6. **0003, `vm_area_alloc_pages`: staged `area->nr_pages` ->
   `nr_pages`, and `area->pages[i]` -> `pages[i]`** (the size
   expression, the loop bound, the prefix-note argument, and the fill
   target). The real function is `static inline unsigned int
   vm_area_alloc_pages(gfp_t gfp, int nid, unsigned int order,
   unsigned int nr_pages, struct page **pages)` (lines 3593-3595);
   there is no `struct vm_struct *` in scope at this center. The
   caller (`__vmalloc_area_node`, line 3729) passes `nr_small_pages`,
   the total PAGE_SIZE page count, so `nr_pages` here is the same
   quantity the staged code meant by `area->nr_pages`.
7. **0003, same site, return shapes: staged `return -ENOMEM;` on
   bridge failure -> `return 0;`; staged `return 0;` on success ->
   `return nr_pages;`.** The real function returns an unsigned page
   count: the number of entries filled in `pages[]`. The caller
   assigns it to `area->nr_pages` and treats any count short of the
   request as failure (vmalloc.c lines 3729-3758). Returning -ENOMEM
   as an unsigned count would read as roughly four billion pages
   allocated; returning 0 on success would read as total failure.
   The adapted shapes are the function's own: 0 is its "nothing
   allocated" outcome, `nr_pages` its complete one. The design is
   unchanged: one bridge run backs the whole area, or the area fails.

No other staged name or shape needed adaptation. In particular 0004
needed none at all.

Two placement observations, recorded, not acted on:

- In 0002, the alias path also reaches the insertion:
  `__kmem_cache_alias` returns an existing cache through the same
  `return s;`, so an aliased creation would register the same cache
  pointer a second time. `iscandar_cache_lookup` returns the first
  match, so the second entry is shadowed, but the layer would hold a
  duplicate registration. The staged placement ("at the successful
  return") produces this; the rebase keeps it and records it.
- In 0003, when the caller requests `order > 0` (huge vmalloc), the
  stock body fills `pages[]` with the sub-pages of split high-order
  allocations, while the bridge fill supplies one order-0 arena page
  per slot, in order. The slot count is the same (`nr_pages` counts
  PAGE_SIZE pages in both readings). Whether the mapping step
  (`vmap_pages_range` at the area's page shift) accepts order-0 pages
  in every slot is a bench question; the staged design's fill is
  carried unchanged.

## 3. Identifier resolution

Every `iscandar_*` / `ISCANDAR_*` identifier in the inserted code of
0001-0004 was checked against the union of: the shared header
`build/bridge/iscandar_bridge.h` (all `ISCANDAR_*` constants and
`IscandarBridgeRecord`), the glue header created by 0005
(`include/linux/iscandar.h`: the function declarations and the
`extern atomic64_t iscandar_slab_page_calls;`), and the definitions in
the bridge core created by 0005 (`mm/iscandar_bridge.c`).

Result: **no unresolved identifiers.** The 29 distinct tokens used
(15 constants, the record type, 11 functions, and the counter
variable; one further token, `iscandar_bridge`, appears only inside a
comment's `/proc/iscandar_bridge` path) all resolve. The record fields
the insertions touch (`op`, `status`, `size`, `align`, `cache_id`,
`result`) are all fields of `IscandarBridgeRecord` in the shared
header. Every function the insertions call is both declared in the
glue header and defined in the staged bridge core.

One gap the staged set itself leaves, flagged as an open item rather
than filled: **no staged hunk adds `#include <linux/iscandar.h>` to
any of the four target files**, and the rebased patches, following the
staged set, do not either. As they stand, mm/slub.c, mm/slab_common.c,
mm/vmalloc.c, mm/percpu.c and lib/genalloc.c do not see the bridge
declarations. At application time each of those files needs
`#include <linux/iscandar.h>` (which in turn includes
`<iscandar_bridge.h>`). This is a compile-time necessity the staging
never wrote down; it is recorded here so the bench adds it knowingly,
as part of the rebase step BUILD-STATUS.md already carries, instead
of discovering it as a wall of implicit-declaration errors.

## 4. The percpu chunk question (0004, center 4.2)

Asked: does the real `pcpu_alloc_noprof` match the staged anchor's
assumptions, and does the redirect as designed fit?

The anchor fits exactly. The real function (mm/percpu.c line 1743) is
`void __percpu *pcpu_alloc_noprof(size_t size, size_t align, bool
reserved, gfp_t gfp)`; size, alignment and the reserved flag are all
in hand at the top of the function, and the rebase needed no
adaptation. That is where the fit ends. Three findings from the tree:

1. **The return value's address-space convention does not match.**
   The stock body ends (line 1901) with
   `ptr = __addr_to_pcpu_ptr(chunk->base_addr + off);`, where `off` is
   an offset the chunk layer carved from a chunk anchored in the
   percpu address space at `pcpu_base_addr`. Every consumer reaches
   CPU *c*'s copy through `per_cpu_ptr(ptr, cpu)`, which adds
   `__per_cpu_offset[cpu]` (mm/percpu.c line 3325) to the pointer:
   the returned value is meaningful only as a coordinate in that
   replicated layout, where each CPU's instance sits a per-CPU stride
   from the base. The staged insertion returns a single arena-mapping
   pointer cast to `void __percpu *`. Adding `__per_cpu_offset[cpu]`
   to it does not reach a per-CPU replica of anything; there is one
   run, not one instance per CPU at the percpu strides. The staged
   size expression (`size * num_possible_cpus()`) buys the bytes for
   all CPUs in one run, but the tree's addressing never walks a
   single run that way.
2. **There is no free door.** Staged 0004 patches only the allocation
   center. `free_percpu` (mm/percpu.c line 2234) resolves its pointer
   with `pcpu_chunk_addr_search(addr)` and frees by
   `pcpu_free_area(chunk, off)`; a bridge-born pointer belongs to no
   chunk, so the search has no chunk to find. The shared header
   defines `ISCANDAR_OP_PERCPU_FREE` (op 12), but no staged hunk
   uses it. Any percpu area granted through the inserted door cannot
   be freed through the stock door.
3. **The chunk layer is not beneath this center; it is the center's
   body, and the insertion bypasses it entirely.** The staged
   preamble says the chunk bookkeeping stays the kernel's over the
   bridge run. The inserted code returns before any of it runs: no
   `pcpu_alloc_area`, no chunk bitmap marks, no populate step, no
   per-CPU zeroing (the stock `area_found` path memsets each CPU's
   copy). Nothing records the run in any chunk, so the chunk layer
   can neither manage nor reclaim it. The insertion also sits above
   the stock body's own front matter: `size = ALIGN(size,
   PCPU_MIN_ALLOC_SIZE)` and the illegal-size/alignment rejection
   (size over `PCPU_MIN_UNIT_SIZE`, align over `PAGE_SIZE`) happen
   after the door, so the bridge is asked for the caller's raw,
   unrounded size, and requests the stock body would refuse outright
   are bridge-served instead.

Verdict: the rebase is faithful and the anchor is real, but the
redirect as inserted does not by itself make a working percpu
allocation against this tree. It stands exactly where the
reconciliation and BUILD-STATUS.md left it: a separate design
question, now with its three concrete obstacles named from the
source. The rebased patch carries the staged hunk unchanged, so the
bench build will compile the door in and the evidence run can meet
the question empirically; nothing here pre-decides it.

## 5. The vfree_atomic question (0003, center 2.4)

Asked: does 6.12.116 have a `vfree_atomic` path separate from `vfree`
that would bypass the patched `vfree` door?

It exists, it is a separate entry point, and it does not bypass the
door; it postpones it. The tree, mm/vmalloc.c:

- `vfree_atomic` (line 3373) frees nothing itself. It adds the
  address to a per-CPU lockless list (`vfree_deferred`, line 91) and
  schedules work: `if (addr && llist_add((struct llist_node *)addr,
  &p->list)) schedule_work(&p->wq);`.
- The worker, `delayed_vfree_work` (line 3357), drains the list by
  calling `vfree(llnode)` on every queued address. A free that enters
  through `vfree_atomic` therefore arrives at the patched `vfree`
  door shortly after, in process context.
- The traffic also runs the other way: `vfree` itself, when called
  in interrupt context, delegates to `vfree_atomic` and returns
  (lines 3412-3414), and those frees likewise come back through the
  worker into `vfree`.

So no second door is needed for routing: every vmalloc free in this
tree eventually passes through `vfree` exactly once, which is where
the staged design puts its door, and the staged comment-only hunk at
`vfree_atomic` is consistent with that. Two findings ride along,
both for the bench, neither acted on in the rebase:

1. **The inserted vfree door sits above vfree's own atomic
   delegation.** The insertion is at the very top of `vfree`, before
   the `in_interrupt()` check that hands off to `vfree_atomic`. An
   interrupt-context `vfree()` of an in-arena pointer would therefore
   take the bridge call directly instead of deferring. In the staged
   bridge core, `iscandar_bridge_call` takes a mutex and polls with
   `mdelay(1)` up to a 5-second timeout: it sleeps in exactly the
   context the deferral exists for. The staged comment at
   `vfree_atomic` says the atomic question "is not pre-decided" and
   lands at the bench if the call turns out blocking; on the staged
   core's own text, the call does block. Flagged as the finding the
   comment anticipates, one step earlier than expected.
2. **The vfree door's range test and the vmalloc address range.**
   `iscandar_ptr_in_arena` tests against the arena's kernel mapping
   (`iscandar_arena_base` .. + `ISCANDAR_ARENA_BYTES`, bridge core).
   That test is coherent for Family 1 pointers, which are handed out
   as arena-mapping addresses by `iscandar_offset_to_ptr` itself.
   But an address handed to `vfree` is a vmalloc-area virtual
   address, assigned by the stock reservation step (2.2, unpatched)
   from the vmalloc range, with the arena's pages mapped into it by
   `vmap_pages_range`. It is not an address in the arena mapping's
   range, so as inserted the door's range test would not recognize a
   genuine bridge-backed vmalloc area, and the stock body would go on
   to `__free_page` the arena's pages. The staged code has the same
   test; the rebase preserves it and flags the geometry as a design
   question Andrew should rule on before the bench boot, since it
   decides whether center 2.4 fires at all. (The gen_pool door's
   identical test is coherent under 0004's own stipulation that a
   registered pool's managed range is the arena mapping.)

## 6. The alloc_slab_page watch counter

Status: complete as staged; nothing missing.

- The counter itself lives in the bridge core 0005 installs:
  `atomic64_t iscandar_slab_page_calls = ATOMIC64_INIT(0);`
  (mm/iscandar_bridge.c, line 28 of the created file), declared
  `extern atomic64_t iscandar_slab_page_calls;` in the glue header
  with the comment "The bypass watch counter (0001 instruments the
  site)", and the core's /proc read reports it among the bridge
  counters (per the core's closing comment: "reading reports the
  counters, the watch counter among them").
- The increment call site is provided by staged 0001's final hunk,
  and the rebased 0001 carries it at the top of `alloc_slab_page`
  (mm/slub.c line 2470): `atomic64_inc(&iscandar_slab_page_calls);`
  under the staged comment. No call had to be invented.
- One wording note: the patch README says "`alloc_slab_page` is not
  patched; it gains a counter", while staged 0001 does carry a hunk
  at the function. The two agree in substance (the function gains the
  counter increment and nothing else; it is instrumented, not
  redirected), but a reader of the README alone would not expect the
  hunk to exist. Recorded here; no file changed.

## 7. Header-path discrepancy in the patch README

`build/patch/README.md` states the application rule as: "application
copies `build/bridge/iscandar_bridge.h` into the tree as
include/iscandar_bridge.h". 0005's own preamble says the same
("copied verbatim ... to include/iscandar_bridge.h in the tree").
That is not what the rest of the record does:

- 0005's diff creates `include/linux/iscandar.h`, a glue header whose
  body does `#include <iscandar_bridge.h>`; the shared header is the
  file that glue expects to find as `<iscandar_bridge.h>`, i.e. at
  `include/iscandar_bridge.h` would not satisfy it either (the
  include is `<iscandar_bridge.h>`, resolved on the kernel's include
  path, conventionally `include/linux/iscandar_bridge.h` or a
  `-I`-visible copy).
- BUILD-STATUS.md's telesart run list (step 3) instead instructs:
  copy `build/bridge/iscandar_bridge.h` into the tree as
  `include/linux/iscandar.h` verbatim.
- The bench followed the run list: the shared header was installed
  as `include/linux/iscandar.h`.

Per instructions this report follows the 0005/bench reality
(`include/linux/iscandar.h`) and does not edit any of the three
documents. But the three cannot all hold at once: if the shared
header occupies `include/linux/iscandar.h`, the glue header 0005
creates at the same path is displaced, and the glue's declarations
(the function prototypes the centers use) then rest on the shared
header alone, which does not carry them (it defines constants, the
record, and the control block, not the `iscandar_*` prototypes).
Which of the two files is to live at `include/linux/iscandar.h`,
and where the other lives, is a naming decision for Andrew; the
identifier check in section 3 was run against the union of both
headers plus the core, so its result holds under either resolution.

## 8. Verification

Method: the pristine extracted files were copied to a fresh test
tree (`~/workspace/scratch-rebase/test/`, `mm/` and `lib/` at the
top so `-p1` strips correctly). Each rebased patch was dry-run in
the order 0001, 0002, 0003, 0004, 0005b, then all five were applied
for real in the same order, and every resulting file was byte-compared
against the independently edited copies the diffs were generated
from.

Results:

- `patch -p1 --dry-run`: all five patches exit 0. No FAILED hunks,
  no fuzz, and no offsets reported: every hunk landed at its stated
  line numbers.
- Real application: all six files patched cleanly
  (mm/slub.c, mm/slab_common.c, mm/vmalloc.c, mm/percpu.c,
  lib/genalloc.c, mm/Makefile); no `.rej` or `.orig` files produced.
- Byte comparison: all six resulting files are identical to the
  edited copies of record.
- The rebased 0001-0004 do not textually depend on 0005's created
  files (their only link to 0005 is the header include path, an
  application-time matter; see the open item in section 3), so the
  dry-runs above were run against the pristine files alone, as the
  bench will meet them after 0005.

Not verified, by design: compilation. The patches are rebased and
application-verified; no kernel was configured or built here, and
nothing in this report claims a compile. The first compile happens
at the bench, with the Fubuki config of record, after the include
item in section 3 is settled.

## 9. Rulings, 2026-11-17 (Andrew)

- Header arrangement: the shared header is copied into the tree
  as `include/iscandar_bridge.h`; the glue header 0005 creates
  stays at `include/linux/iscandar.h`. The run-list wording in
  BENCH-RUN-SHEET.md and BUILD-STATUS.md is corrected to match.
  The bench confirmed the glue occupies its seat and copied the
  shared header to its ruled home.
- Return shapes (section 2, items 5 and 7): both adaptations are
  ACCEPTED, with the requirement that the code carry comments
  recording the staged text and the reason. The comments are in
  the rebased 0002 (at the `__kmem_cache_create_args` register
  site) and the rebased 0003 (inside the `vm_area_alloc_pages`
  bridge block). Both patches were regenerated after the
  comments were added and the full set re-verified: real
  application to pristine files, all six results byte-identical
  to the edited copies of record, no `.rej` or `.orig` files.
- vfree door (section 5, rider ii): FIXED, as ruled. The door in
  the rebased 0003 no longer tests the address against the
  arena's kernel mapping; it finds the vm area and tests the
  area's first page against the arena's physical range, taking
  the record's offset from that page. It also sits below
  `vfree`'s `in_interrupt()` delegation (rider i), so a deferred
  atomic free meets it in process context. Two helpers were
  added to the bridge core for this, `iscandar_page_in_arena`
  and `iscandar_page_to_offset`, beside their pointer siblings,
  with declarations in the glue header; 0005's glue and core
  hunks carry them (counts 42 and 201). A comment at the door
  records the staged text and the geometry. The full set,
  edited 0005 included, was re-verified by real application:
  all six target files byte-identical to the copies of record,
  the door below the delegation and above the stock teardown,
  no `.rej` or `.orig` files. Bench note: telesart already
  holds the glue and core from the original 0005; both files
  must be refreshed from the updated 0005 when the set is
  pushed.
- Percpu (section 6): RULED 2026-11-17. Center 4.2 is served
  under a stated uniprocessor precondition: Iscandar is an
  embedded, single-CPU allocator and claims no SMP service
  ("We cant be all things to all people... a fit for space
  embedded memory allocator can also serve as a multiple cpu
  consumer product" is the claim Andrew declines, in his
  words; he wants the scope stated in the paper). On a UP
  build the percpu translations are the identity, so the
  staged return stands and the staged size line's
  `num_possible_cpus()` factor is 1. The rebased 0004's
  percpu door now also: mirrors the stock rejection set
  (zero size, size above PCPU_MIN_UNIT_SIZE, align above
  PAGE_SIZE or not a power of two), refuses atomic-context
  callers (the bridge call sleeps; NULL is the handled
  shape), and zeroes the run before returning it. A free
  door was added in `free_percpu`, above the chunk search:
  the header names ISCANDAR_OP_PERCPU_FREE the 4.2 free
  door and the layer implements it, but the staged set
  carried no hunk sending it. Andrew's parity idea (realspace
  pointers are always even; percpu pointers could be minted
  odd and intercepted by parity, which survives the even
  per-CPU strides) is recorded as the identification
  mechanism in the drawer; under identity translation the
  plain range test suffices and parity is not used. The
  bench build is uniprocessor by one ruled derivative on
  the Fubuki config of record (SMP disabled; run sheet
  Task 5). Full set re-verified after these changes: real
  application, all six files byte-identical to the copies
  of record, no `.rej` or `.orig` files.
