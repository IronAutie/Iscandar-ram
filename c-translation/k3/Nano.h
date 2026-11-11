/* Nano.h -- transcription of Nano.py, the author's design;
 * the Python is the specification and this code reproduces
 * it exactly. Transcription only: no original content. */

#ifndef ISCANDAR_NANO_H
#define ISCANDAR_NANO_H

#include <stdint.h>

/* class Nano (Mode): "handles request inside bytes" -- the
 * request mode for 1..8 sectors, dispatched by the Modes
 * table. Its search is the base Mode pattern (the planes
 * gear over the rows gear, this bullet as the probe); that
 * composition lives in Mode.c. Only Nano's own bullet is
 * declared here. */

/* Nano.range(): inclusive(1, SECTORS_PER_BYTE) -- 1..8.
 * Reported as the inclusive bounds. */
__declspec(dllexport)
void nano_range(uint32_t *lo_out, uint32_t *hi_out);

/* The bullet: probe one row. ctx is the sectors words
 * array; *sector_out takes the run's start sector on
 * conviction (GEAR_SUCCESS). Acquittal is NANO_FAIL: the
 * global row past 1023, the request outside the mode's
 * range, or no clear run of the request's length in the
 * row. The probe never marks. */
__declspec(dllexport)
int32_t nano_bullet(uint32_t plane, uint32_t row,
                    uint32_t request, void *ctx,
                    uint32_t *sector_out);

#endif /* ISCANDAR_NANO_H */
