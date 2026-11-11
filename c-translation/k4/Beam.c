/* Beam.c -- see Beam.h. The control flow follows Beam.py line
 * for line: allocate's overall gate, the scan's per-engine room
 * check, the first-acceptance walk with rotation, transform on
 * acceptance, and free's bookkeeping on GEAR_SUCCESS only. */

#include "Beam.h"
#include "K4Status.h"

int32_t beam_init(Beam *beam, int32_t cardinality)
{
    int32_t i;

    /* Beam.py: cardinality = [cardinality, 1][cardinality <= 0] */
    if (cardinality <= 0) {
        cardinality = 1;
    }
    if (cardinality > K4_BEAM_MAX_CUBES) {
        return K4_FAIL;
    }

    beam->count_overall = 0;
    beam->cardinality = cardinality;
    for (i = 0; i < cardinality; i++) {
        cube_engine_init(&beam->engines[i]);
    }
    beam->sectors_supported =
        (int32_t)K4_SECTORS_PER_CUBE * cardinality;
    beam->lead = 0;
    return K4_SUCCESS;
}

void beam_rotate(Beam *beam)
{
    beam->lead = (beam->lead + 1) % beam->cardinality;
}

/* Beam.scan: offer the request to the lead engine. A lead
 * engine whose remaining count cannot hold the request is
 * skipped without a call, exactly as in the Python. */
static void beam_scan(Beam *beam, int32_t request,
                      CubeEngineResult *out)
{
    CubeEngine *engine = &beam->engines[beam->lead];

    if ((int32_t)K4_SECTORS_PER_CUBE - (int32_t)engine->count_overall
        < request) {
        out->kind = K4_ENGINE_NONE;
        return;
    }

    cube_engine_call(engine, request, out);
}

/* Beam.transform: the accepting engine's k=3 pointer, with its
 * cube, becomes a k=4 CPUPointer. The engine's (start, count)
 * is the MPUPointer reduced to its fields: its coordinate is
 * Bijection.to_coordinate(start), and the count joins here,
 * when the CPUPointer is minted. Creation in this path cannot
 * fail (cube < 32, displacement < 32768, count in 1..32768);
 * the status is checked anyway, per doctrine. */
static int32_t beam_transform(const CubeEngineResult *result,
                              int32_t cube, CPUPointer *out)
{
    BeamCoordinate bc;
    uint32_t d = result->start;
    int32_t plane = (int32_t)(d / 1024u);
    int32_t row = (int32_t)((d % 1024u) / 32u);
    int32_t sector = (int32_t)(d % 32u);

    beam_coordinate_init(&bc, cube, sector, row, plane);
    return cpu_pointer_create(&bc, result->count, 0,
                              K4_ORDER_MOTOROLA, out);
}

int32_t beam_allocate(Beam *beam, int32_t request, CPUPointer *out)
{
    int32_t index;

    if (beam->sectors_supported - beam->count_overall < request) {
        return K4_FAIL;
    }

    for (index = 0; index < beam->cardinality; index++) {
        CubeEngineResult result;

        beam_scan(beam, request, &result);

        if (result.kind == K4_ENGINE_POINTER) {
            beam->count_overall += request;
            return beam_transform(&result, beam->lead, out);
        }

        beam_rotate(beam);
    }

    return K4_FAIL;
}

int32_t beam_free(Beam *beam, const CPUPointer *pointer)
{
    BeamCoordinate coordinate;
    K3Coordinate local;
    int32_t status;
    uint32_t start;
    uint32_t count;

    /* pointer.coordinate(): where this block was born. */
    status = cpu_pointer_coordinate(pointer, K4_ORDER_MOTOROLA,
                                    &coordinate);
    if (status != K4_SUCCESS) {
        return status;
    }
    if (coordinate.cube < 0 || coordinate.cube >= beam->cardinality) {
        return K4_FAIL;
    }

    /* MPUPointer.load_coordinate(coordinate.k3(), count),
     * reduced to the (start, count) the unmark consumes. */
    beam_coordinate_k3(&coordinate, &local);
    start = (uint32_t)k3_coordinate_displacement(&local);
    count = cpu_pointer_count(pointer);

    status = cube_engine_unmark(&beam->engines[coordinate.cube],
                                start, count);

    if (status == K4_GEAR_SUCCESS) {
        beam->count_overall -= (int32_t)count;
    }

    return status;
}
