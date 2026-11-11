/* Coordinate.h -- transcription of Coordinate.py, the author's
 * design; the Python is the specification and this code
 * reproduces it exactly. Transcription only: no original
 * content. */

#ifndef ISCANDAR_COORDINATE_H
#define ISCANDAR_COORDINATE_H

#include <stdint.h>

#include "Geometry.h"

/* Row.SECTORS_PER_ROW and Plane.SECTORS_PER_PLANE in the Python:
 * the row and plane spans the displacement formula walks. */
#define COORDINATE_SECTORS_PER_ROW \
    GEOMETRY_ARITY
#define COORDINATE_SECTORS_PER_PLANE \
    (GEOMETRY_ARITY * GEOMETRY_ARITY)

/* class Coordinate: "A lattice coordinate: one sector, in one
 * row, in one plane." The Python __init__ validates nothing --
 * it stores the three fields as given -- so this type stores
 * them as given too. Range discipline lives in the modules
 * that do validate in the Python (Bijection's arithmetic is
 * exact for any fields; MPUPointer refuses out-of-range
 * fields). Fields are unsigned: the engine's coordinates are
 * never negative. */
typedef struct {
    uint32_t sector;
    uint32_t row;
    uint32_t plane;
} Coordinate;

/* Coordinate(sector=..., row=..., plane=...): plain storage,
 * the Python defaults being 0, 0, 0. */
__declspec(dllexport)
void coordinate_init(Coordinate *coordinate,
                     uint32_t sector, uint32_t row, uint32_t plane);

/* The displacement property: this coordinate as a scalar
 * address (Axiom IV, Equation 6). Never fails in the Python,
 * so it returns the value directly. */
__declspec(dllexport)
uint32_t coordinate_displacement(const Coordinate *coordinate);

/* __eq__: the field triples compare equal or they do not.
 * Returns 1 when equal, 0 when not. */
__declspec(dllexport)
int coordinate_equals(const Coordinate *a, const Coordinate *b);

#endif /* ISCANDAR_COORDINATE_H */
