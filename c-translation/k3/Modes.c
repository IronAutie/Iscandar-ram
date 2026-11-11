/* Modes.c -- see Modes.h. Transcription of Modes.py (and
 * the engine-reachable CubeCheck search from
 * CubeCheck.py); the Python is the specification. */

#include "Modes.h"

#include "Cube.h"
#include "Geometry.h"
#include "Gears.h"
#include "Huge.h"
#include "Large.h"
#include "Nano.h"
#include "Nothing.h"
#include "Plane.h"
#include "Small.h"
#include "Tiny.h"
#include "status.h"

/* --- searches -------------------------------------------------
 * Each search below has the ModeSearchFn shape (Gears.h)
 * and delegates to the transcribed class search of its
 * mode, exactly as each Python class's search delegates;
 * the CubeCheck search is the one exception, being short
 * enough to stand here in full. Counters the Python
 * reaches through self (count_planes, count_overall) are
 * carried on the ModeCtx.
 */

/* Nothing.search returns None in the Python; here it is
 * the identity sentinel: it serves nothing, and the
 * Engine's comparison against modes_nothing() is what
 * recognizes it. Its search is never invoked with effect;
 * it returns GEAR_SUCCESS and zero coordinates, purely so
 * the function pointer is valid. */
static int32_t table_nothing_search(const ModeCtx *ctx,
                              uint32_t request,
                              uint32_t *plane_out,
                              uint32_t *row_out,
                              uint32_t *sector_out)
{
    (void)ctx;
    (void)request;
    *plane_out  = 0;
    *row_out    = 0;
    *sector_out = 0;
    return GEAR_SUCCESS;
}

static int32_t table_nano_search(const ModeCtx *ctx, uint32_t request,
                           uint32_t *plane_out, uint32_t *row_out,
                           uint32_t *sector_out)
{
    /* Nano.search is the base Mode pattern with Nano's
     * bullet (Mode.search, inherited unchanged). */
    return mode_base_search(ctx, request, nano_bullet,
                            plane_out, row_out, sector_out);
}

static int32_t table_tiny_search(const ModeCtx *ctx, uint32_t request,
                           uint32_t *plane_out, uint32_t *row_out,
                           uint32_t *sector_out)
{
    /* Tiny.search is likewise the base Mode pattern. */
    return mode_base_search(ctx, request, tiny_bullet,
                            plane_out, row_out, sector_out);
}

static int32_t table_small_search(const ModeCtx *ctx, uint32_t request,
                            uint32_t *plane_out, uint32_t *row_out,
                            uint32_t *sector_out)
{
    return small_search(request, ctx->count_planes,
                        ctx->count_rows, ctx->words,
                        plane_out, row_out, sector_out);
}

static int32_t table_large_search(const ModeCtx *ctx, uint32_t request,
                            uint32_t *plane_out, uint32_t *row_out,
                            uint32_t *sector_out)
{
    return large_search(request, ctx->count_planes,
                        ctx->count_rows, ctx->words,
                        plane_out, row_out, sector_out);
}

static int32_t table_huge_search(const ModeCtx *ctx,
                                uint32_t request,
                                uint32_t *plane_out,
                                uint32_t *row_out,
                                uint32_t *sector_out)
{
    return huge_search(request, ctx->count_planes,
                       ctx->count_rows, ctx->words,
                       plane_out, row_out, sector_out);
}

/* CubeCheck.search: the cube answers a whole-cube request
 * only when every sector is free. count_overall falls
 * back to the sum of the plane counts when the caller did
 * not pass it, as the Python does (in the Engine it is
 * always passed). */
static int32_t table_cubecheck_search(const ModeCtx *ctx,
                                uint32_t request,
                                uint32_t *plane_out,
                                uint32_t *row_out,
                                uint32_t *sector_out)
{
    uint32_t overall;
    uint32_t i;

    *plane_out  = 0;
    *row_out    = 0;
    *sector_out = 0;
    if (request != CUBE_SECTORS_PER_CUBE) {
        return GEAR_EXHAUST;
    }
    if (ctx->count_overall >= 0) {
        overall = (uint32_t)ctx->count_overall;
    } else {
        overall = 0;
        for (i = 0; i < CUBE_PLANES_PER_CUBE; i++) {
            overall += ctx->count_planes[i];
        }
    }
    if (overall != 0) {
        return GEAR_EXHAUST;
    }
    return GEAR_SUCCESS;
}

/* --- the modes ------------------------------------------------
 * Ranges half-open, built from the class bounds
 * (Modes.tuple in the Python), in the Python's
 * construction order: Nothing, Nano, Tiny, Small, Large,
 * Huge, CubeCheck.
 */

/* Half-open bounds, from each class's range() in the
 * Python (inclusive bounds there, +1 here): Nothing {0},
 * Nano 1..8, Tiny 9..32, Small 33..64, Large 65..1024,
 * Huge 1025..32767, CubeCheck {32768}. */
static const Mode nothing_mode = { 0, 1, table_nothing_search };
static const Mode nano_mode = { 1, 9, table_nano_search };
static const Mode tiny_mode = { 9, 33, table_tiny_search };
static const Mode small_mode = { 33, 65, table_small_search };
static const Mode large_mode = { 65, 1025, table_large_search };
static const Mode huge_mode = { 1025, 32768, table_huge_search };
static const Mode cubecheck_mode = {
    CUBE_SECTORS_PER_CUBE, CUBE_SECTORS_PER_CUBE + 1,
    table_cubecheck_search
};

uint32_t modes_count(void)
{
    return CUBE_SECTORS_PER_CUBE + 1;
}

int modes_has_key(uint32_t key)
{
    return key <= CUBE_SECTORS_PER_CUBE;
}

const Mode *modes_get(uint32_t key)
{
    if (key == 0) {
        return &nothing_mode;
    }
    if (key < nano_mode.hi && key >= nano_mode.lo) {
        return &nano_mode;
    }
    if (key < tiny_mode.hi && key >= tiny_mode.lo) {
        return &tiny_mode;
    }
    if (key < small_mode.hi && key >= small_mode.lo) {
        return &small_mode;
    }
    if (key < large_mode.hi && key >= large_mode.lo) {
        return &large_mode;
    }
    if (key < huge_mode.hi && key >= huge_mode.lo) {
        return &huge_mode;
    }
    if (key == CUBE_SECTORS_PER_CUBE) {
        return &cubecheck_mode;
    }
    return &nothing_mode;
}

const Mode *modes_nothing(void)
{
    return &nothing_mode;
}
