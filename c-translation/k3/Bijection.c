/* Bijection.c -- see Bijection.h. Transcription of
 * Bijection.py; the Python is the specification. */

#include "Bijection.h"
#include "Geometry.h"

void bijection_to_coordinate(uint32_t displacement,
                             Coordinate *coordinate_out)
{
    uint32_t plane;
    uint32_t rem;
    uint32_t row;
    uint32_t sector;

    plane  = displacement / (GEOMETRY_ARITY * GEOMETRY_ARITY);
    rem    = displacement % (GEOMETRY_ARITY * GEOMETRY_ARITY);
    row    = rem / GEOMETRY_ARITY;
    sector = rem % GEOMETRY_ARITY;

    coordinate_out->sector = sector;
    coordinate_out->row    = row;
    coordinate_out->plane  = plane;
}

uint32_t bijection_to_displacement(const Coordinate *coordinate)
{
    return ((coordinate->plane * GEOMETRY_ARITY + coordinate->row)
            * GEOMETRY_ARITY + coordinate->sector);
}
