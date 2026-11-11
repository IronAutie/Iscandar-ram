/* Coordinate.c -- see Coordinate.h. Transcription of
 * Coordinate.py; the Python is the specification. */

#include "Coordinate.h"

void coordinate_init(Coordinate *coordinate,
                     uint32_t sector, uint32_t row, uint32_t plane)
{
    coordinate->sector = sector;
    coordinate->row    = row;
    coordinate->plane  = plane;
}

uint32_t coordinate_displacement(const Coordinate *coordinate)
{
    /* sector + row * Row.SECTORS_PER_ROW
     *        + plane * Plane.SECTORS_PER_PLANE */
    return (coordinate->sector
            + coordinate->row   * COORDINATE_SECTORS_PER_ROW
            + coordinate->plane * COORDINATE_SECTORS_PER_PLANE);
}

int coordinate_equals(const Coordinate *a, const Coordinate *b)
{
    return (a->sector == b->sector
            && a->row    == b->row
            && a->plane  == b->plane) ? 1 : 0;
}
