/* Engine.h -- transcription target of allocator.py.
 *
 * Engine is the allocator: a Cube (the words store, Cube.h)
 * plus the three counters Engine.__init__ creates
 * (count_overall, count_planes, count_rows) and the mode
 * table. Engine.__call__ dispatches a request through the
 * table (Modes.h) to the request mode that serves its
 * size, verifies the found placement against the counters,
 * mints an MPUPointer on the coordinates, marks the sectors
 * through the C mark path (Mark.c/Unmark.c over Commit.c --
 * the same entry the Python reaches through Gears.py), and
 * returns the pointer word. Engine.mark and Engine.unmark
 * move sectors directly under a pointer word's start and
 * count, keeping count_overall in step here, exactly as
 * the Python keeps it (the C marker owns the sectors and
 * the row and plane counters; the overall counter is the
 * caller's, per Mark.c's settled contract).
 *
 * Return-shape note: the Python's __call__ returns either
 * a status int (a failure), None (the Nothing outcome for
 * request 0 and for out-of-table requests), or an
 * MPUPointer. engine_call keeps those three outcomes
 * distinct: ENGINE_OUTCOME_STATUS, ENGINE_OUTCOME_NONE,
 * ENGINE_OUTCOME_POINTER, with the status or the pointer
 * word in the caller's EngineResult. Negative Python
 * requests (which fall through to the Nothing fallback and
 * return None) have no representation here: engine_call
 * takes uint32_t, so that case cannot be expressed -- see
 * the tranche report.
 */

#ifndef ISCANDAR_ENGINE_H
#define ISCANDAR_ENGINE_H

#include <stdint.h>

#include "Cube.h"

typedef struct {
    Cube     cube;                              /* the store */
    uint32_t count_overall;
    uint32_t count_planes[CUBE_PLANES_PER_CUBE];
    uint32_t count_rows[CUBE_ROWS_PER_CUBE];
} Engine;

enum {
    ENGINE_OUTCOME_STATUS  = 0,
    ENGINE_OUTCOME_NONE    = 1,
    ENGINE_OUTCOME_POINTER = 2
};

typedef struct {
    int32_t  outcome; /* one of the ENGINE_OUTCOME_* */
    int32_t  status;  /* when outcome is STATUS */
    uint32_t word;    /* the pointer word, when POINTER */
} EngineResult;

/* engine_init: Engine.__init__ -- a fresh Cube store and
 * zeroed counters. */
void engine_init(Engine *engine);

/* engine_mark / engine_unmark: Engine.mark / Engine.unmark.
 * The pointer is a word; start and count are its fields
 * (Motorola order, the Python properties' default). FAIL
 * when start + count would run past the cube; otherwise
 * the C mark path's status, and on GEAR_SUCCESS the
 * engine's count_overall moves by count. */
int32_t engine_mark(Engine *engine, uint32_t word);
int32_t engine_unmark(Engine *engine, uint32_t word);

/* engine_call: Engine.__call__. Allocates request sectors
 * and reports the outcome in *result. */
void engine_call(Engine *engine, uint32_t request,
                 EngineResult *result);

/* engine_deallocate: Engine.deallocate. The Python method
 * is an unimplemented stub (its body is a docstring); this
 * transcription has the same content: nothing. The word
 * argument mirrors the Python's address parameter. */
void engine_deallocate(Engine *engine, uint32_t word);

#endif /* ISCANDAR_ENGINE_H */
