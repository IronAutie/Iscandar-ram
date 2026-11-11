/* BeamCoordinate.c -- see BeamCoordinate.h for the contract. */

#include "BeamCoordinate.h"

void beam_coordinate_init(BeamCoordinate *out, int32_t cube,
                          int32_t sector, int32_t row, int32_t plane)
{
    out->cube = cube;
    out->sector = sector;
    out->row = row;
    out->plane = plane;
}

void beam_coordinate_k3(const BeamCoordinate *bc, K3Coordinate *out)
{
    out->sector = bc->sector;
    out->row = bc->row;
    out->plane = bc->plane;
}

int64_t k3_coordinate_displacement(const K3Coordinate *c)
{
    /* Row.SECTORS_PER_ROW = 32, Plane.SECTORS_PER_PLANE = 1024. */
    return (int64_t)c->sector
         + (int64_t)c->row * 32
         + (int64_t)c->plane * 1024;
}

int beam_coordinate_equal(const BeamCoordinate *a,
                          const BeamCoordinate *b)
{
    return a->cube == b->cube
        && a->sector == b->sector
        && a->row == b->row
        && a->plane == b->plane;
}
