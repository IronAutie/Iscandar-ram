/* Large.h -- transcription of Large.py, the author's
 * design; the Python is the specification and this code
 * reproduces it exactly. Transcription only: no original
 * content. */

#ifndef ISCANDAR_LARGE_H
#define ISCANDAR_LARGE_H

#include <stdint.h>

/* class Large (Mode): the request mode for 65..1024
 * sectors (2A < L <= A^2), dispatched by the Modes table.
 * Rederived from requirements (Large.py's blackboard, items
 * 1 to 9, including the 2026-10-26 split-edge amendment):
 * Option A, consecutive full rows for the midsection, a
 * head and/or tail edge satisfying the remainder, and a
 * second pass at core - 1 blank rows when the first finds
 * no launching. Large.search in the Python is its own
 * plane driver over its own row driver over the per-start
 * probe -- no DLL gear -- and so is this transcription. */

/* Large.range(): inclusive(SECTORS_TWEEN_ROWS + 1,
 * SECTORS_PER_PLANE) -- 65..1024. Reported as the
 * inclusive bounds. */
__declspec(dllexport)
void large_range(uint32_t *lo_out, uint32_t *hi_out);

/* The two row-word oracles of Large.py, shared with Huge:
 * the free run ending at sector 31 (Right oracle), and the
 * free run starting at sector 0 (Left oracle). A word with
 * the edge sector occupied gives 0; a wholly clear word
 * gives 32. They cannot fail. */
__declspec(dllexport)
uint32_t large_trailing_run(uint32_t word);
__declspec(dllexport)
uint32_t large_leading_run(uint32_t word);

/* The bullet: __call__ for one core start, shaped like the
 * other modes' bullets. plane, row are the core start in
 * plane/row form; ctx is the sectors words array;
 * *sector_out takes the start sector on conviction
 * (GEAR_SUCCESS), and the full start is kept for
 * large_last_start, because a Large start can sit in the
 * head row, in a different plane from the core start, and
 * the bullet contract carries only one sector cell.
 * Acquittal is LARGE_FAIL. */
__declspec(dllexport)
int32_t large_bullet(uint32_t plane, uint32_t row,
                     uint32_t request, void *ctx,
                     uint32_t *sector_out);

/* The full start of the most recent large_bullet
 * conviction: (plane, row, sector). In the Python this is
 * the mode instance's last_start attribute; here it is
 * file state, in the manner of the gears' bound handler
 * state. Meaningful only immediately after a GEAR_SUCCESS
 * from large_bullet. */
__declspec(dllexport)
void large_last_start(uint32_t *plane_out, uint32_t *row_out,
                      uint32_t *sector_out);

/* Large.search: the plane driver's two passes. The return
 * is the uniform gear outcome: on GEAR_SUCCESS the start
 * (plane, row, sector); LARGE_FAIL when the request is
 * outside the mode's range; GEAR_EXHAUST when no launching
 * in either pass fits. */
__declspec(dllexport)
int32_t large_search(uint32_t request,
                     const uint32_t *count_planes,
                     const uint32_t *count_rows,
                     const uint32_t *words,
                     uint32_t *plane_out, uint32_t *row_out,
                     uint32_t *sector_out);

#endif /* ISCANDAR_LARGE_H */
