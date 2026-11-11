/* Engine.c -- see Engine.h. Transcription of
 * allocator.py; the Python is the specification. */

#include "Engine.h"

#include <string.h>

#include "Coordinate.h"
#include "Gears.h"
#include "Modes.h"
#include "MPUPointer.h"
#include "status.h"

/* The Engine's own verification gates for the Large/Huge
 * and Small modes, defined below engine_call. */
static int large_huge_span_fits(const Engine *engine,
                                uint32_t plane, uint32_t row,
                                uint32_t sector, uint32_t request);
static int small_span_fits(const Engine *engine,
                           uint32_t plane, uint32_t row,
                           uint32_t request);

void engine_init(Engine *engine)
{
    cube_init(&engine->cube);
    engine->count_overall = 0;
    memset(engine->count_planes, 0, sizeof engine->count_planes);
    memset(engine->count_rows, 0, sizeof engine->count_rows);
}

int32_t engine_mark(Engine *engine, uint32_t word)
{
    uint32_t start = mpupointer_start(word);
    uint32_t count = mpupointer_count(word);
    int32_t  status;

    if (start + count > CUBE_SECTORS_PER_CUBE) {
        return FAIL;
    }
    status = mark_sectors(engine->cube.words,
                          engine->count_rows,
                          engine->count_planes,
                          start, count);
    if (status == GEAR_SUCCESS) {
        engine->count_overall += count;
    }
    return status;
}

int32_t engine_unmark(Engine *engine, uint32_t word)
{
    uint32_t start = mpupointer_start(word);
    uint32_t count = mpupointer_count(word);
    int32_t  status;

    if (start + count > CUBE_SECTORS_PER_CUBE) {
        return FAIL;
    }
    status = unmark_sectors(engine->cube.words,
                            engine->count_rows,
                            engine->count_planes,
                            start, count);
    if (status == GEAR_SUCCESS) {
        engine->count_overall -= count;
    }
    return status;
}

void engine_call(Engine *engine, uint32_t request,
                 EngineResult *result)
{
    const Mode *mode;
    ModeCtx     ctx;
    uint32_t    plane;
    uint32_t    row;
    uint32_t    sector;
    int32_t     status;
    Coordinate  coordinate;
    uint32_t    word;
    int         fits;

    /* Step 1: the admission gate. A request larger than
     * what remains fails outright, with no mode dispatch
     * and no sweep. */
    if (request > CUBE_SECTORS_PER_CUBE - engine->count_overall) {
        result->outcome = ENGINE_OUTCOME_STATUS;
        result->status  = FAIL;
        return;
    }

    /* Step 2: the dispatch, with the Nothing fallback for
     * out-of-table requests. Nothing's search returns None
     * in the Python and the Engine returns it directly, so
     * the Nothing outcome is recognized here by identity,
     * before any search runs. */
    mode = modes_get(request);
    if (mode == modes_nothing()) {
        result->outcome = ENGINE_OUTCOME_NONE;
        return;
    }

    /* Step 3: the mode's search. */
    ctx.count_planes  = engine->count_planes;
    ctx.count_rows    = engine->count_rows;
    ctx.words         = engine->cube.words;
    ctx.count_overall = (int64_t)engine->count_overall;
    status = mode->search(&ctx, request, &plane, &row, &sector);

    /* Step 4: the search's own failure is the failure
     * returned. */
    if (status != GEAR_SUCCESS) {
        result->outcome = ENGINE_OUTCOME_STATUS;
        result->status  = status;
        return;
    }

    /* Post-search verification, spoken for each mode as the
     * Python speaks it (isinstance order: CubeCheck, then
     * Large or Huge, then Small, then the rest). The modes
     * are identified by the table entries a key in their
     * range returns, which is the same partition isinstance
     * makes. */
    if (mode == modes_get(CUBE_SECTORS_PER_CUBE)) {
        /* CubeCheck: the certification is the overall count
         * itself -- zero, and the request the whole cube. */
        fits = request == CUBE_SECTORS_PER_CUBE
            && engine->count_overall == 0;
    } else if (mode == modes_get(65) || mode == modes_get(1025)) {
        /* Large and Huge: the span must fit the cube and
         * the spanned planes' free total must reach the
         * request. */
        fits = large_huge_span_fits(engine, plane, row, sector,
                                    request);
    } else if (mode == modes_get(33)) {
        /* Small: the anchor row's free plus its ancillary's
         * free together must reach the request; the
         * ancillary row must exist. */
        fits = small_span_fits(engine, plane, row, request);
    } else {
        /* The rest: the found plane's free and the found
         * row's free must each reach the request. */
        fits = (PLANE_SECTORS_PER_PLANE
                    - engine->count_planes[plane] >= request)
            && (ROW_SECTORS_PER_ROW
                    - engine->count_rows[
                          plane * ROW_SECTORS_PER_ROW + row]
                    >= request);
    }
    if (!fits) {
        result->outcome = ENGINE_OUTCOME_STATUS;
        result->status  = FAIL;
        return;
    }

    /* Step 5: the Coordinate, fields kept separate -- the
     * sector is relative to the row found, never folded. */
    coordinate_init(&coordinate, sector, row, plane);

    /* Step 6: construct the pointer from the Coordinate,
     * born virtual, then mark first; a failed mark
     * propagates its status and no pointer is returned.
     * load_coordinate cannot refuse a coordinate a
     * successful search produced (its start is in range and
     * the count is the request), but its status is honored
     * if it ever did. */
    status = mpupointer_load_coordinate(&coordinate, request, 0,
                                        MPUPOINTER_MOTOROLA,
                                        &word);
    if (status != SUCCESS) {
        result->outcome = ENGINE_OUTCOME_STATUS;
        result->status  = status;
        return;
    }
    status = engine_mark(engine, word);
    if (status != GEAR_SUCCESS) {
        result->outcome = ENGINE_OUTCOME_STATUS;
        result->status  = status;
        return;
    }

    /* Step 7: return the pointer. */
    result->outcome = ENGINE_OUTCOME_POINTER;
    result->word    = word;
}

void engine_deallocate(Engine *engine, uint32_t word)
{
    (void)engine;
    (void)word;
}

/* The Large/Huge verification gate: the Engine's, not a
 * mode's search step (allocator.py, the Large/Huge branch
 * of the post-search verification). first_plane is the
 * span's start plane by construction, so the loop runs
 * from the found plane. */
static int large_huge_span_fits(const Engine *engine,
                                uint32_t plane, uint32_t row,
                                uint32_t sector, uint32_t request)
{
    uint32_t start_global;
    uint32_t end_global;
    uint32_t last_plane;
    uint32_t free_spanned = 0;
    uint32_t pl;

    start_global = plane * PLANE_SECTORS_PER_PLANE
                 + row * ROW_SECTORS_PER_ROW + sector;
    end_global = start_global + request;
    if (end_global > CUBE_SECTORS_PER_CUBE) {
        return 0;
    }
    last_plane = (end_global - 1) / PLANE_SECTORS_PER_PLANE;
    for (pl = plane; pl <= last_plane; pl++) {
        free_spanned += PLANE_SECTORS_PER_PLANE
                        - engine->count_planes[pl];
    }
    return free_spanned >= request;
}

/* The Small verification gate, likewise the Engine's: the
 * ancillary row (anchor + 1) must exist, and the pair's
 * free sum must reach the request. */
static int small_span_fits(const Engine *engine,
                           uint32_t plane, uint32_t row,
                           uint32_t request)
{
    uint32_t anchor;
    uint32_t ancillary;

    anchor    = plane * ROW_SECTORS_PER_ROW + row;
    ancillary = anchor + 1;
    if (ancillary > CUBE_ROWS_PER_CUBE - 1) {
        return 0;
    }
    return (ROW_SECTORS_PER_ROW - engine->count_rows[anchor])
         + (ROW_SECTORS_PER_ROW - engine->count_rows[ancillary])
         >= request;
}
