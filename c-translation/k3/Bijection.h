/* Bijection.h -- transcription of Bijection.py, the author's
 * design; the Python is the specification and this code
 * reproduces it exactly. Transcription only: no original
 * content. */

#ifndef ISCANDAR_BIJECTION_H
#define ISCANDAR_BIJECTION_H

#include <stdint.h>

#include "Coordinate.h"

/* class Bijection: "Conversions from paper: displacement <->
 * coordinate, exactly." Bijection.A in the Python is
 * Geometry.ARITY; the formulas below walk by it directly.
 *
 * The Python validates nothing: both directions are pure
 * arithmetic (divmod one way, multiply-accumulate the other),
 * exact for any displacement or fields the words can hold,
 * including displacements past the cube (32768 -> plane 32).
 * The C is the same arithmetic in uint32_t, so it agrees with
 * the Python everywhere the engine operates and everywhere
 * both can represent the result. Neither function can fail,
 * so neither returns a status. */

/* to_coordinate(displacement): plane, rem = divmod(d, A*A);
 * row, sector = divmod(rem, A). */
__declspec(dllexport)
void bijection_to_coordinate(uint32_t displacement,
                             Coordinate *coordinate_out);

/* to_displacement(coordinate):
 * (plane * A + row) * A + sector. */
__declspec(dllexport)
uint32_t bijection_to_displacement(const Coordinate *coordinate);

#endif /* ISCANDAR_BIJECTION_H */
