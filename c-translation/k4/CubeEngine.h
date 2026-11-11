/* CubeEngine.h -- the per-cube engine boundary for Beam.c.
 *
 * ==================== REAL ENGINE BOUNDARY ====================
 * Beam.py drives the k=3 Engine (allocator.py) directly, one
 * Engine instance per cube. This header declares the minimal
 * interface Beam needs, and CubeEngine.c supplies an ADAPTER
 * behind it: each CubeEngine holds one real k=3 C Engine
 * (iscandar-engine, Engine.h), and the functions below reduce
 * its three return shapes to the CubeEngineResult Beam.c
 * observes.
 *
 * The adapter keeps the Engine's boundary contract exactly,
 * since that is what Beam observes:
 *   - a request larger than the sectors remaining returns a
 *     FAIL status before any search (Engine.__call__ step 1,
 *     inside engine_call);
 *   - a zero request yields no result (the Nothing mode);
 *   - a negative request yields no result: the Python Engine
 *     returns None there (the Nothing fallback for an
 *     out-of-table key), and the C engine_call, which takes
 *     uint32_t, cannot express the case, so the adapter
 *     answers it directly, with no engine call and no gear
 *     interaction, exactly as the Python's None path has none;
 *   - allocate yields a (start, count) pointer result, a bare
 *     status, or nothing -- the three shapes Engine.__call__
 *     can return (MPUPointer, status int, None);
 *   - unmark returns the k=3 engine's own status for the
 *     (start, count) block (GEAR_SUCCESS / FAIL_OVERLAY /
 *     FAIL), mapped value-for-value (see CubeEngine.c).
 *
 * Placement inside the cube is the real modes' probe
 * placement, no longer a stub's first fit. The test oracle
 * (gen_k4_expected.py) drives the real Python Beam over the
 * real Python k=3 engine, so the Beam test now compares Beam
 * plus engine against Beam plus engine.
 *
 * INTERFACE CHANGE FROM THE STUB ERA, flagged per the work
 * order: the function signatures and CubeEngineResult are
 * unchanged, and Beam.c is unchanged. The CubeEngine struct
 * changed: the stub's bitmap store is gone, replaced by the
 * real Engine it adapts, plus the count_overall mirror
 * Beam.c's scan reads (kept in step by the adapter after
 * every call and unmark). This header now includes the k=3
 * engine's Engine.h, so compiling anything that includes
 * this header needs the engine directory on the include
 * path (run_k4_c_tests.sh supplies it).
 * ============================================================= */

#ifndef ISCANDAR_K4_CUBE_ENGINE_H
#define ISCANDAR_K4_CUBE_ENGINE_H

#include <stdint.h>

#include "Engine.h"

#define K4_SECTORS_PER_CUBE 32768u

/* The three shapes an engine call can return. */
#define K4_ENGINE_NONE    0  /* Python: None (Nothing mode) */
#define K4_ENGINE_STATUS  1  /* Python: a bare status int */
#define K4_ENGINE_POINTER 2  /* Python: an MPUPointer */

typedef struct {
    int kind;          /* K4_ENGINE_* */
    int32_t status;    /* valid when kind == K4_ENGINE_STATUS */
    uint32_t start;    /* valid when kind == K4_ENGINE_POINTER */
    uint32_t count;    /* valid when kind == K4_ENGINE_POINTER */
} CubeEngineResult;

typedef struct {
    Engine engine;           /* the real k=3 C Engine for this cube */
    uint32_t count_overall;  /* mirror of engine.count_overall,
                                the field Beam.c's scan reads */
} CubeEngine;

void cube_engine_init(CubeEngine *engine);

/* Engine.__call__(request), boundary contract as above. */
void cube_engine_call(CubeEngine *engine, int32_t request,
                      CubeEngineResult *out);

/* Engine.unmark(pointer) reduced to (start, count): returns
 * GEAR_SUCCESS, FAIL, or FAIL_OVERLAY (the K4 names for the
 * same Status.py values). */
int32_t cube_engine_unmark(CubeEngine *engine, uint32_t start,
                           uint32_t count);

#endif /* ISCANDAR_K4_CUBE_ENGINE_H */
