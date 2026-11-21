/* check_mass.c -- the desk check for the marked mass.
 *
 * Proves, on this VM and without booting, the five claims
 * in MASS-LAYOUT.md:
 *   1. the emitted mass is self-consistent (marks summed
 *      against counters, the fixture set exact, everything
 *      else zero);
 *   2. it is byte-identical to the state built the runtime
 *      way (beam_init plus the fixture runs marked through
 *      the engine);
 *   3. seated as the Beam, it behaves identically to that
 *      reference under a probe battery, and returns to its
 *      born state when the battery's grants are freed;
 *   4. the witness word reads the same by byte offset and
 *      by the struct path (the init canary's shape);
 *   5. the section sizes are as laid out (checked by the
 *      runner with readelf, not here).
 *
 * Modes: struct (checks 1 and 4), a (battery on the mass),
 * b (reference construction, check 2, battery on it).
 * Modes a and b run in separate processes: the gears keep
 * their cursors in process-global statics, so one process
 * cannot fairly run both batteries (the atomicity
 * simulation's finding). The runner diffs the batteries.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Beam.h"
#include "BeamCoordinate.h"
#include "CPUPointer.h"
#include "Engine.h"
#include "MPUPointer.h"
#include "K4Status.h"
#include "status.h"

extern char iscandar_mass_start[];
extern char iscandar_beam[];
extern char iscandar_ledger[];
extern char iscandar_registry[];
extern char iscandar_mass_end[];
extern char iscandar_arena_start[];
extern char iscandar_arena_end[];

#define BEAM_BYTES   266512
#define LEDGER_BYTES 262144
#define REG_BYTES    32768
#define MASS_BYTES   (BEAM_BYTES + LEDGER_BYTES + REG_BYTES)
#define ARENA_BYTES  536870912
#define CUBES        4
#define FIXTURE_C0   27
#define FIXTURE_CN   19
#define FIXTURE_SUM  84

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("check FAIL: %s\n", msg); failures++; } \
} while (0)

static uint32_t popcount32(uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0F0F0F0Fu;
    return (x * 0x01010101u) >> 24;
}

static void check_struct(void)
{
    Beam *beam = (Beam *)iscandar_beam;
    int c, r, p, s;

    CHECK((size_t)(iscandar_mass_end - iscandar_mass_start)
          == MASS_BYTES, "mass size");
    CHECK((size_t)(iscandar_arena_end - iscandar_arena_start)
          == ARENA_BYTES, "arena size");
    CHECK(iscandar_ledger - iscandar_beam == BEAM_BYTES,
          "ledger offset");
    CHECK(iscandar_registry - iscandar_ledger == LEDGER_BYTES,
          "registry offset");
    CHECK(beam->cardinality == CUBES, "cardinality");
    CHECK(beam->count_overall == FIXTURE_SUM, "beam count_overall");
    CHECK(beam->sectors_supported == 131072, "sectors_supported");
    CHECK(beam->lead == 0, "lead");

    for (c = 0; c < CUBES; c++) {
        Engine *e = &beam->engines[c].engine;
        uint32_t run = (c == 0) ? FIXTURE_C0 : FIXTURE_CN;
        uint32_t marked = 0, sum_rows = 0, sum_planes = 0;
        for (r = 0; r < 1024; r++) {
            marked += popcount32(e->cube.words[r]);
            sum_rows += e->count_rows[r];
        }
        for (p = 0; p < 32; p++)
            sum_planes += e->count_planes[p];
        CHECK(marked == run, "popcount vs fixture run");
        CHECK(sum_rows == run, "row counters vs fixture run");
        CHECK(sum_planes == run, "plane counters vs fixture run");
        CHECK(e->count_overall == run, "engine overall");
        CHECK(beam->engines[c].count_overall == run, "mirror overall");
        /* the marked set is exactly sectors 0..run-1 */
        for (s = 0; s < 32768; s++) {
            int bit = (int)((e->cube.words[s / 32] >> (s % 32)) & 1u);
            int want = (s < (int)run) ? 1 : 0;
            if (bit != want) {
                CHECK(0, "marked set differs from the fixture set");
                break;
            }
        }
    }
    for (c = CUBES; c < 32; c++) {
        const uint32_t *slot = (const uint32_t *)&beam->engines[c];
        int i, nz = 0;
        for (i = 0; i < 8328 / 4; i++)
            if (slot[i] != 0) nz = 1;
        CHECK(!nz, "slot above cardinality not zero");
    }
    {
        const unsigned char *z;
        size_t i;
        int nz = 0;
        z = (const unsigned char *)iscandar_ledger;
        for (i = 0; i < LEDGER_BYTES; i++) if (z[i]) nz = 1;
        CHECK(!nz, "ledger not born zero");
        z = (const unsigned char *)iscandar_registry;
        for (i = 0; i < REG_BYTES; i++) if (z[i]) nz = 1;
        CHECK(!nz, "registry not born zero");
    }

    /* check 4: the witness word, two paths */
    {
        uint32_t by_offset = *(const uint32_t *)(iscandar_mass_start + 16);
        uint32_t by_struct = beam->engines[0].engine.cube.words[0];
        CHECK(by_offset == by_struct, "witness word, two paths differ");
        CHECK(by_struct == 0x07FFFFFFu, "witness word value");
    }
    printf("check struct: %s\n", failures ? "FAIL" : "ok");
}

static const int32_t battery[] = {1, 5, 8, 32, 64, 300, 1024, 5000};
#define NB (sizeof battery / sizeof battery[0])

static void run_battery(Beam *beam, const char *tag)
{
    static unsigned char born[BEAM_BYTES];
    CPUPointer held[NB];
    int32_t    got[NB];
    unsigned   i;

    memcpy(born, beam, BEAM_BYTES);
    for (i = 0; i < NB; i++) {
        int32_t st = beam_allocate(beam, battery[i], &held[i]);
        got[i] = (st == K4_SUCCESS);
        if (got[i]) {
            BeamCoordinate coord;
            K3Coordinate   local;
            if (cpu_pointer_coordinate(&held[i], K4_ORDER_MOTOROLA,
                                       &coord) != K4_SUCCESS) {
                printf("grant %ld -> DECODE FAIL\n", (long)battery[i]);
                failures++;
                continue;
            }
            beam_coordinate_k3(&coord, &local);
            printf("grant %ld -> cube %ld start %ld count %lu\n",
                   (long)battery[i], (long)coord.cube,
                   (long)k3_coordinate_displacement(&local),
                   (unsigned long)cpu_pointer_count(&held[i]));
        } else {
            printf("grant %ld -> FAIL\n", (long)battery[i]);
        }
    }
    for (i = NB; i-- > 0; ) {
        if (got[i]) {
            int32_t st = beam_free(beam, &held[i]);
            printf("free %ld -> %s\n", (long)battery[i],
                   st == K4_GEAR_SUCCESS ? "ok" : "STATUS");
            if (st != K4_GEAR_SUCCESS) failures++;
        }
    }
    CHECK(memcmp(born, beam, BEAM_BYTES) == 0,
          "state did not return to born after the battery");
    printf("check battery (%s): %s\n", tag,
           failures ? "FAIL" : "ok");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: check_mass struct|a|b\n");
        return 1;
    }
    check_struct();
    if (strcmp(argv[1], "a") == 0) {
        run_battery((Beam *)iscandar_beam, "mass");
    } else if (strcmp(argv[1], "b") == 0) {
        static Beam ref;
        int c;
        if (beam_init(&ref, CUBES) != K4_SUCCESS) {
            fprintf(stderr, "beam_init failed\n");
            return 1;
        }
        for (c = 0; c < CUBES; c++) {
            uint32_t word;
            uint32_t run = (c == 0) ? FIXTURE_C0 : FIXTURE_CN;
            if (mpupointer_load(0, run, 0, MPUPOINTER_MOTOROLA,
                                &word) != 0) {
                fprintf(stderr, "load refused\n");
                return 1;
            }
            if (engine_mark(&ref.engines[c].engine, word)
                != GEAR_SUCCESS) {
                fprintf(stderr, "fixture mark failed, cube %d\n", c);
                return 1;
            }
            ref.engines[c].count_overall =
                ref.engines[c].engine.count_overall;
        }
        ref.count_overall = FIXTURE_SUM;
        CHECK(memcmp(&ref, iscandar_beam, BEAM_BYTES) == 0,
              "mass differs from the constructed reference state");
        printf("check identity: %s\n", failures ? "FAIL" : "ok");
        run_battery(&ref, "reference");
    } else if (strcmp(argv[1], "struct") != 0) {
        fprintf(stderr, "unknown mode %s\n", argv[1]);
        return 1;
    }
    return failures ? 1 : 0;
}
