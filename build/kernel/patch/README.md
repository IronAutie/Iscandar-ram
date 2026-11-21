# The kernel-round patch set

Status: STAGED, 2026-11-20, and STETTED by Andrew the
same evening. This set is the kernel round's patch
record; the rebase onto the real tree and the compile
are the next steps, in the bench flow. Written against the stetted
design (../KERNEL-BUILD-DESIGN-DRAFT.md, Andrew's stet
2026-11-19) and the stetted coverage audit
(../COVERAGE-AUDIT.md, Andrew's stet 2026-11-20), for
Linux 6.12.116. This set replaces the bridge round's
staged set (../../patch/) for the kernel build: the
doors stand where the bridge patches stood, and every
bridge call becomes a direct in-kernel call under the
design's atomic discipline (R2). The bridge core, the
binding, the canary patch, and the fix round
(bridge 0005 to 0008) do not carry over; the userland
rig is retired and its channel has no counterpart here.

Staging terms, as the bridge set's: each patch is
anchored on its center's definition at the line the
manifest records against the pristine tree, the
inserted code is the design's content, and the
surrounding context lines are schematic. On telesart,
against the real tree, each patch is re-based (context
regenerated with git diff) before it is applied.
Nothing here has been applied to any tree, and nothing
here claims a compile. Line anchors in this set were
checked against a pristine 6.12.116 tree on this
machine at staging time, which the bridge set could
not do; the check is recorded in the manifest.

## The set

- `0001-kernel-core.patch`: apply FIRST. The core the
  doors call: a new directory mm/iscandar/ holding the
  glue (mm/iscandar/core.c), the marked mass
  (mm/iscandar/iscandar_mass.s, copied verbatim from
  ../iscandar_mass.s at application time), the engine
  text (copied verbatim from the c-translation and
  engine repositories at application time; R9, one
  engine text), the glue header
  (include/linux/iscandar.h), the build wiring
  (mm/iscandar/Makefile, one line in mm/Makefile), and
  the linker placement of the .iscandar_state and
  .iscandar_arena sections. The core owns: the engine
  seated over the mass by cast, the irqsave alloc and
  free glue with the extent ledger, the cache registry
  in the mass's third region (control path, mutex,
  confirm-then-commit), the gen_pool served-set gate
  under the audit's backing rule, the early-boot
  prefix machinery (R11), the latency ledger (design
  section 5), the init canary and the ready flag, the
  iscandar_hold boot parameter (the degrade proof's
  instrument), and /proc/iscandar, the state read.
- `0002-slab-centers.patch`: mm/slub.c. The doors at
  1.1, 1.3, and 1.4 in their kernel form, and the
  watch-point counter at alloc_slab_page, unchanged
  from the bridge staging.
- `0003-cache-setup.patch`: mm/slab_common.c. Cache
  setup 1.5 against the in-kernel registry; 1.6 by
  composition, as before.
- `0004-vmalloc-backing.patch`: mm/vmalloc.c. The
  backing at 2.3 as one exact-fit run through the
  direct call, and the free doors at 2.4. vfree_atomic
  takes its door in this build: the bridge staging
  left that question open, and the design's section 3
  closes it (the section is sleep-free, so atomic
  callers are served).
- `0005-second-ring-and-bulk.patch`: mm/percpu.c,
  lib/genalloc.c, mm/slub.c. Center 4.2 in kernel
  form (the bridge door's atomic-context refusal has
  no remaining reason and is removed; the mirrored
  rejection set stays), center 4.3 gated by the
  backing rule (the gate's test runs where chunks are
  added; the doors consult the recorded decision),
  and the alloc_bulk funnel hunk in mm/slub.c, the
  audit's item 2 as ruled: the private per-CPU drain
  in __kmem_cache_alloc_bulk is replaced by a loop
  over the door'd center, kfence first, the
  all-or-nothing error path unchanged. Center 4.1
  (mempool) takes no hunk, as before: served through
  its backings.

## Rules the hunks obey

- No silent fallback (standing). A refusal at a
  patched center after the prefix is the verdict: the
  door returns its failure shape. The only stock path
  after the prefix is the free doors' range decision,
  the design's named seam: a pointer outside the
  arena was born outside it.
- The prefix is bounded by the core's init, not by a
  raise: before the canary passes and ready opens,
  every center is stock, counted per center and
  logged, exactly as R11 states. A canary failure, or
  iscandar_hold on the command line, leaves the
  engine shut for the boot (state closed) and stock
  serves; that is the degrade proof's shape, not an
  error path.
- Atomic discipline (R2, R3): every alloc and free
  door's engine call runs inside one spin_lock_irqsave
  section in the core. Nothing inside the section
  sleeps; the sleep-free audit that ran on the harness
  objects (2026-11-19) is a build gate here, re-run on
  the kernel objects (verification step 2).
- One engine text (R9): the engine sources are copied
  verbatim at application time from their homes of
  record, never retyped into a patch. The only
  platform seam in their build is the established
  prefix header defining __declspec away, carried in
  mm/iscandar/ as declspec.h and applied to that
  directory's objects only.
- Constants have one home: the build constants in
  ../../bridge/iscandar_bridge.h (read through
  ../../constants/iscandar-build.conf), the engine
  geometry in the engine headers, the mass layout in
  ../MASS-LAYOUT.md. No patch defines a constant of
  its own.
