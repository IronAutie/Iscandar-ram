/* FoundationCheck.c -- test driver for the C foundation
 * tranche (Geometry, Coordinate, Bijection, MPUPointer,
 * Nothing, Error). It runs the identical operation script as
 * gen_foundation_vectors.py and prints the identical lines;
 * the diff between its output and
 * foundation_vectors_expected.txt (the Python-generated
 * oracle) must be empty.
 *
 * Two invariants are checked silently, with a nonzero exit
 * instead of a printed line, because the Python produces no
 * object at all on a refusal and so has no line for them: a
 * refused load or adoption writes nothing to its output.
 *
 * Build (Linux; the prefix header defines __declspec away,
 * exactly as iscandar-linux-setup.sh builds the gears):
 *   printf '#define __declspec(x)\n' > /tmp/declspec.h
 *   cc -std=c99 -Wall -Wextra -include /tmp/declspec.h \
 *      -o /tmp/FoundationCheck FoundationCheck.c Coordinate.c \
 *      Bijection.c MPUPointer.c Nothing.c Error.c
 * Run:
 *   /tmp/FoundationCheck > /tmp/foundation_vectors_actual.txt
 *   diff foundation_vectors_expected.txt \
 *        /tmp/foundation_vectors_actual.txt
 */

#include <stdint.h>
#include <stdio.h>

#include "Geometry.h"
#include "Coordinate.h"
#include "Bijection.h"
#include "MPUPointer.h"
#include "Nothing.h"
#include "Error.h"
#include "status.h"

static const uint32_t BOUNDARY[] =
    {0, 1, 8, 9, 32, 33, 64, 65, 1024, 1025, 32767, 32768};
static const uint32_t COUNTS[] =
    {1, 8, 9, 32, 33, 64, 65, 1024, 1025, 32767, 32768};
static const uint32_t STARTS[] =
    {0, 1, 8, 9, 32, 33, 64, 65, 1024, 1025, 32767};

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

static void fields_line(const char *prefix, uint32_t word,
                        const MPUPointerFields *f)
{
    printf("%s -> word=0x%08X v=%u r=%u start=%u count=%u\n",
           prefix, (unsigned)word, (unsigned)f->virtual,
           (unsigned)f->reserved, (unsigned)f->start,
           (unsigned)f->count);
}

static int check_load(uint32_t start, uint32_t count,
                      uint32_t reserved, int order)
{
    uint32_t word = 0;
    MPUPointerFields f;
    char prefix[128];

    if (mpupointer_load(start, count, reserved, order, &word)
            != SUCCESS) {
        return 1;
    }
    if (mpupointer_unload(word, order, &f) != SUCCESS) {
        return 1;
    }
    (void)snprintf(prefix, sizeof(prefix),
                   "mpu load start=%u count=%u reserved=%u "
                   "order=%s",
                   (unsigned)start, (unsigned)count,
                   (unsigned)reserved,
                   order == MPUPOINTER_INTEL ? "intel" : "motorola");
    fields_line(prefix, word, &f);
    return 0;
}

int main(void)
{
    size_t i;
    Coordinate c;
    Coordinate c2;
    MPUPointerFields f;
    uint32_t word;
    uint32_t lo;
    uint32_t hi;
    int32_t status;
    int present;

    /* --- Geometry --- */
    printf("geometry arity=%d dimensions=%d sectors_per_byte=%d\n",
           GEOMETRY_ARITY, GEOMETRY_DIMENSIONS,
           GEOMETRY_SECTORS_PER_BYTE);

    /* --- MPUPointer class constants --- */
    printf("mpu max_word=%u max_start=%d max_count=%d\n",
           (unsigned)MPUPOINTER_MAX_WORD, MPUPOINTER_MAX_START,
           MPUPOINTER_MAX_COUNT);

    /* --- Coordinate: displacement --- */
    {
        static const uint32_t triples[][3] = {
            {0, 0, 0}, {1, 0, 0}, {31, 0, 0}, {0, 1, 0},
            {0, 0, 1}, {5, 7, 3}, {31, 31, 31}
        };
        for (i = 0; i < ARRAY_LEN(triples); i++) {
            coordinate_init(&c, triples[i][0], triples[i][1],
                            triples[i][2]);
            printf("coord disp sector=%u row=%u plane=%u -> %u\n",
                   (unsigned)c.sector, (unsigned)c.row,
                   (unsigned)c.plane,
                   (unsigned)coordinate_displacement(&c));
        }
    }

    /* --- Coordinate: equality --- */
    {
        static const uint32_t pairs[][6] = {
            {1, 2, 3, 1, 2, 3}, {1, 2, 3, 1, 2, 4},
            {0, 0, 0, 0, 0, 1}, {31, 31, 31, 31, 31, 31}
        };
        for (i = 0; i < ARRAY_LEN(pairs); i++) {
            coordinate_init(&c, pairs[i][0], pairs[i][1],
                            pairs[i][2]);
            coordinate_init(&c2, pairs[i][3], pairs[i][4],
                            pairs[i][5]);
            printf("coord eq (%u,%u,%u)==(%u,%u,%u) -> %d\n",
                   (unsigned)c.sector, (unsigned)c.row,
                   (unsigned)c.plane, (unsigned)c2.sector,
                   (unsigned)c2.row, (unsigned)c2.plane,
                   coordinate_equals(&c, &c2));
        }
    }

    /* --- Bijection --- */
    for (i = 0; i < ARRAY_LEN(BOUNDARY); i++) {
        bijection_to_coordinate(BOUNDARY[i], &c);
        printf("bijection to_coordinate %u -> sector=%u row=%u "
               "plane=%u\n",
               (unsigned)BOUNDARY[i], (unsigned)c.sector,
               (unsigned)c.row, (unsigned)c.plane);
    }
    for (i = 0; i < ARRAY_LEN(BOUNDARY); i++) {
        bijection_to_coordinate(BOUNDARY[i], &c);
        printf("bijection roundtrip %u -> %u\n",
               (unsigned)BOUNDARY[i],
               (unsigned)bijection_to_displacement(&c));
    }
    {
        static const uint32_t triples[][3] = {
            {0, 0, 0}, {31, 31, 31}, {1, 0, 1}, {0, 31, 0}
        };
        for (i = 0; i < ARRAY_LEN(triples); i++) {
            coordinate_init(&c, triples[i][0], triples[i][1],
                            triples[i][2]);
            printf("bijection to_displacement sector=%u row=%u "
                   "plane=%u -> %u\n",
                   (unsigned)c.sector, (unsigned)c.row,
                   (unsigned)c.plane,
                   (unsigned)bijection_to_displacement(&c));
        }
    }

    /* --- MPUPointer.load, Motorola: count sweep --- */
    for (i = 0; i < ARRAY_LEN(COUNTS); i++) {
        if (check_load(0, COUNTS[i], 0, MPUPOINTER_MOTOROLA)) {
            return 1;
        }
    }
    /* --- MPUPointer.load, Motorola: start sweep --- */
    for (i = 0; i < ARRAY_LEN(STARTS); i++) {
        if (check_load(STARTS[i], 1, 0, MPUPOINTER_MOTOROLA)) {
            return 1;
        }
    }
    /* --- MPUPointer.load, Intel --- */
    {
        static const uint32_t sc[][2] = {
            {0, 1}, {0, 32768}, {32767, 1}, {32767, 32768},
            {1025, 9}
        };
        for (i = 0; i < ARRAY_LEN(sc); i++) {
            if (check_load(sc[i][0], sc[i][1], 0,
                           MPUPOINTER_INTEL)) {
                return 1;
            }
        }
    }
    /* --- MPUPointer.load, reserved --- */
    if (check_load(0, 1, 1, MPUPOINTER_MOTOROLA)) {
        return 1;
    }
    if (check_load(100, 200, 1, MPUPOINTER_INTEL)) {
        return 1;
    }

    /* --- MPUPointer.load_coordinate --- */
    {
        static const uint32_t dc[][2] = {
            {0, 1}, {1, 8}, {1024, 1024}, {32767, 32768},
            {1025, 9}
        };
        char prefix[128];
        for (i = 0; i < ARRAY_LEN(dc); i++) {
            bijection_to_coordinate(dc[i][0], &c);
            if (mpupointer_load_coordinate(&c, dc[i][1], 0,
                                           MPUPOINTER_MOTOROLA,
                                           &word) != SUCCESS) {
                return 1;
            }
            if (mpupointer_unload(word, MPUPOINTER_MOTOROLA, &f)
                    != SUCCESS) {
                return 1;
            }
            (void)snprintf(prefix, sizeof(prefix),
                           "mpu load_coordinate disp=%u count=%u",
                           (unsigned)dc[i][0], (unsigned)dc[i][1]);
            fields_line(prefix, word, &f);
        }
    }

    /* --- MPUPointer adoption --- */
    {
        static const uint32_t words[] = {
            0, 1, 0x80000000u, 0x40000000u, 0x0000FFFFu,
            0xFFFFFFFFu, 0xDEADBEEFu
        };
        for (i = 0; i < ARRAY_LEN(words); i++) {
            if (mpupointer_adopt(words[i], &word) != SUCCESS) {
                return 1;
            }
            printf("mpu adopt word=0x%08X -> v=%u r=%u start=%u "
                   "count=%u\n",
                   (unsigned)word,
                   (unsigned)mpupointer_virtual(word),
                   (unsigned)mpupointer_reserved(word),
                   (unsigned)mpupointer_start(word),
                   (unsigned)mpupointer_count(word));
        }
    }

    /* --- MPUPointer.coordinate --- */
    if (mpupointer_load(1025, 9, 0, MPUPOINTER_MOTOROLA, &word)
            != SUCCESS) {
        return 1;
    }
    if (mpupointer_coordinate(word, MPUPOINTER_MOTOROLA, &c)
            != SUCCESS) {
        return 1;
    }
    printf("mpu coordinate start=1025 count=9 order=motorola -> "
           "sector=%u row=%u plane=%u\n",
           (unsigned)c.sector, (unsigned)c.row,
           (unsigned)c.plane);
    if (mpupointer_load(32767, 1, 0, MPUPOINTER_INTEL, &word)
            != SUCCESS) {
        return 1;
    }
    if (mpupointer_coordinate(word, MPUPOINTER_INTEL, &c)
            != SUCCESS) {
        return 1;
    }
    printf("mpu coordinate start=32767 count=1 order=intel -> "
           "sector=%u row=%u plane=%u\n",
           (unsigned)c.sector, (unsigned)c.row,
           (unsigned)c.plane);

    /* --- MPUPointer refusals: the printed status is the
     * returned status, so a wrong code fails the diff. The
     * sentinel additionally proves, silently, that a refused
     * call wrote nothing. --- */
    word = 0xAAAAAAAAu;
    status = mpupointer_load(32768, 1, 0, MPUPOINTER_MOTOROLA,
                             &word);
    printf("mpu refuse load start=32768 count=1 -> status=%d\n",
           (int)status);
    if (word != 0xAAAAAAAAu) {
        return 1;
    }
    status = mpupointer_load(0xFFFFFFFFu, 1, 0, MPUPOINTER_MOTOROLA,
                             &word);
    printf("mpu refuse load start=-1 count=1 -> status=%d\n",
           (int)status);
    status = mpupointer_load(0, 0, 0, MPUPOINTER_MOTOROLA, &word);
    printf("mpu refuse load start=0 count=0 -> status=%d\n",
           (int)status);
    status = mpupointer_load(0, 32769, 0, MPUPOINTER_MOTOROLA,
                             &word);
    printf("mpu refuse load start=0 count=32769 -> status=%d\n",
           (int)status);
    status = mpupointer_load(0, 1, 2, MPUPOINTER_MOTOROLA, &word);
    printf("mpu refuse load start=0 count=1 reserved=2 -> "
           "status=%d\n", (int)status);
    status = mpupointer_load(0, 1, 0, 99, &word);
    printf("mpu refuse load start=0 count=1 order=bogus -> "
           "status=%d\n", (int)status);
    word = 0xAAAAAAAAu;
    status = mpupointer_adopt((uint64_t)4294967296ULL, &word);
    printf("mpu refuse adopt word=4294967296 -> status=%d\n",
           (int)status);
    if (word != 0xAAAAAAAAu) {
        return 1;
    }
    status = mpupointer_adopt((uint64_t)-1, &word);
    printf("mpu refuse adopt word=-1 -> status=%d\n", (int)status);
    status = mpupointer_unload(0, 99, &f);
    printf("mpu refuse unload order=bogus -> status=%d\n",
           (int)status);
    bijection_to_coordinate(32768, &c);
    status = mpupointer_load_coordinate(&c, 1, 0,
                                        MPUPOINTER_MOTOROLA,
                                        &word);
    printf("mpu refuse load_coordinate disp=32768 count=1 -> "
           "status=%d\n", (int)status);

    /* --- Nothing --- */
    nothing_range(&lo, &hi);
    printf("nothing range -> lo=%u hi=%u len=%u\n",
           (unsigned)lo, (unsigned)hi,
           (unsigned)(hi - lo + 1));
    {
        static const uint32_t reqs[] = {0, 7, 32768};
        for (i = 0; i < ARRAY_LEN(reqs); i++) {
            printf("nothing call request=%u -> %u\n",
                   (unsigned)reqs[i],
                   (unsigned)nothing_call(reqs[i]));
        }
    }
    present = 1;
    (void)nothing_search(&present);
    printf("nothing search -> %s\n", present ? "RESULT" : "None");

    /* --- Error --- */
    error_range(&lo, &hi);
    printf("error range -> lo=%u hi=%u len=%u\n",
           (unsigned)lo, (unsigned)hi,
           (unsigned)(hi - lo + 1));
    {
        uint32_t plane_out;
        uint32_t row_out;
        uint32_t sector_out;
        status = error_search(&plane_out, &row_out, &sector_out);
        printf("error search -> status=%d plane=%u row=%u "
               "sector=%u\n",
               (int)status, (unsigned)plane_out,
               (unsigned)row_out, (unsigned)sector_out);
    }

    return 0;
}
