/* Beam.h -- C transcription of Beam.py.
 *
 * A Beam is a line of cube engines under a rotary rule: an
 * allocation is offered to the lead engine first, then to each
 * engine in turn as the lead rotates, and the first engine that
 * accepts serves it. A fit never crosses a cube wall. Beam's own
 * logic -- the overall gate, the per-engine room check before
 * scanning, the first-acceptance walk, the count bookkeeping,
 * and the transform of an accepting engine's k=3 pointer into a
 * k=4 CPUPointer -- is transcribed in full from Beam.py. The
 * per-cube engines sit behind the adapter boundary in
 * CubeEngine.h (one real k=3 C Engine per cube).
 *
 * Settled shape: a Beam runs 1 to 32 engines. The Python clamps
 * a non-positive cardinality to 1; the C does the same, and
 * refuses a cardinality above 32 with K4_FAIL (the Python would
 * build that many engines; the settled design caps the beam at
 * 32, and the C array is sized to the cap).
 *
 * Returns: beam_allocate returns K4_SUCCESS with *out written
 * when an engine accepts, and K4_FAIL where the Python returns
 * None. beam_free returns the engine's own status, as the
 * Python does. */

#ifndef ISCANDAR_K4_BEAM_H
#define ISCANDAR_K4_BEAM_H

#include <stdint.h>

#include "CPUPointer.h"
#include "CubeEngine.h"

#define K4_BEAM_MAX_CUBES 32

typedef struct {
    int32_t cardinality;
    int32_t count_overall;
    int32_t sectors_supported;
    int32_t lead;
    CubeEngine engines[K4_BEAM_MAX_CUBES];
} Beam;

/* Beam.__init__: cardinality <= 0 becomes 1; above 32 refuses. */
int32_t beam_init(Beam *beam, int32_t cardinality);

/* Beam.rotate: advance the lead engine, wrapping. */
void beam_rotate(Beam *beam);

/* Beam.allocate(request). */
int32_t beam_allocate(Beam *beam, int32_t request, CPUPointer *out);

/* Beam.free(pointer): deallocate in the cube the pointer was
 * born in. A pointer naming a cube this beam does not run is
 * K4_FAIL (the Python raises IndexError there; the C doctrine
 * is a status, never a crash -- see the composition record). */
int32_t beam_free(Beam *beam, const CPUPointer *pointer);

#endif /* ISCANDAR_K4_BEAM_H */
