/* Mode.h -- transcription target of Mode.py.
 *
 * A Mode is one named slice of the mode table, the
 * half-open range of request sizes [lo, hi) it answers,
 * together with the search C function that serves it.
 * Mode.py dispatches every need of every mode through
 * self.tuple(); in C the mode is a small struct: its
 * range bounds and its search function pointer. The
 * search signature is ModeSearchFn below, which is the
 * shape the Modes table stores and the Engine invokes,
 * exactly as the planes gear invokes its row handler.
 */

#ifndef ISCANDAR_MODE_H
#define ISCANDAR_MODE_H

#include <stdint.h>

#include "Gears.h"

/* The context a mode's search runs against: the cube's
 * three counters the Python modes reach through self, plus
 * the overall count CubeCheck's search reads. count_overall
 * is -1 when the caller did not supply it (the Python's
 * search(count_overall=None) case, where CubeCheck falls
 * back to summing the plane counts). */
typedef struct {
    const uint32_t *count_planes; /* [32] */
    const uint32_t *count_rows;   /* [1024] */
    const uint32_t *words;        /* [1024] sector words */
    int64_t         count_overall;
} ModeCtx;

/* A mode's search: the shape Mode.tuple() dispatches in
 * the Python -- (count_planes, count_rows, words, request)
 * in, (status, plane, row, sector) out, the counters and
 * words carried on the ModeCtx. */
typedef int32_t (*ModeSearchFn) (const ModeCtx *ctx,
                                 uint32_t request,
                                 uint32_t *plane_out,
                                 uint32_t *row_out,
                                 uint32_t *sector_out);

typedef struct {
    uint32_t     lo;     /* first size served */
    uint32_t     hi;     /* one past the last size served */
    ModeSearchFn search; /* this mode's search */
} Mode;

/* has_keying (Mode.has_keying): 1 when key lies in this
 * mode's range, else 0. */
int mode_has_keying(const Mode *mode, uint32_t key);

/* The base search pattern (Mode.search in Mode.py): the
 * planes gear over the cube, the rows row handler at each
 * plane, the mode's bullet as the probe, the sectors words
 * as the bullet's context -- Gears.scan in the Python. This
 * is the search Nano and Tiny inherit unchanged. Returns
 * the (status, plane, row, sector) tuple through the outs. */
int32_t mode_base_search(const ModeCtx *ctx, uint32_t request,
                         bullet_fn bullet,
                         uint32_t *plane_out, uint32_t *row_out,
                         uint32_t *sector_out);

#endif /* ISCANDAR_MODE_H */
