/* Small.h -- transcription of Small.py, the author's
 * design; the Python is the specification and this code
 * reproduces it exactly. Transcription only: no original
 * content. */

#ifndef ISCANDAR_SMALL_H
#define ISCANDAR_SMALL_H

#include <stdint.h>

/* class Small (Mode): "handles between rows requests" --
 * the request mode for 33..64 sectors, dispatched by the
 * Modes table. Its search runs the separate planes small
 * gear (PlaneSmallDriver.c), not the standard planes-over-
 * rows pattern, exactly as Small.search calls
 * Gears.scan_small in the Python. */

/* Small.range(): inclusive(SECTORS_PER_ROW + 1,
 * SECTORS_TWEEN_ROWS) -- 33..64. Reported as the inclusive
 * bounds. */
__declspec(dllexport)
void small_range(uint32_t *lo_out, uint32_t *hi_out);

/* The bullet: probe a pair of rows. The anchor is the
 * earlier row of the pair, the ancillary simply the next
 * word in ctx, even across a plane boundary; global row
 * 1023 has no later row and cannot be an anchor. ctx is
 * the sectors words array; *sector_of_anchor takes the
 * placement's start sector within the anchor row on
 * conviction (GEAR_SUCCESS). Acquittal is SMALL_FAIL.
 * The probe never marks. */
__declspec(dllexport)
int32_t small_bullet(uint32_t plane, uint32_t row,
                     uint32_t request, void *ctx,
                     uint32_t *sector_of_anchor);

/* Small.search: bind the bullet and fire the planes small
 * gear once. The return is the raw gear outcome: the
 * status, and on GEAR_SUCCESS the plane, row, and sector
 * of the conviction. */
__declspec(dllexport)
int32_t small_search(uint32_t request,
                     const uint32_t *count_planes,
                     const uint32_t *count_rows,
                     const uint32_t *words,
                     uint32_t *plane_out, uint32_t *row_out,
                     uint32_t *sector_out);

#endif /* ISCANDAR_SMALL_H */
