/* Tiny.h -- transcription of Tiny.py, the author's design;
 * the Python is the specification and this code reproduces
 * it exactly. Transcription only: no original content. */

#ifndef ISCANDAR_TINY_H
#define ISCANDAR_TINY_H

#include <stdint.h>

/* class Tiny (Mode): "handles request inside row" -- the
 * request mode for 9..32 sectors, dispatched by the Modes
 * table. Its search is the base Mode pattern; that
 * composition lives in Mode.c. Only Tiny's own bullet is
 * declared here. */

/* Tiny.range(): inclusive(SECTORS_PER_BYTE + 1,
 * SECTORS_PER_ROW) -- 9..32. Reported as the inclusive
 * bounds. */
__declspec(dllexport)
void tiny_range(uint32_t *lo_out, uint32_t *hi_out);

/* The bullet: probe one row, two passes. Left pass first:
 * the first (lowest-start) clear run of at least 9 sectors,
 * placed at its start when it reaches the request. Right
 * pass as the second bite: the tail run ending at sector
 * 31, placed ending at sector 31 when it reaches the
 * request. ctx is the sectors words array. Conviction is
 * GEAR_SUCCESS with *sector_out set; acquittal is
 * TINY_FAIL. The probe never marks. */
__declspec(dllexport)
int32_t tiny_bullet(uint32_t plane, uint32_t row,
                    uint32_t request, void *ctx,
                    uint32_t *sector_out);

#endif /* ISCANDAR_TINY_H */
