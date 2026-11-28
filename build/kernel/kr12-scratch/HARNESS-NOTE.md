# KR12 harness note: door choice, control mechanism, fairness

Prepared 2026-11-27 for Andrew's check, beside the harness
source and KR12-RUN-SHEET-DRAFT.md in this directory. All
source citations are to the staged tree
(~/workspace/kernel-trees/linux-6.12.116-kr), which carries
patches 0001-0012 applied; 0013 is not applied there.

## 1. The door: vmalloc/vfree, adopted

Callers of iscandar_alloc in the applied tree, complete list:
mm/slub.c (slab object door, slab_alloc_node; and the
kmalloc-large door, __kmalloc_large_noprof), mm/vmalloc.c
(vm_area_alloc_pages), mm/percpu.c (__alloc_percpu_gfp's
door branch), lib/genalloc.c (served pools only). Every door
tests iscandar_active() and falls back to its stock body when
the engine is not ready. Any of them satisfies the identity
requirement in form; they differ in what else they drag in.

vmalloc is adopted, on four grounds from the sources:

(a) Exact class landing. The door calls
iscandar_alloc((u64)nr_pages << PAGE_SHIFT, PAGE_SIZE, 0)
(mm/vmalloc.c:3643). Alignment is PAGE_SIZE, never above it,
so iscandar_alloc's shifted-grant path (align > PAGE_SIZE)
cannot trigger: total == nr_pages exactly, and the class is
iscandar_class_of(nr_pages) with no correction term. The
request sizes are page-aligned powers of two, and
__vmalloc_area_node computes nr_small_pages from
get_vm_area_size(area), the request with the guard page
already excluded, so vmalloc(4096/32768/65536/131072) puts
exactly 1/8/16/32 pages at the door: nano bottom, nano top,
tiny middle, tiny top.

(b) No precondition. The slab door refuses, with a printed
finding, any allocation from a cache with no registry entry
(slub.c, slab_alloc_node: cache_id == 0 is a refusal, not a
fallback). A harness cache created by a module after boot
would have to be proven registered before its timings meant
anything, and a registration failure would present as a
refusal storm, not a measurement. The vmalloc door has no
registry: cache_id is 0, anonymous, by design.

(c) The stock side is the same function. In the control
boot, vm_area_alloc_pages does not detour: the door branch
is skipped and the function's own stock body (bulk order-0
page allocation) runs. The harness times vmalloc() in both
boots; the call site, the VA machinery around it, and the
mapping work are literally the same code. What changes is
only who supplies the pages, which is the layer's whole
claim to exist.

(d) The alternatives measure the wrong thing or carry
extra terms. The slab object door in the serving boot is a
layer call per object; in the control boot it is SLUB's
per-cpu lockless freelist, a cache of already-supplied
pages: the comparison would be layer-walk versus freelist
hit, which conflates page supply with object caching. The
percpu door multiplies size by num_possible_cpus and serves
mostly boot-time callers. The genalloc door reaches only
pools the layer has agreed to serve (3 of 4 at KR11). The
kmalloc-large door is respectable (align 0, direct) but its
entry threshold is a config-shaped boundary (which sizes
escape the kmalloc caches), one more fact to verify per
build; vmalloc's threshold is the call itself.

Overheads that ride along, stated not hidden: both boots
pay vmalloc's common costs, the vmap_area allocation, the
page-array allocation (itself a small slab call), the
page-table mapping at alloc and the unmap at free, and
vfree's deferred VA purge, which can smear an individual
free sample when a purge batch fires. These are common-mode
in kind: they inflate both distributions alike, and the
paper's figure is the ratio inside each distribution, not
the difference between medians. The one asymmetric term is
the point of the exercise: page supply, engine walk under
the section lock versus the buddy allocator's order-0 paths.
Two deliberate exclusions keep the terms clean: vmalloc, not
vzalloc, so no zeroing is spent on either side (patch 0011's
memset term is the door's, and it stays out); and one block
outstanding at a time, so neither side is measured doing
batch or fragmentation work the other is spared.

## 2. The control: iscandar_hold, already in the build

No new mechanism is needed, and no 0014 draft is offered.
core.c already carries the control: __setup("iscandar_hold",
iscandar_hold_setup) sets a static flag, and iscandar_init
tests it before the canary: state = CLOSED, one pr_info
("iscandar: held shut by iscandar_hold; stock serves"),
return. /proc/iscandar is created before the test, so the
control boot still presents the state window, reading
"state closed" (the KR2 precedent: the window works in the
closed state; the doors take their stock bodies and count
prefix notes).

This beats both alternatives the task named:

- Config-out build (i): the layer is not a config-gated
  subsystem in the patch set; removing it means unapplying
  the door patches, a second tree and a second binary, and
  every difference between the two binaries becomes an
  argument about the control's purity. The hold boot is the
  same binary. There is nothing to argue about.
- Force-closed addition (ii): it would duplicate, worse,
  what the hold already does deliberately and in the build
  of record. Writing it would be inventing a second control
  to stand beside the author's own.

Residual layer presence in the control boot, for the
record: the doors still execute their iscandar_active()
test (one READ_ONCE) and, on the stock path, an
iscandar_prefix_note counter increment. That is the entire
footprint, it is identical in kind to the KR2 closed boot
the paper already reports, and the run sheet requires the
console line and the /proc state line as evidence the hold
took. If either is absent, the boot is not a control and
the sheet says stop.

## 3. Module, not built-in; and how the harness labels itself

CONFIG_MODULES=y and CONFIG_MODULE_UNLOAD=y in the bench
config; no MODVERSIONS, no module-signing enforcement in
the config, so vermagic is the release string plus the
usual flags, and a module built in the telesart tree
against the 0013 build loads in both boots (same binary).
The KR11 carry-in route (ext2 image on virtio-blk) carries
a .ko exactly as it carried the exerciser binary. What
cannot be verified from this VM is the guest userspace's
insmod: the run sheet therefore checks for insmod or
busybox and modules_disabled=0 as a gating step before
either boot's harness step, with a stated stop.

The fallback, flagged and NOT drafted: if the image cannot
load modules at all, the same source compiles built-in with
its proc trigger registered from a late initcall instead of
module_init. That is a new patch against mm/iscandar's
Makefile and Kconfig plumbing, it changes the build of
record's shape for a measurement, and it needs Andrew's
ruling before it exists. The module route makes it
unnecessary unless the bench proves otherwise.

Labelling: no Iscandar symbol is exported (grep: no
EXPORT_SYMBOL anywhere in mm/iscandar), so the module
cannot call iscandar_active(). It instead reads the state
line from /proc/iscandar with filp_open/kernel_read, the
engine's own published state, and prints it verbatim in the
ICH1 header next to boot=serving or boot=control. An
unreadable or unrecognized state aborts the run before any
allocation: the label is evidence, not an operator-supplied
string. Every other symbol the module uses was verified a
plain EXPORT_SYMBOL in the tree (vmalloc_noprof behind the
vmalloc macro, vfree, sort, proc_create, filp_open,
kernel_read, filp_close, simple_read_from_buffer,
mutex_lock_interruptible, default_llseek).

## 4. Statistics choices

- 10,000 samples per size per operation, after 1,000
  warm-up pairs. At door costs of microseconds, one size
  costs a few seconds of guest time; the full set, both
  boots, is minutes. 10,000 makes the median an exact order
  statistic with sub-percent stability for distributions of
  this shape, and the worst a genuine tail observation
  rather than a small-sample accident. Warm-up absorbs the
  first-touch costs (freelist and arena state, page tables,
  cache warmth) that belong to neither distribution.
- All samples are stored (two static arrays of 10,000 u64,
  160 KB of module BSS) and sorted in place: min, median,
  worst are exact. Median convention: the element at index
  n/2 after the ascending sort (upper median at even n),
  stated here and in the source so the paper can state it
  in one clause. The mean is printed because readers ask
  for it; the ratio_milli (worst x 1000 / median) is the
  figure the comparison rides on, an integer so the log
  format is fixed and parseable: ICH1 lines, one per size
  per operation, keys in fixed order.
- Alloc and free are timed separately, matching the latency
  ledger's two note sites; the touch between them (one byte
  per page, plus a sink the compiler cannot elide) keeps the
  grant honest work without adding a memset term.
- Interrupts stay on, in process context. get_cycles is
  the ledger's own instrument, so harness cycles and /proc
  worst_cycles are the same unit by construction. Guest IRQ
  and scheduler noise lands in both boots' samples alike;
  the design's answer is the distribution and the ratio,
  and the worst is everywhere described as the worst
  observed under guest conditions.
- Failure discipline: a NULL from vmalloc at any point
  prints an ICH1 abort line naming size and iteration and
  ends the run. No retry, no reduced sample count silently
  reported as the full set.

## 5. Desk checks actually run (this VM, 2026-11-27)

- Reduction logic (sort, median index, mean, ratio)
  extracted and run in userspace on a synthetic long-tailed
  set: min 900, median 999, worst 5,000,000,
  ratio_milli 5,005,005; and on a constant set: ratio_milli
  exactly 1000. Both correct.
- The module object compiles against the staged tree with
  the tree's own kbuild (make M= modules): the .o built
  with no warnings. modpost then failed on this VM for one
  reason only: the copied tree carries no Module.symvers,
  so every external symbol reads undefined to modpost. The
  symbols themselves were each verified EXPORT_SYMBOL in
  the sources (section 3), so the telesart build, which has
  its tree's symvers, is expected to link clean; the run
  sheet's preparation step checks the .ko's vermagic
  against the guest's uname -r before any insmod.
- Not checked anywhere yet: an actual insmod, an actual
  run. Those are bench events, and the sheet treats the
  first Boot A harness run as the harness's own first test,
  with the header's state line as its pass criterion.

## 6. Judgment calls, flagged for Andrew

1. Door = vmalloc. Argued above; the slab door is the
   runner-up and is rejected for its registry precondition
   and its freelist-hit control side, but a reasonable
   reader could want both doors timed. The harness as
   drafted times one door well rather than two doors
   thinly. Ruling invited if he wants the slab door added
   as a second table.
2. One block outstanding. This measures the stationary
   walk, matching the exerciser's discipline. It does not
   measure either allocator under fragmentation or deep
   occupancy; KR12's claim is scoped to the distribution
   of the door call as made from a settled system, and the
   paper should say so rather than let the table imply
   more.
3. The harness runs AFTER the exerciser in Boot A, and the
   sheet takes the step readings of record before insmod,
   so the harness's own 44,000 door calls cannot contaminate
   them; a final harness-inclusive reading is kept and
   labelled as such. In Boot B there is no exerciser run
   (it would time libc malloc against stock, a non-party).
4. The prediction concedes the medians in advance. This is
   deliberate and in Andrew's stated terms (variance, not
   speed), but it is a rhetorical choice as well as a
   measurement one: the paper gives SLUB its due in print
   before the ratios are read. Flagged so the concession is
   his, not the draft's.
5. Boot B's /proc counters move (prefix notes at the
   doors). The sheet does not use them as evidence either
   way; the control's evidence is the console hold line,
   the state line, and the ICH1 table. If Andrew wants the
   control boot's prefix census as a cross-check that the
   harness traffic really went stock-side, it is there to
   be read, and the sheet can be amended to require it.
