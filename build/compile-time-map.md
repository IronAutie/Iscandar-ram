# The compile-time memory map (Fubuki live build)

Paper 3, the build's first artifact, drawn 2026-11-11. This
document states the guest layout of the patched Fubuki boot
and every region the build reserves before the kernel is
compiled, so that a mismatch fails the build rather than the
boot (Andrew's ruling, 2026-11-11). Every constant named here
is defined exactly once, in `build/bridge/iscandar_bridge.h`;
this document is the arithmetic that justifies the header,
and `build/link-check.sh` verifies the two agree.

## The guest

QEMU hands Fubuki 512 MiB of RAM (the boot of record, Stage B
of the build and boot sheet, runs at `-m 512`). Inside that
one machine, four regions concern this build:

1. The stock system: kernel, initramfs, and userland, served
   by the stock page allocator, which stands underneath as
   substrate (ruling 1). Its demand is the substrate's own
   business and is not charged to this map, except in the
   resident arithmetic below.
2. The arena: the simulated SRAM, Iscandar's store, raised by
   the user-space translation layer by mmap from the
   substrate.
3. The bridge channel: one reserved region, in neither side's
   allocation, through which the patched centers reach the
   layer.
4. The translation layer's own working memory: reserved in
   this map, outside the arena, never Iscandar-serviced
   (Andrew's ruling, 2026-11-11: the layer must hold memory
   before the first grant can be recorded, and it must keep
   standing when a grant fails).

## The arena

Calibration: one engine sector is one guest page, 4 KiB
(`ISCANDAR_PAGE_BYTES` 4096). A cube is 32,768 sectors
(`ISCANDAR_CUBE_PAGES`), so a cube spans 128 MiB
(`ISCANDAR_CUBE_BYTES` 134,217,728). Cube c's base in the
simulated SRAM is the arena base plus c times the cube span,
the address form of B_4(c, x) = c * 32768 + B_3(x); no table
mediates the correspondence. The arena base is fixed at the
raise and published by the layer in the channel control
block.

The arena of record is 4 cubes (`ISCANDAR_ARENA_CUBES`):
131,072 pages (`ISCANDAR_ARENA_PAGES`), 512 MiB
(`ISCANDAR_ARENA_BYTES` 536,870,912) of virtual reservation.

### The resident arithmetic, and what is provisional

A virtual reservation is not a resident charge: the
substrate faults arena pages in only as the engine grants
them. What the guest must actually hold at the peak is the
granted total. The evidence, from the control trace and the
Paper 2 replay at this same calibration:

- The recorded boot asked for 383.69 MiB across 812,106
  allocations, with 13.38 MiB requested live at the peak in
  56,023 objects, and a largest single request of 996,600
  bytes (244 pages).
- Replayed at one sector to one page, the Beam's peak
  granted total was 56,535 pages: 56,535 x 4,096 =
  231,567,360 bytes, 220.8 MiB. (The buddy control peaked at
  56,599 granted pages.)
- The fixtures (below) add 84 pages, touched at the raise.

So the arena's resident peak is about 221 MiB. Against the
512 MiB guest, that leaves about 291 MiB for the stock
system and the layer. The one whole-system measurement on
record, from the stock boot's own counters (2026-11-03), put
the stock system's practical occupancy (MemTotal minus
MemAvailable) at 75.9 MiB after the boot churn, a figure that
already includes the stock slab backing the live build
replaces. 291 MiB of room against a measured 75.9 MiB of
whole-system occupancy is headroom of about 3.8 to 1.

PROVISIONAL: the arena stands at 4 cubes, the configuration
of record, and the arithmetic above says the resident peak
fits the guest with room. What no input measures is the
substrate's page-direct demand during the patched boot
itself (the control trace armed the kmem family only), so
the fit is a computed expectation, not a measurement. If the
evidence boot shows substrate pressure, the fallback is
settled in advance and costs one line: `ISCANDAR_ARENA_CUBES`
to 2, a 256 MiB arena of 65,536 pages, net served 65,498
after the same fixture charge scaled to two cubes (2 x 19 +
8 = 46 pages). The halved-arena replay of record carried
this same load with no refusals, its 56,535-page peak
standing 8,963 pages inside that net. The prediction sheet
for the evidence run is drawn against whichever cardinality
the header carries when the kernel is built.

## The relocated fixtures

Each cube carries one relocated area at its head, 19 pages
(`ISCANDAR_FIXTURE_PAGES_PER_CUBE`), pre-allocated at the
raise and marked by hand, in the manner of the 80188 design:

- The cube's slice of the extent ledger: 16 pages
  (`ISCANDAR_FIXTURE_LEDGER_PAGES`). One bin per page of the
  cube, 32,768 bins at 2 bytes: 65,536 bytes, exactly 16
  pages. At grant time the length of the grant is stuffed
  into the starting page's bin; zero is Nothing, no grant
  starts here. The ledger lives in the arena, in this slice;
  it is not a layer-side copy.
- The cube's maps and counters: 3 pages
  (`ISCANDAR_FIXTURE_MAPS_PAGES`). The serving state's size
  was measured from the C structs in 2026-11-11 session: per
  cube, the cube store 1,024 x 4 B = 4,096 B, the row
  counters 4,096 B, the plane counters 128 B, the overall
  counters 8 B; 8,328 B in all, which stands inside 3 pages
  (12,288 B) with 3,960 B of slack. In this build the live
  engine structs are seated in the layer's reserved state
  region (see below); the fixture area in the arena carries
  the charge, so the arena's accounting is the same either
  way: the pages stand permanently live and are never
  granted.

Four cubes at 19 pages: 76 pages. The prediction sheet names
76 as the fixtures' opening live figure at the raise, before
the cache registry's charge, which follows.

## The cache registry fixture

The registry is the translation layer's record of the named
caches, one entry per cache the kernel creates (center 1.5).
By Andrew's ruling of 2026-11-11 it is seated inside the
governed memory among the fixtures, under a hard cap, sized
when the build is drawn; a creation past the cap is refused,
and the refusal is a finding.

- Cap: 512 entries (`ISCANDAR_REGISTRY_CAP`). Derivation:
  the sift counted 464 `kmem_cache_create` call sites in the
  whole 6.12.116 tree; Fubuki compiles a subset of the tree,
  so 464 bounds any count her build can create statically,
  and the kmalloc size caches (built by
  `create_kmalloc_caches`, some 30 across both kmalloc
  types) ride inside the same registry. 464 + 30 = 494,
  rounded up to 512. PROVISIONAL in one stated respect: the
  compiled-subset enumeration counted patch points, not
  caches, so the cap rests on the sift bound, not on a count
  of Fubuki's own caches; extending that enumeration to
  caches would replace the bound with a count.
- Entry: 64 bytes (`ISCANDAR_REGISTRY_ENTRY_BYTES`): object
  size, alignment, flags, constructor cookie, live count,
  and the cache's name.
- Seat: 512 x 64 B = 32,768 bytes, 8 pages
  (`ISCANDAR_REGISTRY_PAGES`), in cube 0's fixture area
  behind its 19 pages, at page offset 19
  (`ISCANDAR_REGISTRY_OFFSET_PAGES`) of cube 0. Cube 0's
  fixture run at the raise is therefore 27 pages; cubes 1
  to 3 carry 19 each.

Fixture charge in total (`ISCANDAR_FIXTURE_PAGES_TOTAL`):
76 + 8 = 84 pages. Net served capacity
(`ISCANDAR_NET_SERVED_PAGES`): 131,072 - 84 = 130,988 pages.
The manuscript's relocated-areas figure (76 pages, net
130,996) predates the registry ruling; the difference is
exactly the registry's 8 pages, charged here so the build's
prediction sheet and the arena's books agree to the page.

## The bridge channel

Reserved in this map at 512 KiB: 128 pages
(`ISCANDAR_CHANNEL_PAGES`, `ISCANDAR_CHANNEL_BYTES`
524,288), in neither side's allocation. Layout by page:

- Control: 1 page (`ISCANDAR_CHANNEL_CONTROL_PAGES`). The
  control block (magic, version, the arena base published at
  the raise, ring indices, served and refusal counters, the
  early-boot prefix counters) stands at its head.
- Request ring: 32 pages (`ISCANDAR_CHANNEL_REQ_PAGES`) of
  64-byte records (`ISCANDAR_RECORD_BYTES`): 2,048 records
  (`ISCANDAR_RING_RECORDS`).
- Response ring: 32 pages (`ISCANDAR_CHANNEL_RESP_PAGES`),
  the same record shape, 2,048 records.
- Early-boot prefix log: 63 pages
  (`ISCANDAR_CHANNEL_PREFIX_PAGES`) of 16-byte entries
  (`ISCANDAR_PREFIX_ENTRY_BYTES`): 16,128 entries
  (`ISCANDAR_PREFIX_ENTRIES`), with exact per-center
  counters kept in the control block beside it.

Sizing: serving is synchronous, a caller waits for its
response, so in-flight depth is bounded by the guest's CPU
count, and a 2,048-record ring stands orders of magnitude
past any burst the boot can raise; the frees that may be
batched meet the same depth against a measured whole-boot
free total of 838,318 spread over the boot's course. The
prefix log's 16,128 entries hold detail; the control block's
counters are exact regardless, and an overflow sets a flag
there rather than passing silently.

PROVISIONAL: the channel reservation is a build figure. Any
size at or past the burst bound serves; 512 KiB is the drawn
figure, and it changes in exactly one place, the header.

## The translation layer's reservation

Reserved in this map, outside the arena: 160 pages
(`ISCANDAR_LAYER_RESERVE_PAGES`), 640 KiB. The accounting:
the Beam state struct measures 266,512 B (16 B of header
plus 32 engine slots at 8,328 B), 66 pages rounded up; the
layer's binary and data allowance 64 pages; stacks 16
pages; slack 14 pages. The layer seats its Beam in this
region, in storage the layer itself provides: the
transcription allocates nothing, so where the structs stand
is the caller's choice by construction, and the choice here
is this reserved region. The extent ledger is not in this
region: it stands in the arena fixtures, above. The
registry is not here either: it stands in cube 0's fixture.
What stands here is machinery, not data about the served
world, and the bootstrap argument of Andrew's ruling is the
reason: the layer records grants in this memory, so this
memory cannot itself be a grant.

## Summary table

| Region | Pages | Bytes | Charged to |
|---|---|---|---|
| Arena (4 cubes, virtual) | 131,072 | 536,870,912 | substrate, on fault |
| Fixtures: ledgers (4 x 16) | 64 | 262,144 | arena, at the raise |
| Fixtures: maps and counters (4 x 3) | 12 | 49,152 | arena, at the raise |
| Fixture: cache registry (cube 0) | 8 | 32,768 | arena, at the raise |
| Net served arena | 130,988 | 536,526,848 | callers, through the centers |
| Bridge channel | 128 | 524,288 | this map's reservation |
| Translation layer | 160 | 655,360 | this map's reservation |

Every figure in this table is a header constant or a sum of
header constants, and the link-check recomputes the sums.
