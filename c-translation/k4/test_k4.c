/* test_k4.c -- the C driver for the k=4 transcription test.
 *
 * Runs the identical operation script as gen_k4_expected.py
 * (the Python oracle) and prints the identical transcript.
 * The outputs are diffed; the diff must be empty. Both sides
 * run the real k=3 engine per cube: the Python oracle through
 * Beam.py, this driver through the CubeEngine adapter. See
 * CubeEngine.h for the boundary. */

#include <stdio.h>

#include "Beam.h"
#include "BeamCoordinate.h"
#include "CPUPointer.h"
#include "K4Status.h"

static const struct { int32_t cube, sector, row, plane; } BCS[6] = {
    {0, 0, 0, 0}, {2, 3, 4, 5}, {31, 31, 31, 31},
    {5, 40, 0, 0}, {7, 0, 33, 0}, {0, 0, 0, 0}
};

static void print_bc(int i, const BeamCoordinate *c)
{
    K3Coordinate k3;
    beam_coordinate_k3(c, &k3);
    printf("BC %d c=%d s=%d r=%d p=%d disp=%lld\n", i,
           (int)c->cube, (int)c->sector, (int)c->row, (int)c->plane,
           (long long)k3_coordinate_displacement(&k3));
}

static void test_coordinates(void)
{
    BeamCoordinate bc[6];
    static const int pairs[4][2] = {{0, 5}, {0, 1}, {1, 1}, {3, 4}};
    int i;

    for (i = 0; i < 6; i++) {
        beam_coordinate_init(&bc[i], BCS[i].cube, BCS[i].sector,
                             BCS[i].row, BCS[i].plane);
        print_bc(i, &bc[i]);
    }
    for (i = 0; i < 4; i++) {
        printf("BCEQ %d %d = %d\n", pairs[i][0], pairs[i][1],
               beam_coordinate_equal(&bc[pairs[i][0]],
                                     &bc[pairs[i][1]]));
    }
}

static const struct { int bc; uint32_t count; uint32_t res; int order; }
CREATES[9] = {
    {1, 100, 0, 0}, {1, 100, 1, 0}, {1, 100, 0, 1}, {1, 100, 1, 1},
    {2, 32768, 0, 0}, {0, 1, 0, 0}, {3, 5, 0, 0}, {4, 10, 0, 0},
    {1, 32768, 0, 1}
};

static void test_cpu_create(void)
{
    int i;

    for (i = 0; i < 9; i++) {
        BeamCoordinate bc;
        CPUPointer p;
        CPUPointerFields f;
        BeamCoordinate co;
        int32_t st;

        beam_coordinate_init(&bc, BCS[CREATES[i].bc].cube,
                             BCS[CREATES[i].bc].sector,
                             BCS[CREATES[i].bc].row,
                             BCS[CREATES[i].bc].plane);
        st = cpu_pointer_create(&bc, CREATES[i].count,
                                CREATES[i].res, CREATES[i].order, &p);
        if (st != K4_SUCCESS) {
            printf("CPU %d -> FAIL\n", i);
            continue;
        }
        (void)cpu_pointer_coordinate(&p, K4_ORDER_MOTOROLA, &co);
        (void)cpu_pointer_unload(&p, CREATES[i].order, &f);
        printf("CPU %d word=0x%016llX v=%u r=%u start=%u count=%u "
               "coord=(%d,%d,%d,%d) ov=%u or=%u ostart=%u ocount=%u\n",
               i, (unsigned long long)cpu_pointer_word(&p),
               cpu_pointer_virtual(&p), cpu_pointer_reserved(&p),
               cpu_pointer_start(&p), cpu_pointer_count(&p),
               (int)co.cube, (int)co.sector, (int)co.row,
               (int)co.plane, f.virtual_bit, f.reserved_bit,
               f.start, f.count);
    }
}

static void test_cpu_bad(void)
{
    /* (coordinate source, count, reserved, order): source >= 0
     * indexes BCS; -1 and -2 are the extra coordinates X0/X1. */
    static const struct { int src; uint32_t count; uint32_t res;
                          int order; } BAD[6] = {
        {1, 0, 0, 0}, {1, 32769, 0, 0}, {1, 10, 2, 0},
        {-1, 10, 0, 0}, {-2, 10, 0, 0}, {1, 10, 0, 99}
    };
    int i;

    for (i = 0; i < 6; i++) {
        BeamCoordinate bc;
        CPUPointer p;
        int32_t st;

        if (BAD[i].src == -1) {
            beam_coordinate_init(&bc, 32768, 0, 0, 0);
        } else if (BAD[i].src == -2) {
            beam_coordinate_init(&bc, 0, -1, 0, 0);
        } else {
            beam_coordinate_init(&bc, BCS[BAD[i].src].cube,
                                 BCS[BAD[i].src].sector,
                                 BCS[BAD[i].src].row,
                                 BCS[BAD[i].src].plane);
        }
        st = cpu_pointer_create(&bc, BAD[i].count, BAD[i].res,
                                BAD[i].order, &p);
        if (st != K4_SUCCESS) {
            printf("CPUX %d -> FAIL\n", i);
        } else {
            printf("CPUX %d -> word=0x%016llX\n", i,
                   (unsigned long long)cpu_pointer_word(&p));
        }
    }
}

static void test_adopt(void)
{
    static const uint64_t WORDS[5] = {
        0x0000000000000000ull, 0xFFFFFFFFFFFFFFFFull,
        0x8000000000000063ull, 0x0000000000000001ull,
        0xDEADBEEF12345678ull
    };
    int i;

    for (i = 0; i < 5; i++) {
        CPUPointer p;
        CPUPointerFields f;
        BeamCoordinate co;

        (void)cpu_pointer_adopt(WORDS[i], &p);
        (void)cpu_pointer_coordinate(&p, K4_ORDER_MOTOROLA, &co);
        (void)cpu_pointer_unload(&p, K4_ORDER_INTEL, &f);
        printf("ADOPT %d word=0x%016llX v=%u r=%u start=%u "
               "count=%u coord=(%d,%d,%d,%d) iv=%u ir=%u "
               "istart=%u icount=%u\n",
               i, (unsigned long long)cpu_pointer_word(&p),
               cpu_pointer_virtual(&p), cpu_pointer_reserved(&p),
               cpu_pointer_start(&p), cpu_pointer_count(&p),
               (int)co.cube, (int)co.sector, (int)co.row,
               (int)co.plane, f.virtual_bit, f.reserved_bit,
               f.start, f.count);
    }
}

typedef struct { char op; int32_t val; } BeamOp;

static const BeamOp OPS0[] = {
    {'A', 100}, {'A', 100}, {'A', 200}, {'F', 1}, {'A', 50},
    {'A', 32768}, {'A', 1}, {'F', 4}, {'F', 4}, {'A', 40000},
    {'A', 0}, {'A', -5}, {'A', 32417}, {'F', 0}, {'F', 2},
    {'F', 3}, {'F', 5}, {'F', 6}, {'A', 32768}, {'A', 32768},
    {'A', 1}
};
static const BeamOp OPS1[] = {
    {'A', 500}, {'A', 32768}, {'F', 0}, {'A', 32768}, {'A', 1}
};
static const BeamOp OPS2[] = { {'A', 10} };
static const BeamOp OPS3[] = { {'A', 10} };
static const BeamOp OPS4[] = {
    {'A', 32768}, {'A', 32768}, {'A', 32768}, {'A', 32768},
    {'A', 32768}
};

static void run_beam(int bi, int32_t arg, const BeamOp *ops, int nops)
{
    Beam beam;
    CPUPointer produced[16];
    int nproduced = 0;
    int i;

    if (beam_init(&beam, arg) != K4_SUCCESS) {
        printf("BEAMINIT %d -> FAIL\n", bi);
        return;
    }
    printf("BEAMINIT %d arg=%d card=%d supported=%d\n", bi,
           (int)arg, (int)beam.cardinality,
           (int)beam.sectors_supported);

    for (i = 0; i < nops; i++) {
        if (ops[i].op == 'A') {
            CPUPointer p;
            int32_t st = beam_allocate(&beam, ops[i].val, &p);
            if (st != K4_SUCCESS) {
                printf("BM%d A req=%d -> NONE overall=%d lead=%d\n",
                       bi, (int)ops[i].val, (int)beam.count_overall,
                       (int)beam.lead);
            } else {
                produced[nproduced++] = p;
                printf("BM%d A req=%d -> 0x%016llX overall=%d "
                       "lead=%d\n", bi, (int)ops[i].val,
                       (unsigned long long)cpu_pointer_word(&p),
                       (int)beam.count_overall, (int)beam.lead);
            }
        } else {
            int32_t st = beam_free(&beam, &produced[ops[i].val]);
            printf("BM%d F #%d -> status=%d overall=%d lead=%d\n",
                   bi, (int)ops[i].val, (int)st,
                   (int)beam.count_overall, (int)beam.lead);
        }
    }
}

int main(void)
{
    test_coordinates();
    test_cpu_create();
    test_cpu_bad();
    test_adopt();
    run_beam(0, 2, OPS0, (int)(sizeof(OPS0) / sizeof(OPS0[0])));
    run_beam(1, 1, OPS1, (int)(sizeof(OPS1) / sizeof(OPS1[0])));
    run_beam(2, 0, OPS2, (int)(sizeof(OPS2) / sizeof(OPS2[0])));
    run_beam(3, -3, OPS3, (int)(sizeof(OPS3) / sizeof(OPS3[0])));
    run_beam(4, 4, OPS4, (int)(sizeof(OPS4) / sizeof(OPS4[0])));
    return 0;
}
