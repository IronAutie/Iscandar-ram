# Build status, 2026-11-11

The Paper 3 live build, as far as this VM carries it. Every
figure below was run or read on this machine today; anything
not run is named as not run. The allocator core is the
author's; this build is scaffolding, patching, and linkage
at his direction.

## What stands built

- `build/compile-time-map.md`: the guest layout for the
  512 MiB QEMU boot. Arena 4 cubes, 512 MiB virtual, 131,072
  pages; fixtures 19 pages per cube (16 ledger, 3
  maps/counters) plus the registry's 8 pages seated in
  cube 0, 84 pages total; net served 130,988 pages. The
  manuscript's net (130,996) predates the registry charge;
  the difference is exactly the registry's 8 pages.
- `build/bridge/iscandar_bridge.h`: the one shared bridge
  definition. Channel layout, record layout (64 bytes), op
  codes for the reconciled families, center ids, arena and
  fixture constants. Copied verbatim into the kernel tree at
  patch application; included by the translation layer.
  No second copy of any constant exists (checked).
- `build/layer/iscandar_layer.c`: the user-space translation
  layer. Links the admitted C translation (k=4 over the
  k=3 engine in `c-translation/`, with the author's gear
  bodies linked from the engine repo of record, the same
  arrangement as the k=4 test runner). Seats the Beam in
  caller-provided state, maintains the extent ledger (one
  bin per page) and the capped cache registry, speaks the
  bridge protocol. Two modes: `--self-test` and `--serve`.
- `build/patch/`: the staged patch set against Linux
  6.12.116 (0005 bridge core first, then 0001 slab centers,
  0002 cache setup, 0003 vmalloc backing, 0004 second ring),
  with `MANIFEST.md` accounting for every reconciled center
  and `NOT-PATCHED.md` naming every family not served.
- `build/units/`: payload install, layer, raise, and finish
  units and scripts, in the exerciser unit's style, serial
  console as the console of record.
- `build/elbe/fubuki-iscandar.xml`: the ELBE linkage, a
  patched copy of the stock `fubuki.xml`. Installs the
  units, scripts, and the generated constants file inline
  (checked copies of their files of record); the compiled
  payloads ride the host share at first boot, as the kernel
  already does in this arrangement, because ELBE finetuning
  carries text into an image, not host binaries.
- `build/link-check.sh`: the linkage verified end to end.

## Verified on this VM (actual outputs)

Link-check, run 2026-11-11 (also run from an unrelated
directory, same result):

```
PASS build
PASS conf-idempotent
PASS header-vs-map
PASS xml-manifest
PASS xml-copies
PASS self-test
PASS exerciser
PASS no-duplicates
PASS patch-coverage
PASS not-patched
PASS boot-order
LINK-CHECK: all checks PASS
```

Layer self-test: `SELF-TEST: 842 checks, 0 failures; served
82, refusals 0, invariants 4` and `SELF-TEST PASS`. The four
invariant verdicts are the designed ones (double free,
interior free, foreign free, destroy-with-live), each a
status, none a crash; the end state returns to exactly the
84 fixture pages.

Layer channel mode, driven end to end with a hand-written
record in the shared header's format: an 8,192-byte
allocation request was granted 2 pages at arena offset
122,880 (cube 0, local page 30, the first page past the
27-page cube 0 fixture), the cookie echoed, the ledger read
at exit showed 84 pages held of 131,072, and the layer
stopped cleanly on the stop word. This is the bridge
protocol moving a real grant through the admitted engine,
not a mock.

Exerciser standard run: 5,072 allocations, 5,072 frees,
peak live 9,216 pages, end state 0 pages, matching its
prediction sheet exactly. Extended run: 11,120 and 11,120,
same peak, end zero.

One compiler warning stands in the build: an unused label
in the author's `RowsCheck.c`, pre-existing in his file,
left untouched.

## Provisional figures

- Bridge channel, 512 KiB (128 pages). Derived: 32/32
  request and response rings of 64-byte records (4 KiB
  each), a 256 KiB prefix log (sized past the trace's
  largest same-center burst, 11,404 records), one control
  page. No measured pressure fixes it; it is reserved in
  the map and defined in one place.
- Resident fit. The replay's peak grant (56,535 pages,
  220.8 MiB) against the guest's roughly 291 MiB of
  headroom, beside the stock system's measured practical
  occupancy of 75.9 MiB (2026-11-03): the arena's virtual
  size is the design of record and the resident fit is
  provisional until the bench boot measures it. The
  fallback is one line: cardinality 2, the halved-arena
  configuration whose replay already carried this load.
- Registry cap, 512 entries (8 pages). Derived from the
  sift's tree-wide count of creation sites (464) plus the
  kmalloc size caches, rounded up. The compiled subset is
  smaller; the cap is a build figure, and a creation past
  it is refused as a finding.

## Scope notes, stated plainly

- Engine seating: the admitted structures live where the
  caller places them. The layer seats the Beam in its own
  reserved state mapping; the extent ledger's bins are
  served literally from the arena fixtures, as designed.
  The live engine arrays are not byte-seated inside the
  arena's 3-page maps/counters areas (structure stride
  prevents it); the arena charge is identical either way,
  and the maps/counters figure of record is the measured
  serving state.
- Cache service at 1.1 grants a fresh object per request;
  the object returns to the engine at the free door. Cache
  free-list reuse (the per-CPU partial lists and their kin)
  is outside this build's scope by the design's own line:
  bookkeeping belongs to the layer, and reuse policy is a
  further design step, recorded here rather than silently
  absent. See `patch/MANIFEST.md`.
- The layer's `--serve` channel on this VM is a file, the
  rig stand-in for the reserved channel region. The bench
  binding is step 4 of the run list.

## Open items carried, not decided here

- Percpu (4.2): the staged hunk serves one bridge run per
  per-cpu area. Whether the chunk layer's internal carving
  is compatible with that shape is build-time design work
  at the bench; the reconciliation already flags the center
  as a separate design question from the boot-load families.
- vfree_atomic: staged to take the same door as vfree, on
  the ground that the bridge call completes synchronously
  in this build. If the bench build makes the call
  blocking, this door is where the finding lands.
- gen_pool served set: the redirect is gated on a pool
  registered as an arena pool; no pool is registered by
  default. The build names the set one pool at a time.
- The kernel source tree is not on this VM, and no tree was
  downloaded. The patches are staged with schematic context
  (marked as such in each file) and re-base on the real
  6.12.116 tree at the bench.

## The telesart run list, in order

1. Entry check: in `build/`, run `./link-check.sh`. All
   eleven checks should pass there as they do here.
2. Prepare the Linux 6.12.116 source tree (the Fubuki
   kernel's version, per the target description of record).
   Record the tree's origin and a hash of it in this file's
   successor note before patching.
3. Re-base the patch set on the real tree: apply 0005 first,
   then 0001 to 0004. Copy `build/bridge/iscandar_bridge.h`
   into the tree as `include/linux/iscandar.h` verbatim;
   never retype a constant. Resolve each center at its
   patch-point line; the schematic context gives way to the
   tree's real context. Build the kernel with the Fubuki
   config of record (`telezart-kconfig` in the share).
4. Bind the arena physically: reserve the arena span and
   the channel span at boot so the page allocator never
   hands them out; set the kernel's `-m 512` arena base to
   the arena span's physical start; bind the layer's serve
   path to the reserved channel region in place of the
   rig's file; confirm the raise registers the prefix-born
   caches from the prefix counters.
5. Stage the payload: `make` in `build/`, then copy
   `build/bin/iscandar-layer` and `build/bin/iscandar-exerciser`
   into `~/telezart/share/iscandar-payload/`.
6. Submit the image by absolute path, per the build sheet:
   `elbe initvm submit /home/andrew/telezart/fubuki-iscandar.xml`
   (the patched XML placed at that path), and build the
   image from the resulting build folder.
7. Boot the evidence run: QEMU at `-m 512`, the patched
   kernel and its initrd handed in by `-kernel` and
   `-initrd` (the Fubuki arrangement), the trace armed
   (`trace_pipe_arm`), `iscandar_channel=` and
   `iscandar_arena=` naming the reserved regions of the
   compile-time map, the 9p share up for the payload and
   the exports, the serial console on ttyS0 as the console
   of record.
8. Read the evidence from the exported `iscandar-results`:
   the layer ledger (end state, refusals, invariants, and
   the alloc_slab_page watch counter, expected zero for
   redirected caches after the raise), the exerciser
   ledger, the channel snapshot, the bridge counters, with
   the boot trace and stats beside them. The Discussion's
   two questions (a direct replacement? does it run without
   a panic?) are answered from these, and from nothing else.

## Commits

Local, in order: `9a83525` (compile-time map), `40e5050`
(translation layer), `e8240dd` (staged patch set),
`5d2b51c` (boot services), `5c57517` (ELBE linkage),
`c8fa534` (link-check), `d4337c4` (this report), `0afa340`
(center count correction: the prefix counters are indexed
by center id, ids to 43, so the count is 48; the layer, the
constants, and the bridge core already carried 48, and this
brings the header of record into line). Nothing was pushed
to GitHub; the push waits for Andrew's token. Overleaf and
paper-3.tex were not touched.
