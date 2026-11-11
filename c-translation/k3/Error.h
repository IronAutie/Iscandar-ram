/* Error.h -- transcription of Error.py, the author's design;
 * the Python is the specification and this code reproduces it
 * exactly. Transcription only: no original content. */

#ifndef ISCANDAR_ERROR_H
#define ISCANDAR_ERROR_H

#include <stdint.h>

#include "Geometry.h"

/* Cube.SECTORS_PER_CUBE in the Python: ARITY ** 3, the whole
 * cube, 32768 sectors. Error.py takes its range span from
 * Cube; in this tranche the value is expressed from Geometry,
 * the constant both are built from. */
#define ERROR_SECTORS_PER_CUBE \
    (GEOMETRY_ARITY * GEOMETRY_ARITY * GEOMETRY_ARITY)

/* class Error (Mode): the fill for the decision table, in
 * case any table entries are missing from fill by other
 * handlers. A complete table never selects it. Only Error.py's
 * own definitions are transcribed here; the Mode machinery is
 * not part of this tranche. */

/* Error.range(): range(Cube.SECTORS_PER_CUBE) -- 0 through
 * 32767. Reported as the inclusive bounds. */
__declspec(dllexport)
void error_range(uint32_t *lo_out, uint32_t *hi_out);

/* Error.search(...): if Error is selected, that is a table
 * fault, so it fails explicitly, in the uniform search shape
 * (FAIL, 0, 0, 0), and Engine returns that FAIL with no mark
 * and no pointer. No sweep. The C writes the three zero
 * outputs and returns FAIL. */
__declspec(dllexport)
int32_t error_search(uint32_t *plane_out, uint32_t *row_out,
                     uint32_t *sector_out);

#endif /* ISCANDAR_ERROR_H */
