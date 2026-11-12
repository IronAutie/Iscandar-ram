/* iscandar_bridge.h -- the ONE shared bridge definition for
 * Paper 3 ("Debian to Iscandar: Full Speed Ahead!").
 *
 * This header is the single home of every bridge and arena
 * constant in the live build. The kernel-side patch (staged in
 * build/patch/) copies this file verbatim into the kernel tree
 * at application time; the user-space translation layer
 * (build/layer/) includes it directly. No constant defined here
 * may be defined again anywhere else in the build: the
 * link-check (build/link-check.sh) greps for duplicates.
 *
 * The figures themselves are stated, with their arithmetic, in
 * build/compile-time-map.md. Figures marked PROVISIONAL there
 * are build parameters, and this header is the one place they
 * are set, so changing one is a one-line act.
 *
 * Doctrine (project law, carried from the engine): a failure
 * is a status, never a crash; no retries, no silent fallbacks.
 * The bridge status codes below are the layer's own namespace,
 * distinct from the engine's Status.py codes, which the engine
 * keeps to itself behind the Beam boundary. */

#ifndef ISCANDAR_BRIDGE_H
#define ISCANDAR_BRIDGE_H

#include <stdint.h>

/* ---- The calibration and the arena --------------------------
 * The standing calibration of the live build: one engine
 * sector is one guest page, 4 KiB. A cube is 32,768 sectors,
 * so a cube spans 128 MiB of the simulated SRAM, and the
 * arena of record is four cubes, 512 MiB, 131,072 pages. */

#define ISCANDAR_PAGE_BYTES        4096u
#define ISCANDAR_CUBE_PAGES        32768u   /* sectors per cube */
#define ISCANDAR_CUBE_BYTES        (ISCANDAR_CUBE_PAGES * ISCANDAR_PAGE_BYTES)
#define ISCANDAR_ARENA_CUBES       4        /* PROVISIONAL: see map */
#define ISCANDAR_ARENA_PAGES       (ISCANDAR_ARENA_CUBES * ISCANDAR_CUBE_PAGES)
#define ISCANDAR_ARENA_BYTES       (ISCANDAR_ARENA_PAGES * ISCANDAR_PAGE_BYTES)

/* Cube c's base in the simulated SRAM is the arena base plus
 * c times the cube span: the address form of B_4(c, x) =
 * c * 32768 + B_3(x). No table mediates it. The arena base
 * itself is fixed at the raise and published by the layer in
 * the channel control block (arena_base below). */

/* ---- The relocated fixtures ---------------------------------
 * Each cube carries one relocated area at its head, marked by
 * hand at the raise: the cube's slice of the extent ledger,
 * then its maps and counters. The cache registry is seated in
 * cube 0's area behind those, under its hard cap. */

#define ISCANDAR_FIXTURE_LEDGER_PAGES   16u  /* 32,768 bins x 2 B */
#define ISCANDAR_FIXTURE_MAPS_PAGES     3u   /* engine state, 8,328 B */
#define ISCANDAR_FIXTURE_PAGES_PER_CUBE (ISCANDAR_FIXTURE_LEDGER_PAGES \
                                         + ISCANDAR_FIXTURE_MAPS_PAGES)
#define ISCANDAR_REGISTRY_CAP           512u /* entries; see map */
#define ISCANDAR_REGISTRY_ENTRY_BYTES   64u
#define ISCANDAR_REGISTRY_PAGES         8u   /* 512 x 64 B = 32 KiB */
#define ISCANDAR_REGISTRY_CUBE          0u
#define ISCANDAR_REGISTRY_OFFSET_PAGES  ISCANDAR_FIXTURE_PAGES_PER_CUBE
#define ISCANDAR_FIXTURE_PAGES_TOTAL    (ISCANDAR_FIXTURE_PAGES_PER_CUBE \
                                         * ISCANDAR_ARENA_CUBES \
                                         + ISCANDAR_REGISTRY_PAGES)
#define ISCANDAR_NET_SERVED_PAGES       (ISCANDAR_ARENA_PAGES \
                                         - ISCANDAR_FIXTURE_PAGES_TOTAL)

/* ---- The translation layer's own reservation ----------------
 * The layer's working memory stands reserved in the compile-
 * time map, outside the arena, by Andrew's ruling: it is never
 * Iscandar-serviced. The figure is a reservation in the map's
 * accounting: Beam state 66 pages (266,512 B of struct),
 * binary and data allowance 64 pages, stacks 16 pages, slack
 * 14 pages. */

#define ISCANDAR_LAYER_RESERVE_PAGES    160u

/* ---- The bridge channel -------------------------------------
 * Reserved in the compile-time map, in neither side's
 * allocation. Layout by page: control 1, request ring 32,
 * response ring 32, early-boot prefix log 63. Total 128
 * pages, 512 KiB. PROVISIONAL as a build figure: the burst
 * arithmetic in the map shows the rings are generously deep,
 * and the reservation is one line here. */

#define ISCANDAR_CHANNEL_PAGES          128u
#define ISCANDAR_CHANNEL_BYTES          (ISCANDAR_CHANNEL_PAGES \
                                         * ISCANDAR_PAGE_BYTES)
#define ISCANDAR_CHANNEL_CONTROL_PAGES  1u
#define ISCANDAR_CHANNEL_REQ_PAGES      32u
#define ISCANDAR_CHANNEL_RESP_PAGES     32u
#define ISCANDAR_CHANNEL_PREFIX_PAGES   63u
#define ISCANDAR_RECORD_BYTES           64u
#define ISCANDAR_RING_RECORDS           (ISCANDAR_CHANNEL_REQ_PAGES \
                                         * ISCANDAR_PAGE_BYTES \
                                         / ISCANDAR_RECORD_BYTES)
#define ISCANDAR_PREFIX_ENTRY_BYTES     16u
#define ISCANDAR_PREFIX_ENTRIES         (ISCANDAR_CHANNEL_PREFIX_PAGES \
                                         * ISCANDAR_PAGE_BYTES \
                                         / ISCANDAR_PREFIX_ENTRY_BYTES)

#define ISCANDAR_CHANNEL_MAGIC          0x49534352u /* "ISCR" */
#define ISCANDAR_CHANNEL_VERSION        1u

/* Centers, for the prefix counters and the patch manifest.
 * The numbers are the patch-point list's own. */
#define ISCANDAR_CENTER_SLAB_ALLOC      11  /* 1.1 slab_alloc_node */
#define ISCANDAR_CENTER_KMALLOC_NODE    12  /* 1.2 __do_kmalloc_node */
#define ISCANDAR_CENTER_KMALLOC_LARGE   13  /* 1.3 __kmalloc_large_noprof */
#define ISCANDAR_CENTER_FREE            14  /* 1.4 kfree / kmem_cache_free */
#define ISCANDAR_CENTER_CACHE_SETUP     15  /* 1.5 __kmem_cache_create_args */
#define ISCANDAR_CENTER_KREALLOC        16  /* 1.6 krealloc_noprof */
#define ISCANDAR_CENTER_VMALLOC         21  /* 2.1 center */
#define ISCANDAR_CENTER_VMALLOC_BACKING 23  /* 2.3 vm_area_alloc_pages */
#define ISCANDAR_CENTER_VFREE           24  /* 2.4 vfree */
#define ISCANDAR_CENTER_MEMPOOL         41  /* 4.1 mempool_alloc_noprof */
#define ISCANDAR_CENTER_PERCPU          42  /* 4.2 pcpu_alloc_noprof */
#define ISCANDAR_CENTER_GENPOOL         43  /* 4.3 gen_pool_alloc_algo_owner */
#define ISCANDAR_CENTER_COUNT           16

/* ---- Operations ---------------------------------------------
 * One record crosses per call. The op names the reconciled
 * center the call arrived through, so the layer's ledger can
 * be audited family by family against the reconciliation. */

#define ISCANDAR_OP_NOP             0u
#define ISCANDAR_OP_ALLOC           1u  /* 1.1/1.2: byte-size request */
#define ISCANDAR_OP_ALLOC_LARGE     2u  /* 1.3: oversize kmalloc */
#define ISCANDAR_OP_FREE            3u  /* 1.4: free by arena offset */
#define ISCANDAR_OP_CACHE_CREATE    4u  /* 1.5: register a named cache */
#define ISCANDAR_OP_CACHE_DESTROY   5u  /* 1.5: teardown door */
#define ISCANDAR_OP_CACHE_ALLOC     6u  /* 1.1 via a named cache */
#define ISCANDAR_OP_KREALLOC        7u  /* 1.6: composition */
#define ISCANDAR_OP_VMALLOC_BACKING 8u  /* 2.3: one exact-fit run */
#define ISCANDAR_OP_VFREE           9u  /* 2.4: free a vmalloc run */
#define ISCANDAR_OP_MEMPOOL_ALLOC   10u /* 4.1: backing named in aux */
#define ISCANDAR_OP_PERCPU_ALLOC    11u /* 4.2 */
#define ISCANDAR_OP_PERCPU_FREE     12u /* 4.2 free door */
#define ISCANDAR_OP_GENPOOL_ALLOC   13u /* 4.3 */
#define ISCANDAR_OP_GENPOOL_FREE    14u /* 4.3 free door */

/* aux tags for ISCANDAR_OP_MEMPOOL_ALLOC: which backing the
 * pool was created over (reconciliation, Family 4a). */
#define ISCANDAR_MEMPOOL_BACKING_SLAB  0u /* served at 1.1's path */
#define ISCANDAR_MEMPOOL_BACKING_PAGES 1u /* substrate, not served */

/* ---- Bridge status codes ------------------------------------
 * Returned in the response record's status field. Distinct
 * from the engine's statuses: these are the layer's verdicts
 * at its own boundary. */

#define ISCANDAR_BRIDGE_OK          0u
#define ISCANDAR_BRIDGE_REFUSED     1u  /* the engine refused: no fit */
#define ISCANDAR_BRIDGE_INVARIANT   2u  /* invariant violation: a zero
                                           ledger bin at a free, an
                                           unknown cache, a bad offset.
                                           Debugger time; never retried. */
#define ISCANDAR_BRIDGE_SUBSTRATE   3u  /* not served here, by ruling:
                                           the request belongs to the
                                           page allocator underneath. */
#define ISCANDAR_BRIDGE_BADOP       4u  /* an op this build does not know */

/* ---- The record ---------------------------------------------
 * 64 bytes in both directions. In a request: op set, cookie
 * the caller's token (echoed verbatim), size the byte request
 * (for frees and krealloc's old block, size carries the arena
 * offset instead; see the layer's contract in
 * build/layer/iscandar_layer.c), align in bytes (0 means the
 * page), cache_id for cache ops, aux op-specific. In a
 * response: status set, result the arena offset of the grant
 * (byte offset from the arena base), aux the granted page
 * count. */

typedef struct {
    uint32_t op;
    uint32_t status;
    uint64_t cookie;
    uint64_t size;
    uint32_t align;
    uint32_t cache_id;
    uint64_t result;
    uint64_t aux;
    uint8_t  reserved[8];
} IscandarBridgeRecord;

/* ---- The channel control block ------------------------------
 * Page 0 of the channel. The layer writes it at the raise;
 * the kernel side reads the arena base from it and keeps its
 * prefix counters here during the early-boot prefix. */

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint64_t arena_base;        /* the layer's arena mapping base */
    uint32_t arena_cubes;
    uint32_t channel_pages;
    uint32_t req_head;          /* layer reads at req_head */
    uint32_t req_tail;          /* kernel writes at req_tail */
    uint32_t resp_head;         /* kernel reads at resp_head */
    uint32_t resp_tail;         /* layer writes at resp_tail */
    uint64_t requests_served;
    uint64_t refusals;
    uint64_t invariant_violations;
    uint32_t prefix_calls[ISCANDAR_CENTER_COUNT];
    uint32_t prefix_log_written;
    uint32_t prefix_overflow;   /* nonzero: the detail log filled;
                                   the counters above stay exact */
    uint32_t stop;              /* either side sets: serving ends */
} IscandarChannelControl;

/* One early-boot prefix log entry, 16 bytes. Written by the
 * kernel side for stock-served calls during the prefix, so
 * the prefix is logged, not missed (ruling 4). */
typedef struct {
    uint32_t center;            /* ISCANDAR_CENTER_* */
    uint32_t size;              /* the byte request */
    uint64_t token;             /* the pointer the stock side gave */
} IscandarPrefixEntry;

#endif /* ISCANDAR_BRIDGE_H */
