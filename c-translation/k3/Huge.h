/* Huge.h -- transcription of Huge.py, the author's
 * design; the Python is the specification and this code
 * reproduces it exactly. Transcription only: no original
 * content. */

#ifndef ISCANDAR_HUGE_H
#define ISCANDAR_HUGE_H

#include <stdint.h>

/* class Huge (Mode): the request mode for 1025..32767
 * sectors (A^2 < L <= A^3 - 1), dispatched by the Modes
 * table. Rederived from requirements (Huge.py's blackboard,
 * 53 facts, plus the 2026-10-26 split-edge second sweep):
 * core planes counted first, then a leading and a trailing
 * plane; three row handlers (no leading, no trailing,
 * normal) fire per candidate position, and named failure
 * codes mark the exact acquittal site for breakpointing.
 * Huge.search in the Python is its own plane driver over
 * those handlers -- no DLL gear -- and so is this
 * transcription. */

/* The bullet context of the Python: the tuple
 * (count_planes, count_rows, words) that Huge.__call__ and
 * the split bullet unpack. */
typedef struct {
    const uint32_t *count_planes;
    const uint32_t *count_rows;
    const uint32_t *words;
} HugeCtx;

/* Huge.range(): inclusive(SECTORS_PER_PLANE + 1,
 * SECTORS_PER_CUBE - 1) -- 1025..32767. Reported as the
 * inclusive bounds. */
__declspec(dllexport)
void huge_range(uint32_t *lo_out, uint32_t *hi_out);

/* The bullet: __call__ for one candidate. plane is the
 * candidate's first core plane; row is unused, because a
 * core candidate is a plane. ctx is a HugeCtx. On
 * GEAR_SUCCESS *sector_out takes the start sector within
 * the start row and the full start is kept for
 * huge_last_start; on failure the handler's named status
 * is returned, for breakpointing. */
__declspec(dllexport)
int32_t huge_bullet(uint32_t plane, uint32_t row,
                    uint32_t request, void *ctx,
                    uint32_t *sector_out);

/* The full start of the most recent huge_bullet or split
 * conviction: (plane, row, sector). In the Python this is
 * the mode instance's last_start attribute; here it is
 * file state, in the manner of the gears' bound handler
 * state. Meaningful only immediately after a GEAR_SUCCESS
 * that set it. */
__declspec(dllexport)
void huge_last_start(uint32_t *plane_out, uint32_t *row_out,
                     uint32_t *sector_out);

/* Huge.search: the plane driver's two sweeps. The return
 * is the uniform gear outcome: on GEAR_SUCCESS the start
 * (plane, row, sector); HUGE_FAIL when the request is
 * outside the mode's range; GEAR_EXHAUST when no candidate
 * in either sweep fits. */
__declspec(dllexport)
int32_t huge_search(uint32_t request,
                    const uint32_t *count_planes,
                    const uint32_t *count_rows,
                    const uint32_t *words,
                    uint32_t *plane_out, uint32_t *row_out,
                    uint32_t *sector_out);

#endif /* ISCANDAR_HUGE_H */
