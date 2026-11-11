/* Gears.h -- declarations for the C gears the Python reaches
 * through Gears.py. The gear bodies already live in the
 * author's C: RowsCheck.c (rows gear), PlanesCheck.c (planes
 * gear and the rows_row_handler adapter), PairsRowDriver.c
 * (pairs_row_handler), PlaneSmallDriver.c (planes small
 * gear), Mark.c and Unmark.c (the commit path). This header
 * declares them; it defines nothing. It is the C twin of
 * Gears.py's binding block: same functions, same shapes.
 *
 * The gears carry state: the planes gear keeps a sticky
 * start cursor and the planes small gear a rotate cursor,
 * both function-local statics in the gear sources. Linking
 * the author's gear bodies unchanged is what carries that
 * behavior into this transcription exactly. */

#ifndef ISCANDAR_GEARS_H
#define ISCANDAR_GEARS_H

#include <stdint.h>

/* The bullet: a mode's trial. Returns GEAR_SUCCESS with
 * *sector_out set on conviction, or the mode's own failure
 * code on acquittal. The same typedef the gear sources use. */
typedef int32_t (*bullet_fn) (uint32_t plane, uint32_t row,
                              uint32_t request, void *ctx,
                              uint32_t *sector_out);

/* The row handler: fired at one plane; it receives the item
 * handler to pass down to the rows gear. */
typedef int32_t (*row_handler_fn) (const uint32_t *count_rows,
                                   uint32_t plane,
                                   uint32_t request,
                                   bullet_fn item_handler,
                                   void *ctx,
                                   uint32_t *sector_out,
                                   uint32_t *row_out);

/* RowsCheck.c */
__declspec(dllexport)
int32_t gear_abi_version (void);
__declspec(dllexport)
void rows_gear_bind (bullet_fn bullet, void *ctx);
__declspec(dllexport)
int32_t rows_gear (const uint32_t *count_rows, uint32_t plane,
                   uint32_t request, uint32_t *sector_out,
                   uint32_t *row_out);

/* PlanesCheck.c */
__declspec(dllexport)
int32_t rows_row_handler (const uint32_t *count_rows,
                          uint32_t plane, uint32_t request,
                          bullet_fn item_handler, void *ctx,
                          uint32_t *sector_out,
                          uint32_t *row_out);
__declspec(dllexport)
int32_t planes_gear (const uint32_t *count_planes,
                     const uint32_t *count_rows, uint32_t request,
                     row_handler_fn row_handler,
                     bullet_fn item_handler, void *ctx,
                     uint32_t *sector_out, uint32_t *row_out,
                     uint32_t *plane_out);

/* PairsRowDriver.c: the per-plane pair handler the planes
 * small gear fires. (pairs_gear and pairs_gear_bind are the
 * standalone global pairs gear; Small.search does not use
 * them, and neither does this transcription.) */
__declspec(dllexport)
int32_t pairs_row_handler (const uint32_t *count_rows,
                           uint32_t plane, uint32_t request,
                           bullet_fn item_handler, void *ctx,
                           uint32_t *sector_of_anchor,
                           uint32_t *row_out);

/* PlaneSmallDriver.c */
__declspec(dllexport)
void planes_small_gear_bind (bullet_fn bullet, void *ctx);
__declspec(dllexport)
int32_t planes_small_gear (const uint32_t *count_planes,
                           const uint32_t *count_rows,
                           uint32_t request,
                           uint32_t *sector_of_anchor,
                           uint32_t *row_out,
                           uint32_t *plane_out);

/* Mark.c / Unmark.c: the commit path. Success is
 * GEAR_SUCCESS; the only failure is check-then-set,
 * FAIL_OVERLAY. The helper owns the sectors array and the
 * row and plane counters; the overall counter is kept by
 * the caller (Python today, the C Engine in this
 * transcription), exactly as Mark.c's own header states.
 * (Gears.py passes one extra trailing argument, the
 * overall counter's address, which the C ignores.) */
__declspec(dllexport)
int32_t mark_sectors (uint32_t *sectors, uint32_t *count_rows,
                      uint32_t *count_planes, uint32_t start,
                      uint32_t count);
__declspec(dllexport)
int32_t unmark_sectors (uint32_t *sectors, uint32_t *count_rows,
                        uint32_t *count_planes, uint32_t start,
                        uint32_t count);

#endif /* ISCANDAR_GEARS_H */
