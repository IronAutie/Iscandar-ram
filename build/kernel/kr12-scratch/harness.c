/* harness.c -- desk-check driver for patch 0013 (scratch
 * only, not part of the patch). Runs one fixed op script
 * against a 4-cube Beam and prints, per op, the decision
 * the engine made: status, and for grants the pointer's
 * start and count. Built twice, against the pristine
 * objects and against the patched objects; the decision
 * columns must be identical. The patched build also
 * prints iscandar_beam_last_steps per op (STEPS). */

#include <stdio.h>
#include <stdint.h>

#include "Beam.h"
#include "CPUPointer.h"
#include "K4Status.h"

#ifdef STEPS
extern uint64_t iscandar_beam_last_steps;
#endif

static Beam beam;
static CPUPointer held[64];
static int nheld;

static void do_alloc(int32_t request)
{
    CPUPointer ptr;
    int32_t st = beam_allocate(&beam, request, &ptr);

    if (st == K4_SUCCESS) {
        held[nheld++] = ptr;
        printf("alloc %6d -> SUCCESS start %6u count %6u",
               request, cpu_pointer_start(&ptr),
               cpu_pointer_count(&ptr));
    } else {
        printf("alloc %6d -> status %d", request, st);
    }
#ifdef STEPS
    printf(" steps %llu", (unsigned long long)iscandar_beam_last_steps);
#endif
    printf("\n");
}

static void do_free(int slot)
{
    int32_t st = beam_free(&beam, &held[slot]);

    printf("free  slot %2d -> status %d", slot, st);
#ifdef STEPS
    printf(" steps %llu", (unsigned long long)iscandar_beam_last_steps);
#endif
    printf("\n");
}

int main(void)
{
    static const int32_t script[] = {
        1, 8, 9, 32, 33, 64, 65, 1024, 1025, 5000,
        3, 17, 100, 2000, 32767, 32768, 40000, -5, 0
    };
    unsigned i;
    int slot;

    if (beam_init(&beam, 4) != K4_SUCCESS) {
        printf("beam_init failed\n");
        return 1;
    }
    for (i = 0; i < sizeof script / sizeof script[0]; i++)
        do_alloc(script[i]);
    /* free every other held block, then the rest */
    for (slot = 0; slot < nheld; slot += 2)
        do_free(slot);
    do_alloc(32768);            /* CubeCheck on a partly-full beam */
    do_alloc(2048);
    do_alloc(31);
    for (slot = 1; slot < nheld; slot += 2)
        do_free(slot);
    /* drain whatever the late allocs added */
    while (nheld > 0) {
        nheld--;
        do_free(nheld);
    }
    do_alloc(32768);            /* CubeCheck on an empty beam */
    do_alloc(1);
    printf("final count_overall %d lead %d\n",
           beam.count_overall, beam.lead);
    return 0;
}
