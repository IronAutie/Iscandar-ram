/* CubeEngine.c -- the per-cube engine ADAPTER. See
 * CubeEngine.h for the boundary this file stands behind.
 *
 * Each CubeEngine holds one real k=3 C Engine. Allocation is
 * engine_call; the free path rebuilds the MPUPointer word
 * from the (start, count) Beam.c hands down (the word
 * MPUPointer.load_coordinate would mint for that block in
 * Beam.py's free, reduced to its fields) and calls
 * engine_unmark with it.
 *
 * Status mapping: the k=3 C status.h and K4Status.h both
 * transcribe the one canonical Status.py, so a status crosses
 * this boundary by value, unchanged. The statuses the free
 * path can produce are exactly the K4 subset (GEAR_SUCCESS,
 * FAIL_OVERLAY, FAIL); the named cases below pin that
 * correspondence, and any other value (a mode's own failure
 * on the allocate path, where Beam observes only that no
 * pointer came back) passes through as the very int the
 * Python Engine would have returned. */

#include "CubeEngine.h"
#include "K4Status.h"

#include "MPUPointer.h"
#include "status.h"

static int32_t cube_engine_map_status(int32_t status)
{
    switch (status) {
    case GEAR_SUCCESS:
        return K4_GEAR_SUCCESS;
    case FAIL_OVERLAY:
        return K4_FAIL_OVERLAY;
    case FAIL:
        return K4_FAIL;
    default:
        return status;
    }
}

void cube_engine_init(CubeEngine *engine)
{
    engine_init(&engine->engine);
    engine->count_overall = 0;
}

void cube_engine_call(CubeEngine *engine, int32_t request,
                      CubeEngineResult *out)
{
    EngineResult result;

    /* A negative request is out of every mode's table: the
     * Python Engine returns None via the Nothing fallback,
     * with no search and no gear interaction. The C
     * engine_call takes uint32_t and cannot express the
     * case (see Engine.h), so the adapter answers with the
     * same shape directly. */
    if (request < 0) {
        out->kind = K4_ENGINE_NONE;
        return;
    }

    engine_call(&engine->engine, (uint32_t)request, &result);
    engine->count_overall = engine->engine.count_overall;

    switch (result.outcome) {
    case ENGINE_OUTCOME_POINTER:
        out->kind = K4_ENGINE_POINTER;
        out->start = mpupointer_start(result.word);
        out->count = mpupointer_count(result.word);
        break;
    case ENGINE_OUTCOME_NONE:
        out->kind = K4_ENGINE_NONE;
        break;
    default: /* ENGINE_OUTCOME_STATUS */
        out->kind = K4_ENGINE_STATUS;
        out->status = cube_engine_map_status(result.status);
        break;
    }
}

int32_t cube_engine_unmark(CubeEngine *engine, uint32_t start,
                           uint32_t count)
{
    uint32_t word;
    int32_t status;

    /* Rebuild the pointer word for this block, born virtual
     * as load_coordinate mints it. A refusal here (a count
     * of 0, or a start past the last name -- neither can
     * arise from a CPUPointer Beam.c holds) is the boundary
     * refusal FAIL, the shape Engine.unmark's own boundary
     * guard returns. */
    status = mpupointer_load(start, count, 0, MPUPOINTER_MOTOROLA,
                             &word);
    if (status != SUCCESS) {
        return K4_FAIL;
    }

    status = engine_unmark(&engine->engine, word);
    engine->count_overall = engine->engine.count_overall;
    return cube_engine_map_status(status);
}
