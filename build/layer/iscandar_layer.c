/* iscandar_layer.c -- the user-space translation layer of the
 * Paper 3 live build.
 *
 * The layer stands between the patched kernel's allocation
 * centers and the Iscandar engine. It speaks the bridge
 * protocol defined in build/bridge/iscandar_bridge.h (the one
 * shared definition), and it serves requests from the k=4
 * Beam over the real k=3 engine, admitted in c-translation/
 * under the standing rule (C admitted only where it
 * reproduces the Python reference engine's outputs exactly). The engine
 * core is Andrew's design; this file is scaffolding around
 * it: the raise, the fixtures, the extent ledger, the cache
 * registry, and the serving loop.
 *
 * Contract of record (the rulings the layer embodies):
 *
 * - The arena is raised by mmap from the substrate (the stock
 *   page allocator underneath, which parcels nothing for
 *   callers). The calibration is one sector to one page.
 * - At the raise, each cube's fixture is marked by hand, in
 *   the 80188 manner: the fixture run stands in the marks
 *   from the first moment, never requested, granted, or
 *   freed. The extent ledger stands in the arena, in each
 *   cube's fixture slice; the cache registry stands in cube
 *   0's fixture, under its hard cap.
 * - A grant's length lives nowhere in a real pointer, so the
 *   ledger carries it: at grant time the page count is
 *   stuffed into the starting page's bin. A free reverses the
 *   address math, reads the count back, completes the
 *   CPUPointer, and the engine's inverse bijection runs. A
 *   zero bin at a free is a foreign, interior, or double
 *   free: an invariant violation, reported as a status.
 * - Named caches cross as requests, size and alignment only;
 *   all cache bookkeeping (the registry below) is the
 *   layer's. This accommodation is an expansion of Iscandar,
 *   to be identified as such in a future paper.
 * - Failures return status codes. Nothing here crashes,
 *   retries, or falls back silently.
 *
 * Modes: --self-test runs a recorded-style request sequence
 * in-process and checks the balances; --canary runs the
 * cross-boundary canary against the live build and exits
 * (Task 7); --serve PATH runs the channel loop over a
 * shared channel file at PATH (the rig); --serve-region
 * runs the same loop over the live build's reserved
 * channel region, with the arena raised on the kernel's
 * arena span (Task 6). */

#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "iscandar_bridge.h"

#include "Beam.h"
#include "BeamCoordinate.h"
#include "CPUPointer.h"
#include "K4Status.h"
#include "MPUPointer.h"
#include "status.h"

/* Compile-time shape checks (the typedef form, C99-safe). */
typedef char assert_record_64[
    (sizeof(IscandarBridgeRecord) == ISCANDAR_RECORD_BYTES) ? 1 : -1];
typedef char assert_control_fits[
    (sizeof(IscandarChannelControl) <= ISCANDAR_PAGE_BYTES) ? 1 : -1];
typedef char assert_prefix_16[
    (sizeof(IscandarPrefixEntry) == ISCANDAR_PREFIX_ENTRY_BYTES) ? 1 : -1];
/* The finish script sets the stop word at a fixed offset in
 * the channel file; this assertion ties that offset to the
 * header's struct, so a layout change breaks this build
 * rather than the bench run. */
typedef char assert_stop_offset[
    (offsetof(IscandarChannelControl, stop) == 264) ? 1 : -1];

/* The cache registry entry: the layer's bookkeeping for one
 * named cache. 64 bytes, the size the map charges. */
typedef struct {
    uint64_t size;         /* object size in bytes */
    uint32_t align;        /* alignment in bytes */
    uint32_t flags;        /* caller's flags, carried verbatim */
    uint64_t ctor_cookie;  /* constructor token, caller's own */
    uint32_t live;         /* objects currently granted */
    uint32_t used;         /* nonzero: this slot is a live cache */
    uint64_t name_token;   /* the kernel cache pointer, as a token */
    uint8_t  pad[24];
} RegistryEntry;

typedef char assert_entry_64[
    (sizeof(RegistryEntry) == ISCANDAR_REGISTRY_ENTRY_BYTES) ? 1 : -1];

typedef struct {
    unsigned char *arena;          /* the simulated SRAM mapping */
    Beam          *beam;           /* seated in the state region */
    void          *state_region;   /* the layer's reserved state */
    size_t         state_bytes;
    IscandarChannelControl *control; /* channel page 0 (or local) */
    FILE          *ledger_file;    /* may be NULL */
    uint64_t       served;
    uint64_t       refusals;
    uint64_t       invariants;
} Layer;

static void log_line(Layer *L, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    if (L && L->ledger_file) {
        va_start(ap, fmt);
        vfprintf(L->ledger_file, fmt, ap);
        va_end(ap);
        fflush(L->ledger_file);
    }
    fflush(stdout);
}

/* ---- Region mode ------------------------------------------------
 * --serve-region binds the layer to the live build's reserved
 * spans instead of the rig's file: the arena is the kernel's
 * arena span, mapped through /dev/iscandar (the bridge core's
 * device) at file offset ISCANDAR_CHANNEL_BYTES, and the
 * channel is the reserved channel region at offset 0. The
 * addresses come from the boot line, read out of
 * /proc/cmdline: one home for them. */

static int      g_region;          /* nonzero in region mode */
static uint64_t g_channel_phys;
static uint64_t g_arena_phys;
static int      g_dev_fd = -1;     /* /dev/iscandar, region mode */

static int cmdline_phys(const char *key, uint64_t *out)
{
    FILE *f = fopen("/proc/cmdline", "r");
    char buf[4096];
    size_t n, klen = strlen(key);
    const char *p;

    if (!f) {
        return -1;
    }
    n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = '\0';
    p = buf;
    while ((p = strstr(p, key)) != NULL) {
        if ((p == buf || p[-1] == ' ') && p[klen] == '=') {
            *out = strtoull(p + klen + 1, NULL, 0);
            return 0;
        }
        p += klen;
    }
    return -1;
}

/* The ledger slice for a cube: 32,768 bins of 2 bytes, in the
 * arena at the cube's head. */
static uint16_t *ledger_of(Layer *L, uint32_t cube)
{
    return (uint16_t *)(void *)(L->arena
                 + (size_t)cube * ISCANDAR_CUBE_BYTES);
}

/* The registry: seated in cube 0's fixture, behind its 19
 * pages. */
static RegistryEntry *registry_of(Layer *L)
{
    return (RegistryEntry *)(void *)(L->arena
                 + (size_t)ISCANDAR_REGISTRY_CUBE * ISCANDAR_CUBE_BYTES
                 + (size_t)ISCANDAR_REGISTRY_OFFSET_PAGES
                   * ISCANDAR_PAGE_BYTES);
}

static uint32_t fixture_pages_for(uint32_t cube)
{
    uint32_t pages = ISCANDAR_FIXTURE_PAGES_PER_CUBE;
    if (cube == ISCANDAR_REGISTRY_CUBE) {
        pages += ISCANDAR_REGISTRY_PAGES;
    }
    return pages;
}

/* The raise. Returns 0 on success, a bridge status otherwise;
 * on failure nothing is half-raised: the mappings are dropped
 * and the caller reports the status. */
static int layer_raise(Layer *L)
{
    uint32_t c;
    size_t state_pages;

    memset(L, 0, sizeof *L);

    /* The arena: a virtual reservation of the full map size;
     * the substrate faults pages in as grants touch them.
     * In region mode the arena is the kernel's reserved span
     * itself, mapped through the bridge core's device: the
     * layer's ledger and fixtures then stand in the same
     * memory the kernel's offset arithmetic names. */
    if (g_region) {
        g_dev_fd = open("/dev/iscandar", O_RDWR);
        if (g_dev_fd < 0) {
            return ISCANDAR_BRIDGE_REFUSED;
        }
        L->arena = mmap(NULL, ISCANDAR_ARENA_BYTES,
                        PROT_READ | PROT_WRITE, MAP_SHARED,
                        g_dev_fd, ISCANDAR_CHANNEL_BYTES);
        if (L->arena == MAP_FAILED) {
            close(g_dev_fd);
            g_dev_fd = -1;
            return ISCANDAR_BRIDGE_REFUSED;
        }
    } else {
        L->arena = mmap(NULL, ISCANDAR_ARENA_BYTES,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                        -1, 0);
        if (L->arena == MAP_FAILED) {
            return ISCANDAR_BRIDGE_REFUSED;
        }
    }

    /* The layer's own state: the Beam is seated in storage
     * the layer provides, inside the map's reservation for
     * the layer. The transcription allocates nothing itself,
     * so the seat is the caller's choice by construction. */
    state_pages = (sizeof(Beam) + ISCANDAR_PAGE_BYTES - 1)
                  / ISCANDAR_PAGE_BYTES + 1;
    L->state_bytes = state_pages * ISCANDAR_PAGE_BYTES;
    L->state_region = mmap(NULL, L->state_bytes,
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (L->state_region == MAP_FAILED) {
        munmap(L->arena, ISCANDAR_ARENA_BYTES);
        return ISCANDAR_BRIDGE_REFUSED;
    }
    L->beam = (Beam *)L->state_region;
    if (beam_init(L->beam, ISCANDAR_ARENA_CUBES) != K4_SUCCESS) {
        return ISCANDAR_BRIDGE_REFUSED;
    }

    /* The fixtures, marked by hand at the raise: one run at
     * each cube's head, standing in the marks from the first
     * moment. The ledger bin at page 0 of the cube carries
     * the run's length, as for any grant. */
    for (c = 0; c < ISCANDAR_ARENA_CUBES; c++) {
        uint32_t fix = fixture_pages_for(c);
        uint32_t word;
        int32_t st = mpupointer_load(0, fix, 0, MPUPOINTER_MOTOROLA,
                                     &word);
        if (st != SUCCESS) {
            return ISCANDAR_BRIDGE_INVARIANT;
        }
        st = engine_mark(&L->beam->engines[c].engine, word);
        if (st != K4_GEAR_SUCCESS) {
            return ISCANDAR_BRIDGE_INVARIANT;
        }
        L->beam->engines[c].count_overall =
            L->beam->engines[c].engine.count_overall;
        L->beam->count_overall += (int32_t)fix;
        ledger_of(L, c)[0] = (uint16_t)fix;
    }

    /* The registry starts empty (the arena mapping is fresh
     * zeroed storage; stated explicitly so the file-backed
     * case on the bench reads the same). */
    memset(registry_of(L), 0,
           (size_t)ISCANDAR_REGISTRY_PAGES * ISCANDAR_PAGE_BYTES);

    return ISCANDAR_BRIDGE_OK;
}

static void layer_lower(Layer *L)
{
    if (L->arena && L->arena != MAP_FAILED) {
        munmap(L->arena, ISCANDAR_ARENA_BYTES);
    }
    if (L->state_region && L->state_region != MAP_FAILED) {
        munmap(L->state_region, L->state_bytes);
    }
    if (L->ledger_file) {
        fclose(L->ledger_file);
    }
    if (g_dev_fd >= 0) {
        close(g_dev_fd);
        g_dev_fd = -1;
    }
    memset(L, 0, sizeof *L);
}

/* Pages for a byte request, at the calibration. Zero asks for
 * Nothing and is refused at this door (the kernel's own zero
 * case is the kernel's, not a grant). A request past one cube
 * is out of contract: no grant crosses a cube wall. */
static uint32_t pages_for(uint64_t size)
{
    uint64_t pages;
    if (size == 0) {
        return 0;
    }
    pages = (size + ISCANDAR_PAGE_BYTES - 1) / ISCANDAR_PAGE_BYTES;
    if (pages > ISCANDAR_CUBE_PAGES) {
        return 0;
    }
    return (uint32_t)pages;
}

/* Serve one allocation of `pages` pages. On success *offset
 * is the grant's byte offset from the arena base. */
static uint32_t do_alloc(Layer *L, uint32_t pages, uint64_t *offset)
{
    CPUPointer ptr;
    uint32_t start, cube, local;
    int32_t st = beam_allocate(L->beam, (int32_t)pages, &ptr);

    if (st != K4_SUCCESS) {
        return ISCANDAR_BRIDGE_REFUSED;
    }
    start = cpu_pointer_start(&ptr);
    cube = start / ISCANDAR_CUBE_PAGES;
    local = start % ISCANDAR_CUBE_PAGES;
    if (ledger_of(L, cube)[local] != 0) {
        /* The construction says a granted page is never
         * issued again, so this bin was zero. It is not:
         * unwind the grant and report the violation. */
        (void)beam_free(L->beam, &ptr);
        return ISCANDAR_BRIDGE_INVARIANT;
    }
    ledger_of(L, cube)[local] = (uint16_t)pages;
    *offset = (uint64_t)start * ISCANDAR_PAGE_BYTES;
    return ISCANDAR_BRIDGE_OK;
}

/* Serve one free of the block at arena offset `offset`. If
 * cache_id is nonzero it names the cache the block returns
 * to, and its live count is kept. */
static uint32_t do_free(Layer *L, uint64_t offset, uint32_t cache_id,
                        uint32_t *freed_pages)
{
    uint64_t gpage;
    uint32_t cube, local, count;
    BeamCoordinate bc;
    CPUPointer ptr;
    int32_t st;
    RegistryEntry *reg = registry_of(L);

    if (offset % ISCANDAR_PAGE_BYTES != 0) {
        return ISCANDAR_BRIDGE_INVARIANT;
    }
    gpage = offset / ISCANDAR_PAGE_BYTES;
    if (gpage >= ISCANDAR_ARENA_PAGES) {
        return ISCANDAR_BRIDGE_INVARIANT;
    }
    cube = (uint32_t)(gpage / ISCANDAR_CUBE_PAGES);
    local = (uint32_t)(gpage % ISCANDAR_CUBE_PAGES);
    count = ledger_of(L, cube)[local];
    if (count == 0) {
        /* Foreign, interior, or double free. */
        return ISCANDAR_BRIDGE_INVARIANT;
    }
    if (cache_id != 0) {
        if (cache_id > ISCANDAR_REGISTRY_CAP
                || !reg[cache_id - 1].used
                || reg[cache_id - 1].live == 0) {
            return ISCANDAR_BRIDGE_INVARIANT;
        }
    }

    beam_coordinate_init(&bc, (int32_t)cube,
                         (int32_t)(local % 32),
                         (int32_t)((local % 1024) / 32),
                         (int32_t)(local / 1024));
    st = cpu_pointer_create(&bc, count, 0, K4_ORDER_MOTOROLA, &ptr);
    if (st != K4_SUCCESS) {
        return ISCANDAR_BRIDGE_INVARIANT;
    }
    st = beam_free(L->beam, &ptr);
    if (st != K4_GEAR_SUCCESS) {
        /* The books still say live; nothing is papered over. */
        return ISCANDAR_BRIDGE_INVARIANT;
    }
    ledger_of(L, cube)[local] = 0;
    if (cache_id != 0) {
        reg[cache_id - 1].live--;
    }
    *freed_pages = count;
    return ISCANDAR_BRIDGE_OK;
}

/* The dispatch. One record in, one verdict out: the record's
 * status, result, and aux fields are written in place, as the
 * channel loop copies records verbatim in both directions. */
static void layer_serve_one(Layer *L, IscandarBridgeRecord *rec)
{
    uint32_t status = ISCANDAR_BRIDGE_OK;
    uint64_t result = 0;
    uint64_t aux = 0;
    RegistryEntry *reg = registry_of(L);

    switch (rec->op) {
    case ISCANDAR_OP_ALLOC:
    case ISCANDAR_OP_ALLOC_LARGE:
    case ISCANDAR_OP_VMALLOC_BACKING:
    case ISCANDAR_OP_PERCPU_ALLOC:
    case ISCANDAR_OP_GENPOOL_ALLOC: {
        uint32_t pages = pages_for(rec->size);
        if (pages == 0) {
            status = ISCANDAR_BRIDGE_REFUSED;
            break;
        }
        status = do_alloc(L, pages, &result);
        if (status == ISCANDAR_BRIDGE_OK) {
            aux = pages;
        }
        break;
    }

    case ISCANDAR_OP_MEMPOOL_ALLOC:
        if (rec->aux == ISCANDAR_MEMPOOL_BACKING_PAGES) {
            /* A mempool over pages draws on the substrate
             * (reconciliation, Family 4a). Named, not served. */
            status = ISCANDAR_BRIDGE_SUBSTRATE;
            break;
        }
        {
            uint32_t pages = pages_for(rec->size);
            if (pages == 0) {
                status = ISCANDAR_BRIDGE_REFUSED;
                break;
            }
            status = do_alloc(L, pages, &result);
            if (status == ISCANDAR_BRIDGE_OK) {
                aux = pages;
            }
        }
        break;

    case ISCANDAR_OP_FREE:
    case ISCANDAR_OP_VFREE:
    case ISCANDAR_OP_PERCPU_FREE:
    case ISCANDAR_OP_GENPOOL_FREE: {
        uint32_t freed = 0;
        status = do_free(L, rec->size, rec->cache_id, &freed);
        if (status == ISCANDAR_BRIDGE_OK) {
            aux = freed;
        }
        break;
    }

    case ISCANDAR_OP_CACHE_CREATE: {
        uint32_t i;
        status = ISCANDAR_BRIDGE_REFUSED; /* until a slot is found */
        for (i = 0; i < ISCANDAR_REGISTRY_CAP; i++) {
            if (!reg[i].used) {
                reg[i].size = rec->size;
                reg[i].align = rec->align;
                reg[i].flags = (uint32_t)rec->aux;
                reg[i].ctor_cookie = rec->aux;
                reg[i].live = 0;
                reg[i].used = 1;
                reg[i].name_token = rec->cookie;
                rec->cache_id = i + 1; /* ids are 1-based */
                result = i + 1;
                status = ISCANDAR_BRIDGE_OK;
                break;
            }
        }
        break;
    }

    case ISCANDAR_OP_CACHE_ALLOC: {
        uint32_t pages;
        if (rec->cache_id == 0
                || rec->cache_id > ISCANDAR_REGISTRY_CAP
                || !reg[rec->cache_id - 1].used) {
            status = ISCANDAR_BRIDGE_INVARIANT;
            break;
        }
        pages = pages_for(reg[rec->cache_id - 1].size);
        if (pages == 0) {
            status = ISCANDAR_BRIDGE_REFUSED;
            break;
        }
        status = do_alloc(L, pages, &result);
        if (status == ISCANDAR_BRIDGE_OK) {
            reg[rec->cache_id - 1].live++;
            aux = pages;
        }
        break;
    }

    case ISCANDAR_OP_CACHE_DESTROY:
        if (rec->cache_id == 0
                || rec->cache_id > ISCANDAR_REGISTRY_CAP
                || !reg[rec->cache_id - 1].used
                || reg[rec->cache_id - 1].live != 0) {
            /* Unknown cache, or a cache destroyed with objects
             * still live: an invariant violation either way. */
            status = ISCANDAR_BRIDGE_INVARIANT;
            break;
        }
        memset(&reg[rec->cache_id - 1], 0, sizeof(RegistryEntry));
        status = ISCANDAR_BRIDGE_OK;
        break;

    case ISCANDAR_OP_KREALLOC: {
        /* Composition (reconciliation, Family 1d): an
         * allocation, a copy the caller owns, and a free. The
         * layer's part is the two ends. aux carries the old
         * block's arena offset, or UINT64_MAX for none. */
        uint32_t pages;
        if (rec->size == 0) {
            if (rec->aux == UINT64_MAX) {
                status = ISCANDAR_BRIDGE_OK;
                result = 0;
                break;
            }
            {
                uint32_t freed = 0;
                status = do_free(L, rec->aux, 0, &freed);
                result = 0;
            }
            break;
        }
        pages = pages_for(rec->size);
        if (pages == 0) {
            status = ISCANDAR_BRIDGE_REFUSED;
            break;
        }
        status = do_alloc(L, pages, &result);
        if (status != ISCANDAR_BRIDGE_OK) {
            break; /* the old block stands, untouched */
        }
        aux = pages;
        if (rec->aux != UINT64_MAX) {
            uint32_t freed = 0;
            uint32_t fst = do_free(L, rec->aux, 0, &freed);
            if (fst != ISCANDAR_BRIDGE_OK) {
                /* The new grant stands, and the failure to
                 * retire the old block is the verdict. */
                status = fst;
            }
        }
        break;
    }

    case ISCANDAR_OP_NOP:
        status = ISCANDAR_BRIDGE_OK;
        break;

    default:
        status = ISCANDAR_BRIDGE_BADOP;
        break;
    }

    rec->status = status;
    rec->result = result;
    rec->aux = aux;
    L->served++;
    if (status == ISCANDAR_BRIDGE_REFUSED) {
        L->refusals++;
    }
    if (status == ISCANDAR_BRIDGE_INVARIANT) {
        L->invariants++;
    }
    if (L->control) {
        L->control->requests_served = L->served;
        L->control->refusals = L->refusals;
        L->control->invariant_violations = L->invariants;
    }
}

/* ---- Self-test ------------------------------------------------
 * A recorded-style sequence, shaped from the control trace's
 * own figures (its size histogram's shape, its largest
 * request, its cache door's dominance), run in-process. Every
 * expectation is checked; a failed check prints FAIL and the
 * run ends with a nonzero status. The balances at the end
 * are the paper's discipline in miniature: predicted against
 * actual, quantities counted, end state zero beyond the
 * fixtures. */

typedef struct {
    uint64_t offset;
    uint32_t pages;
} LiveBlock;

static int st_checks = 0;
static int st_failures = 0;

static void st_check(int cond, const char *what)
{
    st_checks++;
    if (!cond) {
        st_failures++;
        printf("SELF-TEST FAIL: %s\n", what);
    }
}

static IscandarBridgeRecord st_rec(uint32_t op, uint64_t size,
                                   uint64_t aux, uint32_t cache_id)
{
    IscandarBridgeRecord rec;
    memset(&rec, 0, sizeof rec);
    rec.op = op;
    rec.size = size;
    rec.aux = aux;
    rec.cache_id = cache_id;
    return rec;
}

/* Track a live grant: it must not overlap any other, and it
 * must not start inside its cube's fixture. */
static void st_track(Layer *L, LiveBlock *live, int *nlive,
                     uint64_t offset, uint32_t pages)
{
    uint64_t gpage = offset / ISCANDAR_PAGE_BYTES;
    uint32_t cube = (uint32_t)(gpage / ISCANDAR_CUBE_PAGES);
    uint32_t local = (uint32_t)(gpage % ISCANDAR_CUBE_PAGES);
    int i;

    st_check(local >= fixture_pages_for(cube),
             "grant starts inside a cube fixture");
    for (i = 0; i < *nlive; i++) {
        uint64_t a0 = live[i].offset;
        uint64_t a1 = a0 + (uint64_t)live[i].pages * ISCANDAR_PAGE_BYTES;
        uint64_t b1 = offset + (uint64_t)pages * ISCANDAR_PAGE_BYTES;
        st_check(offset >= a1 || b1 <= a0, "grants overlap");
    }
    /* The grant is real memory: touch its first and last page. */
    L->arena[offset] = (unsigned char)(offset & 0xFF);
    L->arena[offset + (uint64_t)(pages - 1) * ISCANDAR_PAGE_BYTES]
        = (unsigned char)0xA5;
    st_check(L->arena[offset + (uint64_t)(pages - 1)
                        * ISCANDAR_PAGE_BYTES] == 0xA5,
             "granted page not writable");
    live[*nlive].offset = offset;
    live[*nlive].pages = pages;
    (*nlive)++;
}

static void st_untrack(LiveBlock *live, int *nlive, uint64_t offset)
{
    int i;
    for (i = 0; i < *nlive; i++) {
        if (live[i].offset == offset) {
            live[i] = live[*nlive - 1];
            (*nlive)--;
            return;
        }
    }
    st_check(0, "untrack of a block the test does not hold");
}

static uint64_t st_alloc(Layer *L, LiveBlock *live, int *nlive,
                         uint32_t op, uint64_t size, uint32_t cache_id)
{
    IscandarBridgeRecord rec = st_rec(op, size, 0, cache_id);
    layer_serve_one(L, &rec);
    st_check(rec.status == ISCANDAR_BRIDGE_OK, "allocation refused");
    if (rec.status != ISCANDAR_BRIDGE_OK) {
        return UINT64_MAX;
    }
    st_track(L, live, nlive, rec.result, (uint32_t)rec.aux);
    return rec.result;
}

static void st_free(Layer *L, LiveBlock *live, int *nlive,
                    uint64_t offset, uint32_t cache_id)
{
    IscandarBridgeRecord rec = st_rec(ISCANDAR_OP_FREE, offset, 0,
                                      cache_id);
    layer_serve_one(L, &rec);
    st_check(rec.status == ISCANDAR_BRIDGE_OK, "free refused");
    if (rec.status == ISCANDAR_BRIDGE_OK) {
        st_untrack(live, nlive, offset);
    }
}

/* The cross-boundary canary, the layer's half (Task 7).
 * The layer writes ISCANDAR_CANARY_FROM_LAYER at word 0
 * of the fixture page at ISCANDAR_CANARY_OFFSET through
 * its arena mapping, then writes "canary" to
 * /proc/iscandar_bridge; the kernel answers with
 * ISCANDAR_CANARY_FROM_KERNEL at word 1, which the poll
 * waits for. The words land in ledger bins for cube 0
 * pages 2048 and 2049, so both words are restored to 0
 * before the run returns, success or failure: the same
 * scribble-then-restore discipline as the kernel's arena
 * self-check. The two halves are separate functions so
 * the self-test can stand a simulated kernel in the
 * kernel's place. */
static void canary_write(volatile uint32_t *page)
{
    page[1] = 0;
    __sync_synchronize();
    page[0] = ISCANDAR_CANARY_FROM_LAYER;
    __sync_synchronize();
}

static int canary_poll(volatile uint32_t *page, int tries)
{
    struct timespec ts;
    int i;

    ts.tv_sec = 0;
    ts.tv_nsec = 1000000L;
    for (i = 0; i < tries; i++) {
        if (page[1] == ISCANDAR_CANARY_FROM_KERNEL)
            return 0;
        if (page[1] != 0)
            return -1; /* a wrong answer is final */
        nanosleep(&ts, NULL);
    }
    return -1;
}

static int canary_region(void)
{
    int fd, pfd, rc;
    unsigned char *arena;
    volatile uint32_t *page;

    fd = open("/dev/iscandar", O_RDWR);
    if (fd < 0)
        return 1;
    arena = mmap(NULL, ISCANDAR_ARENA_BYTES, PROT_READ | PROT_WRITE,
                 MAP_SHARED, fd, ISCANDAR_CHANNEL_BYTES);
    close(fd);
    if (arena == MAP_FAILED)
        return 1;
    page = (volatile uint32_t *)(void *)(arena
                                         + ISCANDAR_CANARY_OFFSET);
    canary_write(page);
    pfd = open("/proc/iscandar_bridge", O_WRONLY);
    if (pfd < 0) {
        fprintf(stderr, "iscandar-layer: canary: no bridge proc\n");
        page[0] = 0;
        page[1] = 0;
        munmap(arena, ISCANDAR_ARENA_BYTES);
        return 1;
    }
    rc = write(pfd, "canary", 6);
    close(pfd);
    if (rc != 6) {
        fprintf(stderr, "iscandar-layer: canary: kernel refused\n");
        page[0] = 0;
        page[1] = 0;
        munmap(arena, ISCANDAR_ARENA_BYTES);
        return 1;
    }
    rc = canary_poll(page, 10000);
    page[0] = 0;
    page[1] = 0;
    __sync_synchronize();
    munmap(arena, ISCANDAR_ARENA_BYTES);
    if (rc != 0) {
        fprintf(stderr, "iscandar-layer: canary: no answer\n");
        return 1;
    }
    printf("iscandar-layer: canary PASS\n");
    return 0;
}

static int self_test(void)
{
    Layer L;
    LiveBlock live[256];
    int nlive = 0;
    int rc = layer_raise(&L);
    uint32_t c;
    uint64_t big, grow_old, grow_new, vm;
    uint64_t coff[3] = {0, 0, 0};
    uint32_t cache_id;
    IscandarBridgeRecord rec;
    int i;
    static const uint64_t small_sizes[] = {
        64, 72, 80, 184, 256, 1024, 4096, 8192, 16592
    };

    if (rc != ISCANDAR_BRIDGE_OK) {
        printf("SELF-TEST FAIL: raise returned %d\n", rc);
        return 1;
    }
    printf("SELF-TEST raise: arena %u pages in %u cubes, fixtures "
           "%u pages, net served %u pages\n",
           ISCANDAR_ARENA_PAGES, ISCANDAR_ARENA_CUBES,
           ISCANDAR_FIXTURE_PAGES_TOTAL, ISCANDAR_NET_SERVED_PAGES);
    st_check(L.beam->count_overall
                 == (int32_t)ISCANDAR_FIXTURE_PAGES_TOTAL,
             "beam count at raise is not the fixture total");

    /* A named cache, created at the setup door, as the trace's
     * dominant door uses it. */
    rec = st_rec(ISCANDAR_OP_CACHE_CREATE, 184, 0, 0);
    rec.cookie = 0xCACE0001ull;
    layer_serve_one(&L, &rec);
    st_check(rec.status == ISCANDAR_BRIDGE_OK, "cache create refused");
    cache_id = rec.cache_id;
    st_check(cache_id != 0, "cache id is zero");

    /* The small-request shape of the recorded boot, three
     * rounds of its histogram's sizes. */
    for (i = 0; i < 3; i++) {
        size_t j;
        for (j = 0; j < sizeof small_sizes / sizeof small_sizes[0]; j++) {
            st_alloc(&L, live, &nlive, ISCANDAR_OP_ALLOC,
                     small_sizes[j], 0);
        }
    }
    /* The cache door, three objects. Their offsets are kept
     * in the test's hand (coff) so the drain can return them
     * through the cache door and then destroy the cache. */
    for (i = 0; i < 3; i++) {
        coff[i] = st_alloc(&L, live, &nlive, ISCANDAR_OP_CACHE_ALLOC,
                           0, cache_id);
    }
    /* The trace's largest request, through the oversize door. */
    big = st_alloc(&L, live, &nlive, ISCANDAR_OP_ALLOC_LARGE, 996600, 0);
    /* A vmalloc backing: one exact-fit run of 16 pages. */
    vm = st_alloc(&L, live, &nlive, ISCANDAR_OP_VMALLOC_BACKING,
                  16 * ISCANDAR_PAGE_BYTES, 0);
    /* Second ring: percpu, gen_pool, mempool over slab. */
    st_alloc(&L, live, &nlive, ISCANDAR_OP_PERCPU_ALLOC, 8192, 0);
    st_alloc(&L, live, &nlive, ISCANDAR_OP_GENPOOL_ALLOC, 4096, 0);
    rec = st_rec(ISCANDAR_OP_MEMPOOL_ALLOC, 2048,
                 ISCANDAR_MEMPOOL_BACKING_SLAB, 0);
    layer_serve_one(&L, &rec);
    st_check(rec.status == ISCANDAR_BRIDGE_OK, "mempool slab refused");
    if (rec.status == ISCANDAR_BRIDGE_OK) {
        st_track(&L, live, &nlive, rec.result, (uint32_t)rec.aux);
    }
    /* A mempool over pages is the substrate's, by ruling. */
    rec = st_rec(ISCANDAR_OP_MEMPOOL_ALLOC, 2048,
                 ISCANDAR_MEMPOOL_BACKING_PAGES, 0);
    layer_serve_one(&L, &rec);
    st_check(rec.status == ISCANDAR_BRIDGE_SUBSTRATE,
             "mempool over pages not answered SUBSTRATE");

    /* krealloc by composition: grow an 8,192-byte block to
     * 20,000; the old offset must retire. */
    grow_old = st_alloc(&L, live, &nlive, ISCANDAR_OP_ALLOC, 8192, 0);
    rec = st_rec(ISCANDAR_OP_KREALLOC, 20000, grow_old, 0);
    layer_serve_one(&L, &rec);
    st_check(rec.status == ISCANDAR_BRIDGE_OK, "krealloc refused");
    grow_new = rec.result;
    if (rec.status == ISCANDAR_BRIDGE_OK) {
        st_untrack(live, &nlive, grow_old);
        st_track(&L, live, &nlive, grow_new, (uint32_t)rec.aux);
    }

    /* Invariant probes: each must be answered with a status,
     * and the layer must serve on afterwards. */
    rec = st_rec(ISCANDAR_OP_FREE, big, 0, 0);
    layer_serve_one(&L, &rec);
    st_check(rec.status == ISCANDAR_BRIDGE_OK, "free of large refused");
    if (rec.status == ISCANDAR_BRIDGE_OK) {
        st_untrack(live, &nlive, big);
    }
    rec = st_rec(ISCANDAR_OP_FREE, big, 0, 0); /* the double free */
    layer_serve_one(&L, &rec);
    st_check(rec.status == ISCANDAR_BRIDGE_INVARIANT,
             "double free not answered INVARIANT");
    rec = st_rec(ISCANDAR_OP_FREE, vm + ISCANDAR_PAGE_BYTES, 0, 0);
    layer_serve_one(&L, &rec);
    st_check(rec.status == ISCANDAR_BRIDGE_INVARIANT,
             "interior free not answered INVARIANT");
    rec = st_rec(ISCANDAR_OP_FREE,
                 (uint64_t)(ISCANDAR_ARENA_PAGES - 1)
                     * ISCANDAR_PAGE_BYTES, 0, 0);
    layer_serve_one(&L, &rec);
    st_check(rec.status == ISCANDAR_BRIDGE_INVARIANT,
             "foreign free not answered INVARIANT");
    rec = st_rec(ISCANDAR_OP_CACHE_DESTROY, 0, 0, cache_id);
    layer_serve_one(&L, &rec);
    st_check(rec.status == ISCANDAR_BRIDGE_INVARIANT,
             "destroy with live objects not answered INVARIANT");
    st_alloc(&L, live, &nlive, ISCANDAR_OP_ALLOC, 4096, 0);

    /* Drain: the cache objects go home through the cache
     * door, the cache is destroyed, and everything else the
     * test still holds goes back through the plain door. */
    for (i = 0; i < 3; i++) {
        if (coff[i] != 0) {
            st_free(&L, live, &nlive, coff[i], cache_id);
        }
    }
    rec = st_rec(ISCANDAR_OP_CACHE_DESTROY, 0, 0, cache_id);
    layer_serve_one(&L, &rec);
    st_check(rec.status == ISCANDAR_BRIDGE_OK,
             "cache destroy after drain refused");
    {
        RegistryEntry *reg = registry_of(&L);
        uint32_t ri;
        int any_used = 0;
        for (ri = 0; ri < ISCANDAR_REGISTRY_CAP; ri++) {
            if (reg[ri].used) {
                any_used = 1;
            }
        }
        st_check(!any_used, "registry not empty at end of test");
    }
    while (nlive > 0) {
        uint64_t off = live[nlive - 1].offset;
        IscandarBridgeRecord fr = st_rec(ISCANDAR_OP_FREE, off, 0, 0);
        layer_serve_one(&L, &fr);
        if (fr.status != ISCANDAR_BRIDGE_OK) {
            break;
        }
        nlive--;
    }
    st_check(nlive == 0, "drain incomplete");

    /* The canary, in simulation (Task 7): a responder that
     * answers as the kernel would passes; one that never
     * answers fails on the poll; one that answers wrongly
     * fails at once. Both words are restored to 0 inside
     * the block, so the ledger is as the drains left
     * it. */
    {
        volatile uint32_t *page = (volatile uint32_t *)(void *)
            (L.arena + ISCANDAR_CANARY_OFFSET);
        canary_write(page);
        st_check(page[0] == ISCANDAR_CANARY_FROM_LAYER,
                 "canary pattern not written");
        st_check(page[1] == 0, "canary answer not cleared");
        page[1] = ISCANDAR_CANARY_FROM_KERNEL;
        st_check(canary_poll(page, 100) == 0,
                 "canary pass not seen");
        canary_write(page);
        st_check(canary_poll(page, 5) != 0,
                 "canary silence not refused");
        canary_write(page);
        page[1] = 0xDEADBEEFu;
        st_check(canary_poll(page, 100) != 0,
                 "canary wrong answer not refused");
        page[0] = 0;
        page[1] = 0;
    }

    /* The balances. */
    st_check(L.beam->count_overall
                 == (int32_t)ISCANDAR_FIXTURE_PAGES_TOTAL,
             "end state is not the fixture total");
    for (c = 0; c < ISCANDAR_ARENA_CUBES; c++) {
        uint32_t p;
        uint32_t nonzero = 0;
        st_check(L.beam->engines[c].count_overall
                     == fixture_pages_for(c),
                 "per-cube end count is not its fixture");
        for (p = 0; p < ISCANDAR_CUBE_PAGES; p++) {
            if (ledger_of(&L, c)[p] != 0) {
                nonzero++;
                st_check(p == 0
                             && ledger_of(&L, c)[p]
                                    == fixture_pages_for(c),
                         "ledger bin survives outside the fixture");
            }
        }
        st_check(nonzero == 1, "ledger not back to the fixture bin only");
    }

    printf("SELF-TEST: %d checks, %d failures; served %llu, "
           "refusals %llu, invariants %llu\n",
           st_checks, st_failures,
           (unsigned long long)L.served,
           (unsigned long long)L.refusals,
           (unsigned long long)L.invariants);
    if (st_failures == 0 && L.invariants == 4) {
        printf("SELF-TEST PASS\n");
        layer_lower(&L);
        return 0;
    }
    printf("SELF-TEST FAIL\n");
    layer_lower(&L);
    return 1;
}

/* ---- Channel serving ------------------------------------------
 * The channel is a shared file of ISCANDAR_CHANNEL_BYTES:
 * the control page, then the request ring, then the response
 * ring, then the prefix log, at the page offsets the header
 * fixes. The kernel side is the patch's business; this loop
 * is the layer's half, and it doubles as the bench harness
 * the link-check drives a record through by hand. */

static int serve_loop(Layer *L, unsigned char *chan, const char *label)
{
    IscandarBridgeRecord *req_ring, *resp_ring;

    req_ring = (IscandarBridgeRecord *)(void *)(chan
                   + (size_t)ISCANDAR_CHANNEL_CONTROL_PAGES
                     * ISCANDAR_PAGE_BYTES);
    resp_ring = (IscandarBridgeRecord *)(void *)(chan
                   + (size_t)(ISCANDAR_CHANNEL_CONTROL_PAGES
                              + ISCANDAR_CHANNEL_REQ_PAGES)
                     * ISCANDAR_PAGE_BYTES);

    log_line(L, "layer serving: arena base %p, %u cubes, channel %s\n",
             (void *)L->arena, ISCANDAR_ARENA_CUBES, label);

    while (!L->control->stop) {
        if (L->control->req_head != L->control->req_tail) {
            IscandarBridgeRecord rec =
                req_ring[L->control->req_head % ISCANDAR_RING_RECORDS];
            __sync_synchronize();
            layer_serve_one(L, &rec);
            resp_ring[L->control->resp_tail % ISCANDAR_RING_RECORDS]
                = rec;
            __sync_synchronize();
            L->control->resp_tail++;
            L->control->req_head++;
        } else {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 1000000L;
            nanosleep(&ts, NULL);
        }
    }

    log_line(L, "layer stopping: served %llu, refusals %llu, "
                "invariants %llu, beam live %d pages\n",
             (unsigned long long)L->served,
             (unsigned long long)L->refusals,
             (unsigned long long)L->invariants,
             (int)L->beam->count_overall);
    return ISCANDAR_BRIDGE_OK;
}

static int serve_channel(Layer *L, const char *path)
{
    int fd, rc;
    unsigned char *chan;

    fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        return ISCANDAR_BRIDGE_REFUSED;
    }
    if (ftruncate(fd, ISCANDAR_CHANNEL_BYTES) != 0) {
        close(fd);
        return ISCANDAR_BRIDGE_REFUSED;
    }
    chan = mmap(NULL, ISCANDAR_CHANNEL_BYTES, PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0);
    close(fd);
    if (chan == MAP_FAILED) {
        return ISCANDAR_BRIDGE_REFUSED;
    }

    memset(chan, 0, ISCANDAR_CHANNEL_BYTES);
    L->control = (IscandarChannelControl *)(void *)chan;
    L->control->magic = ISCANDAR_CHANNEL_MAGIC;
    L->control->version = ISCANDAR_CHANNEL_VERSION;
    L->control->arena_base = (uint64_t)(uintptr_t)L->arena;
    L->control->arena_cubes = ISCANDAR_ARENA_CUBES;
    L->control->channel_pages = ISCANDAR_CHANNEL_PAGES;

    rc = serve_loop(L, chan, path);
    munmap(chan, ISCANDAR_CHANNEL_BYTES);
    L->control = NULL;
    return rc;
}

/* On the way down in region mode, the layer writes the channel
 * snapshot itself: it is the one process holding the region.
 * The finish step harvests the file. */
static void snapshot_channel(unsigned char *chan)
{
    FILE *f = fopen("/var/log/iscandar/channel-snapshot.bin", "wb");

    if (!f) {
        f = fopen("channel-snapshot.bin", "wb");
    }
    if (!f) {
        return;
    }
    fwrite(chan, 1, ISCANDAR_CHANNEL_BYTES, f);
    fclose(f);
}

static int serve_region(Layer *L)
{
    int fd, rc;
    unsigned char *chan;

    fd = open("/dev/iscandar", O_RDWR);
    if (fd < 0) {
        return ISCANDAR_BRIDGE_REFUSED;
    }
    chan = mmap(NULL, ISCANDAR_CHANNEL_BYTES, PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0);
    close(fd);
    if (chan == MAP_FAILED) {
        return ISCANDAR_BRIDGE_REFUSED;
    }

    /* Attach discipline (Task 6, Decision 4): no memset of the
     * channel. The kernel's prefix writing stands in the
     * control block's prefix fields and in the prefix log
     * pages; the layer writes only the fields it owns, and
     * zeroes the rings, which are empty by construction
     * before the raise (bridge_call refuses until ready). */
    L->control = (IscandarChannelControl *)(void *)chan;
    memset(chan + (size_t)ISCANDAR_CHANNEL_CONTROL_PAGES
             * ISCANDAR_PAGE_BYTES, 0,
           (size_t)(ISCANDAR_CHANNEL_REQ_PAGES
                    + ISCANDAR_CHANNEL_RESP_PAGES)
             * ISCANDAR_PAGE_BYTES);
    L->control->magic = ISCANDAR_CHANNEL_MAGIC;
    L->control->version = ISCANDAR_CHANNEL_VERSION;
    L->control->arena_base = (uint64_t)(uintptr_t)L->arena;
    L->control->arena_cubes = ISCANDAR_ARENA_CUBES;
    L->control->channel_pages = ISCANDAR_CHANNEL_PAGES;
    L->control->req_head = 0;
    L->control->req_tail = 0;
    L->control->resp_head = 0;
    L->control->resp_tail = 0;
    L->control->requests_served = 0;
    L->control->refusals = 0;
    L->control->invariant_violations = 0;
    L->control->stop = 0;
    __sync_synchronize();

    rc = serve_loop(L, chan, "/dev/iscandar channel region");
    snapshot_channel(chan);
    munmap(chan, ISCANDAR_CHANNEL_BYTES);
    L->control = NULL;
    return rc;
}

int main(int argc, char **argv)
{
    Layer L;
    int rc;

    if (argc >= 2 && strcmp(argv[1], "--self-test") == 0) {
        return self_test();
    }
    if (argc >= 2 && strcmp(argv[1], "--canary") == 0) {
        return canary_region();
    }
    if (argc >= 3 && strcmp(argv[1], "--serve") == 0) {
        rc = layer_raise(&L);
        if (rc != ISCANDAR_BRIDGE_OK) {
            fprintf(stderr, "iscandar-layer: raise failed, status %d\n",
                    rc);
            return 1;
        }
        L.ledger_file = fopen("/var/log/iscandar/layer-ledger.txt", "a");
        if (!L.ledger_file) {
            L.ledger_file = fopen("iscandar-layer-ledger.txt", "a");
        }
        log_line(&L, "iscandar layer raised: %u pages, fixtures %u, "
                     "net served %u\n",
                 ISCANDAR_ARENA_PAGES, ISCANDAR_FIXTURE_PAGES_TOTAL,
                 ISCANDAR_NET_SERVED_PAGES);
        rc = serve_channel(&L, argv[2]);
        layer_lower(&L);
        return rc == ISCANDAR_BRIDGE_OK ? 0 : 1;
    }
    if (argc >= 2 && strcmp(argv[1], "--serve-region") == 0) {
        if (cmdline_phys("iscandar_channel", &g_channel_phys) != 0
            || cmdline_phys("iscandar_arena", &g_arena_phys) != 0
            || !g_channel_phys || !g_arena_phys) {
            fprintf(stderr, "iscandar-layer: --serve-region needs "
                    "iscandar_channel= and iscandar_arena= on the "
                    "kernel command line\n");
            return 1;
        }
        g_region = 1;
        rc = layer_raise(&L);
        if (rc != ISCANDAR_BRIDGE_OK) {
            fprintf(stderr, "iscandar-layer: raise failed, status %d\n",
                    rc);
            return 1;
        }
        L.ledger_file = fopen("/var/log/iscandar/layer-ledger.txt", "a");
        if (!L.ledger_file) {
            L.ledger_file = fopen("iscandar-layer-ledger.txt", "a");
        }
        log_line(&L, "iscandar layer raised: %u pages, fixtures %u, "
                     "net served %u\n",
                 ISCANDAR_ARENA_PAGES, ISCANDAR_FIXTURE_PAGES_TOTAL,
                 ISCANDAR_NET_SERVED_PAGES);
        rc = serve_region(&L);
        layer_lower(&L);
        return rc == ISCANDAR_BRIDGE_OK ? 0 : 1;
    }
    fprintf(stderr, "usage: %s --self-test | --canary "
                    "| --serve CHANNEL-FILE "
                    "| --serve-region\n", argv[0]);
    return 2;
}
