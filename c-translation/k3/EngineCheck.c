/* EngineCheck.c -- the admission driver for the C
 * transcription of the request modes and the Engine.
 *
 * It replays engine_ops.txt (written by
 * gen_engine_vectors.py, which also wrote
 * engine_vectors_expected.txt from the Python Engine) and
 * prints one result line per operation, in the expected
 * file's exact format. Admission is an empty diff:
 *
 *   cc -std=c99 -Wall -Wextra -include declspec.h \
 *      EngineCheck.c Engine.c Modes.c Mode.c Nano.c Tiny.c \
 *      Small.c Large.c Huge.c Cube.c Plane.c Coordinate.c \
 *      Bijection.c MPUPointer.c Nothing.c Error.c \
 *      Mark.c Unmark.c Commit.c RowsCheck.c PlanesCheck.c \
 *      PairsRowDriver.c PlaneSmallDriver.c -o EngineCheck
 *   ./EngineCheck engine_ops.txt > engine_vectors_actual.txt
 *   diff engine_vectors_expected.txt engine_vectors_actual.txt
 *
 * The gear sources linked here are the author's own C, the
 * same bodies the Python reaches through its DLL: that is
 * what carries the gears' cursor behavior (the planes
 * gear's sticky start, the small gear's rotate) into this
 * process exactly as the Python process sees it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Engine.h"
#include "Modes.h"
#include "MPUPointer.h"

/* Engine slots: the op script's `reset` seats a fresh
 * Engine in the next slot and makes it current; `use I`
 * makes slot I current again. The Python generator keeps
 * the same slots (its engines list), so interleaved use of
 * two live engines replays exactly. */
#define MAX_ENGINES 64
static Engine engines[MAX_ENGINES];
static int    engine_count = 0;
static Engine *engine_ptr = NULL;

#define engine (*engine_ptr)

static void print_fingerprint(void)
{
    uint64_t sum_rows = 0;
    uint64_t sum_planes = 0;
    uint64_t sum_words = 0;
    uint32_t i;

    for (i = 0; i < CUBE_ROWS_PER_CUBE; i++) {
        sum_rows  += engine.count_rows[i];
        sum_words += engine.cube.words[i];
    }
    for (i = 0; i < CUBE_PLANES_PER_CUBE; i++) {
        sum_planes += engine.count_planes[i];
    }
    printf(" | fp %llu %llu %llu %llu",
           (unsigned long long)engine.count_overall,
           (unsigned long long)sum_rows,
           (unsigned long long)sum_planes,
           (unsigned long long)sum_words);
}

static const char *mode_name(const Mode *mode)
{
    if (mode == modes_nothing()) {
        return "Nothing";
    }
    if (mode == modes_get(1)) {
        return "Nano";
    }
    if (mode == modes_get(9)) {
        return "Tiny";
    }
    if (mode == modes_get(33)) {
        return "Small";
    }
    if (mode == modes_get(65)) {
        return "Large";
    }
    if (mode == modes_get(1025)) {
        return "Huge";
    }
    return "CubeCheck";
}

int main(int argc, char **argv)
{
    FILE *fh;
    char  line[256];

    if (argc < 2) {
        fprintf(stderr, "usage: EngineCheck <ops file>\n");
        return 1;
    }
    fh = fopen(argv[1], "r");
    if (fh == NULL) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 1;
    }
    /* The script begins with `reset`; slot 0 is that first
     * reset's engine, exactly as the generator's engines[0]
     * is its first. */
    engine_ptr = &engines[0];
    engine_init(engine_ptr);
    engine_count = 0;
    while (fgets(line, sizeof line, fh) != NULL) {
        char          op[32];
        unsigned long ua = 0;
        unsigned long ub = 0;
        uint32_t      a;
        uint32_t      b;

        if (sscanf(line, "%31s", op) != 1) {
            continue;
        }
        if (strcmp(op, "reset") == 0) {
            if (engine_count >= MAX_ENGINES) {
                fprintf(stderr, "too many engines\n");
                fclose(fh);
                return 1;
            }
            engine_ptr = &engines[engine_count];
            engine_count += 1;
            engine_init(engine_ptr);
            printf("reset -> OK");
            print_fingerprint();
            printf("\n");
        } else if (strcmp(op, "use") == 0) {
            sscanf(line, "%*s %lu", &ua);
            engine_ptr = &engines[ua];
            printf("use %lu -> OK", ua);
            print_fingerprint();
            printf("\n");
        } else if (strcmp(op, "table") == 0) {
            sscanf(line, "%*s %lu", &ua);
            a = (uint32_t)ua;
            printf("table %lu -> %s", (unsigned long)a,
                   mode_name(modes_get(a)));
            print_fingerprint();
            printf("\n");
        } else if (strcmp(op, "call") == 0) {
            EngineResult result;

            sscanf(line, "%*s %lu", &ua);
            a = (uint32_t)ua;
            engine_call(&engine, a, &result);
            printf("call %lu -> ", (unsigned long)a);
            if (result.outcome == ENGINE_OUTCOME_POINTER) {
                printf("PTR %lu %lu %lu",
                       (unsigned long)mpupointer_start(result.word),
                       (unsigned long)mpupointer_count(result.word),
                       (unsigned long)result.word);
            } else if (result.outcome == ENGINE_OUTCOME_NONE) {
                printf("NONE");
            } else {
                printf("STATUS %ld", (long)result.status);
            }
            print_fingerprint();
            printf("\n");
        } else if (strcmp(op, "mark") == 0
                   || strcmp(op, "unmark") == 0) {
            uint32_t word;
            int32_t  status;

            sscanf(line, "%*s %lu %lu", &ua, &ub);
            a = (uint32_t)ua;
            b = (uint32_t)ub;
            if (mpupointer_load(a, b, 0, MPUPOINTER_MOTOROLA,
                                &word) != 0) {
                fprintf(stderr, "load refused: %s", line);
                fclose(fh);
                return 1;
            }
            if (strcmp(op, "mark") == 0) {
                status = engine_mark(&engine, word);
            } else {
                status = engine_unmark(&engine, word);
            }
            printf("%s %lu %lu -> STATUS %ld", op,
                   (unsigned long)a, (unsigned long)b,
                   (long)status);
            print_fingerprint();
            printf("\n");
        } else if (strcmp(op, "search") == 0) {
            const Mode *mode;
            ModeCtx     ctx;
            uint32_t    plane;
            uint32_t    row;
            uint32_t    sector;
            int32_t     status;

            sscanf(line, "%*s %lu %lu", &ua, &ub);
            a = (uint32_t)ua;
            b = (uint32_t)ub;
            mode = modes_get(a);
            printf("search %lu %lu -> ", (unsigned long)a,
                   (unsigned long)b);
            if (mode == modes_nothing()) {
                /* Nothing.search returns None in the Python. */
                printf("NONE");
            } else {
                ctx.count_planes  = engine.count_planes;
                ctx.count_rows    = engine.count_rows;
                ctx.words         = engine.cube.words;
                ctx.count_overall = (int64_t)engine.count_overall;
                status = mode->search(&ctx, b, &plane, &row,
                                      &sector);
                printf("TUPLE %ld %lu %lu %lu", (long)status,
                       (unsigned long)plane, (unsigned long)row,
                       (unsigned long)sector);
            }
            print_fingerprint();
            printf("\n");
        } else {
            fprintf(stderr, "unknown op: %s", line);
            fclose(fh);
            return 1;
        }
    }
    fclose(fh);
    return 0;
}
