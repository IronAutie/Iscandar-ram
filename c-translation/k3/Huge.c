/* Huge.c -- see Huge.h. Transcription of Huge.py; the
 * Python is the specification. */

#include "Huge.h"

#include "Cube.h"
#include "Geometry.h"
#include "Large.h"
#include "Plane.h"
#include "status.h"

/* The most recent conviction's full start, (plane, row,
 * sector) -- self.last_start in the Python. */
static uint32_t last_plane;
static uint32_t last_row;
static uint32_t last_sector;

void huge_range(uint32_t *lo_out, uint32_t *hi_out)
{
    *lo_out = PLANE_SECTORS_PER_PLANE + 1;
    *hi_out = CUBE_SECTORS_PER_CUBE - 1;
}

static int huge_in_range(uint32_t request)
{
    return request >= PLANE_SECTORS_PER_PLANE + 1
        && request <= CUBE_SECTORS_PER_CUBE - 1;
}

/* _lead_check_step: one step of the lead analysis. The
 * pointer has already been decremented by the caller;
 * verify the row count at it is zero. */
static int32_t huge_lead_check_step(int32_t pointer,
                                    const uint32_t *count_rows)
{
    if (count_rows[pointer] != 0) {
        return HUGE_NO_TRAILER_ROW_FAIL;
    }
    return GEAR_SUCCESS;
}

/* _trailing_row_body: one call of the normal driver's
 * trailing body. Check the row count is zero. */
static int32_t huge_trailing_row_body(int32_t pointer,
                                      const uint32_t *count_rows)
{
    if (count_rows[pointer] != 0) {
        return HUGE_ROW_DRIVER_TRAILING_ROW_FAIL;
    }
    return GEAR_SUCCESS;
}

/* _span_free: sector-exact recheck of a proposed span, so
 * a start is never returned on a handler's arithmetic
 * alone. Returns 1 only when every sector is free. */
static int huge_span_free(int64_t start, uint32_t request,
                          const uint32_t *words)
{
    int64_t end;
    int32_t first_row;
    int32_t last_row;
    int32_t g;
    int32_t lo;
    int32_t hi;
    int32_t sector;
    uint32_t word;

    end = start + (int64_t)request;
    if (start < 0 || end > CUBE_SECTORS_PER_CUBE) {
        return 0;
    }
    first_row = (int32_t)(start / GEOMETRY_ARITY);
    last_row  = (int32_t)((end - 1) / GEOMETRY_ARITY);
    for (g = first_row; g <= last_row; g++) {
        word = words[g];
        if (word == 0) {
            continue;
        }
        lo = (g == first_row)
             ? (int32_t)(start - (int64_t)g * GEOMETRY_ARITY)
             : 0;
        hi = (g == last_row)
             ? (int32_t)(end - (int64_t)g * GEOMETRY_ARITY - 1)
             : GEOMETRY_ARITY - 1;
        for (sector = lo; sector <= hi; sector++) {
            if ((word >> sector) & 1u) {
                return 0;
            }
        }
    }
    return 1;
}

/* _rows_no_leading: the remainder lives in the plane
 * following the core: trailing rows first, then one
 * oracle row. Returns a status; on GEAR_SUCCESS
 * *start_out is the global sector index of the start. */
static int32_t huge_rows_no_leading(uint32_t candidate,
                                    uint32_t request,
                                    const uint32_t *count_planes,
                                    const uint32_t *count_rows,
                                    const uint32_t *words,
                                    int64_t *start_out)
{
    uint32_t core;
    uint32_t remaining;
    uint32_t following;
    uint32_t trailing_rows;
    uint32_t base;
    uint32_t step;
    uint32_t row;

    (void)words;
    core      = request / (GEOMETRY_ARITY * GEOMETRY_ARITY);
    remaining = request - core * PLANE_SECTORS_PER_PLANE;
    if (remaining == 0) {
        *start_out = (int64_t)candidate * PLANE_SECTORS_PER_PLANE;
        return GEAR_SUCCESS;
    }
    following = candidate + core;
    if (following >= CUBE_PLANES_PER_CUBE) {
        return HUGE_FAIL;
    }
    /* Gate: if the plane following cannot satisfy the
     * remaining, this named gate fails. */
    if (PLANE_SECTORS_PER_PLANE - count_planes[following]
            < remaining) {
        return HUGE_GATE_TRAILING_PLANE_GATE_FAIL;
    }
    /* Rows: every required full row must have a zero
     * count. */
    trailing_rows = remaining / GEOMETRY_ARITY;
    base = following * GEOMETRY_ARITY;
    for (step = 0; step < trailing_rows; step++) {
        if (count_rows[base + step] != 0) {
            return HUGE_TRAILING_ROW_GATE_FAIL;
        }
    }
    remaining = remaining % GEOMETRY_ARITY;
    /* Oracle: the following row must consult the Left
     * oracle for a leading series fitting the rest. */
    if (remaining) {
        row = base + trailing_rows;
        if (row >= CUBE_ROWS_PER_CUBE
                || large_leading_run(words[row]) < remaining) {
            return HUGE_NO_LEAD_TRAILER_FAIL;
        }
    }
    *start_out = (int64_t)candidate * PLANE_SECTORS_PER_PLANE;
    return GEAR_SUCCESS;
}

/* _rows_no_trailing: works backwards from the core; the
 * remainder lives in the single plane before the core,
 * reversed into: lead padding rows, then one candidate
 * row. */
static int32_t huge_rows_no_trailing(uint32_t candidate,
                                     uint32_t request,
                                     const uint32_t *count_planes,
                                     const uint32_t *count_rows,
                                     const uint32_t *words,
                                     int64_t *start_out)
{
    uint32_t core;
    uint32_t remaining;
    uint32_t lead_rows;
    uint32_t lead_sectors;
    int32_t  pointer;
    uint32_t step;
    int32_t  status;

    (void)count_planes;
    core         = request / (GEOMETRY_ARITY * GEOMETRY_ARITY);
    remaining    = request - core * PLANE_SECTORS_PER_PLANE;
    if (remaining == 0) {
        *start_out = (int64_t)candidate * PLANE_SECTORS_PER_PLANE;
        return GEAR_SUCCESS;
    }
    lead_rows    = remaining / GEOMETRY_ARITY;
    lead_sectors = remaining % GEOMETRY_ARITY;
    pointer = (int32_t)(candidate * GEOMETRY_ARITY);
    for (step = 0; step < lead_rows; step++) {
        pointer -= 1;
        status = huge_lead_check_step(pointer, count_rows);
        if (status != GEAR_SUCCESS) {
            return status;
        }
    }
    if (lead_sectors) {
        /* One more decrement: the candidate row's free run
         * at its end must cover the leading sectors still
         * owed. */
        pointer -= 1;
        if (pointer < 0
                || large_trailing_run(words[pointer])
                       < lead_sectors) {
            return HUGE_FAIL;
        }
    }
    *start_out = (int64_t)candidate * PLANE_SECTORS_PER_PLANE
                 - (int64_t)remaining;
    return GEAR_SUCCESS;
}

/* _rows_normal: leader first -- the leading plane gives
 * what its free suffix has, and the trailing plane
 * fulfils the rest, mandatorily. */
static int32_t huge_rows_normal(uint32_t candidate,
                                uint32_t request,
                                const uint32_t *count_planes,
                                const uint32_t *count_rows,
                                const uint32_t *words,
                                int64_t *start_out)
{
    uint32_t core;
    uint32_t remaining;
    int32_t  pointer;
    int32_t  plane_floor;
    uint32_t leader_take;
    int64_t  start;
    uint32_t still_owed;
    uint32_t following;
    uint32_t row;
    uint32_t trailing_rows;
    uint32_t step;
    int32_t  status;

    (void)count_planes;
    core      = request / (GEOMETRY_ARITY * GEOMETRY_ARITY);
    remaining = request - core * PLANE_SECTORS_PER_PLANE;
    if (remaining == 0) {
        *start_out = (int64_t)candidate * PLANE_SECTORS_PER_PLANE;
        return GEAR_SUCCESS;
    }

    /* Leader calculation. Row pointer set to the start of
     * the core plane; each step decrements it: a zero-count
     * row counts a full row toward the take, and the first
     * non-zero row gives its end run and stops the walk.
     * The walk never leaves the leading plane. */
    pointer     = (int32_t)(candidate * GEOMETRY_ARITY);
    plane_floor = pointer - GEOMETRY_ARITY;
    leader_take = 0;
    while (leader_take < remaining && pointer > plane_floor) {
        pointer -= 1;
        if (count_rows[pointer] == 0) {
            leader_take += GEOMETRY_ARITY;
        } else {
            leader_take += large_trailing_run(words[pointer]);
            break;
        }
    }
    if (leader_take > remaining) {
        leader_take = remaining;
    }
    start      = (int64_t)candidate * PLANE_SECTORS_PER_PLANE
                 - (int64_t)leader_take;
    still_owed = remaining - leader_take;
    if (still_owed == 0) {
        *start_out = start;
        return GEAR_SUCCESS;
    }

    /* Trailing padding: each required full row after the
     * core planes must have a zero count; then one more
     * row is evaluated with the Left oracle for the
     * sectors still owed. */
    following     = candidate + core;
    row           = following * GEOMETRY_ARITY;
    trailing_rows = still_owed / GEOMETRY_ARITY;
    for (step = 0; step < trailing_rows; step++) {
        status = huge_trailing_row_body((int32_t)(row + step),
                                        count_rows);
        if (status != GEAR_SUCCESS) {
            return status;
        }
    }
    still_owed = still_owed % GEOMETRY_ARITY;
    if (still_owed) {
        row += trailing_rows;
        if (row >= CUBE_ROWS_PER_CUBE
                || large_leading_run(words[row]) < still_owed) {
            return HUGE_ROW_DRIVER_TRAILER_PADDING_FAIL;
        }
    }
    *start_out = start;
    return GEAR_SUCCESS;
}

/* _plane_suffix_take: what a plane can give as a leader:
 * its free suffix, in sectors. At most one plane. */
static uint32_t huge_plane_suffix_take(uint32_t plane,
                                       const uint32_t *count_rows,
                                       const uint32_t *words)
{
    uint32_t take = 0;
    int32_t  row;
    int32_t  floor_row;

    row       = (int32_t)((plane + 1) * GEOMETRY_ARITY - 1);
    floor_row = (int32_t)(plane * GEOMETRY_ARITY);
    while (row >= floor_row) {
        if (count_rows[row] == 0) {
            take += GEOMETRY_ARITY;
            row  -= 1;
        } else {
            take += large_trailing_run(words[row]);
            break;
        }
    }
    return take;
}

/* _plane_prefix_take: what a plane can give as a trailer:
 * its free prefix, in sectors. At most one plane. */
static uint32_t huge_plane_prefix_take(uint32_t plane,
                                       const uint32_t *count_rows,
                                       const uint32_t *words)
{
    uint32_t take = 0;
    uint32_t row;
    uint32_t ceil_row;

    row      = plane * GEOMETRY_ARITY;
    ceil_row = (plane + 1) * GEOMETRY_ARITY;
    while (row < ceil_row) {
        if (count_rows[row] == 0) {
            take += GEOMETRY_ARITY;
            row  += 1;
        } else {
            take += large_leading_run(words[row]);
            break;
        }
    }
    return take;
}

/* _split_attempt: one split candidate -- core - 1 empty
 * planes starting at seam, flanked by both edge planes.
 * The edges together must cover the need, remaining plus
 * one plane; the leader is taken first, the trailer must
 * fulfil the rest. Returns 1 with *start_out set, else 0. */
static int huge_split_attempt(uint32_t seam, uint32_t request,
                              const uint32_t *count_planes,
                              const uint32_t *count_rows,
                              const uint32_t *words,
                              int64_t *start_out)
{
    uint32_t core;
    uint32_t width;
    uint32_t remaining;
    uint32_t follower;
    uint32_t need;
    uint32_t leader_take;
    uint32_t still_owed;
    uint32_t trailer_take;

    core      = request / (GEOMETRY_ARITY * GEOMETRY_ARITY);
    width     = core - 1;
    remaining = request % (GEOMETRY_ARITY * GEOMETRY_ARITY);
    for (follower = seam; follower < seam + width; follower++) {
        if (count_planes[follower] != 0) {
            return 0;
        }
    }
    need        = remaining + PLANE_SECTORS_PER_PLANE;
    leader_take = huge_plane_suffix_take(seam - 1, count_rows,
                                         words);
    if (leader_take > need) {
        leader_take = need;
    }
    still_owed = need - leader_take;
    if (still_owed > 0) {
        trailer_take = huge_plane_prefix_take(seam + width,
                                              count_rows, words);
        if (trailer_take < still_owed) {
            return 0;
        }
    }
    *start_out = (int64_t)seam * PLANE_SECTORS_PER_PLANE
                 - (int64_t)leader_take;
    return 1;
}

/* _rows_split: the split handler, a thin wrapper over
 * _split_attempt so the named failure has the one return
 * site the status discipline wants. */
static int32_t huge_rows_split(uint32_t seam, uint32_t request,
                               const uint32_t *count_planes,
                               const uint32_t *count_rows,
                               const uint32_t *words,
                               int64_t *start_out)
{
    if (!huge_split_attempt(seam, request, count_planes,
                            count_rows, words, start_out)) {
        return HUGE_SPLIT_FAIL;
    }
    return GEAR_SUCCESS;
}

/* Record a convicted global start in the last-start state
 * and in the bullet's sector cell, as both Huge bullets
 * do in the Python. */
static int32_t huge_convict(int64_t start, uint32_t *sector_out)
{
    last_plane  = (uint32_t)(start / PLANE_SECTORS_PER_PLANE);
    last_row    = (uint32_t)((start % PLANE_SECTORS_PER_PLANE)
                             / GEOMETRY_ARITY);
    last_sector = (uint32_t)(start % GEOMETRY_ARITY);
    *sector_out = last_sector;
    return GEAR_SUCCESS;
}

/* _split_call: the bullet for one split candidate. seam
 * is the first of the core - 1 empty planes (for core = 1,
 * the trailer plane of the boundary). */
static int32_t huge_split_call(uint32_t seam, uint32_t request,
                               const HugeCtx *ctx,
                               uint32_t *sector_out)
{
    uint32_t core;
    int32_t  status;
    int64_t  start;

    if (!huge_in_range(request)) {
        return HUGE_SPLIT_FAIL;
    }
    core = request / (GEOMETRY_ARITY * GEOMETRY_ARITY);
    if (seam < 1 || seam + core - 1 > CUBE_PLANES_PER_CUBE - 1) {
        return HUGE_SPLIT_FAIL;
    }
    status = huge_rows_split(seam, request, ctx->count_planes,
                             ctx->count_rows, ctx->words, &start);
    if (status != GEAR_SUCCESS) {
        return status;
    }
    if (!huge_span_free(start, request, ctx->words)) {
        return HUGE_FAIL;
    }
    return huge_convict(start, sector_out);
}

int32_t huge_bullet(uint32_t plane, uint32_t row,
                    uint32_t request, void *ctx,
                    uint32_t *sector_out)
{
    const HugeCtx *hctx = (const HugeCtx *)ctx;
    uint32_t core;
    uint32_t follower;
    int32_t  status;
    int64_t  start;

    (void)row;
    if (!huge_in_range(request)) {
        return HUGE_FAIL;
    }
    core = request / (GEOMETRY_ARITY * GEOMETRY_ARITY);
    if (plane + core > CUBE_PLANES_PER_CUBE) {
        return HUGE_FAIL;
    }
    for (follower = plane; follower < plane + core; follower++) {
        if (hctx->count_planes[follower] != 0) {
            return HUGE_FAIL;
        }
    }
    if (plane == 0) {
        status = huge_rows_no_leading(plane, request,
                                      hctx->count_planes,
                                      hctx->count_rows,
                                      hctx->words, &start);
    } else if (plane + core - 1 == CUBE_PLANES_PER_CUBE - 1) {
        status = huge_rows_no_trailing(plane, request,
                                       hctx->count_planes,
                                       hctx->count_rows,
                                       hctx->words, &start);
    } else {
        status = huge_rows_normal(plane, request,
                                  hctx->count_planes,
                                  hctx->count_rows,
                                  hctx->words, &start);
    }
    if (status != GEAR_SUCCESS) {
        return status;
    }
    if (!huge_span_free(start, request, hctx->words)) {
        return HUGE_FAIL;
    }
    return huge_convict(start, sector_out);
}

void huge_last_start(uint32_t *plane_out, uint32_t *row_out,
                     uint32_t *sector_out)
{
    *plane_out  = last_plane;
    *row_out    = last_row;
    *sector_out = last_sector;
}

/* _planes_driver: first sweep, each plane in turn as the
 * first of the core planes, firing the bullet; second
 * sweep, only when the first finds nothing, the split
 * candidates via the split bullet. On a conviction the
 * start is written from the last-start state and 1 is
 * returned; else 0. */
static int huge_planes_driver(uint32_t request,
                              const uint32_t *count_planes,
                              const uint32_t *count_rows,
                              const uint32_t *words,
                              uint32_t *start_out)
{
    HugeCtx  ctx;
    uint32_t core;
    uint32_t width;
    uint32_t candidate;
    uint32_t seam;
    uint32_t sector_out;
    int32_t  status;

    ctx.count_planes = count_planes;
    ctx.count_rows   = count_rows;
    ctx.words        = words;

    core      = request / (GEOMETRY_ARITY * GEOMETRY_ARITY);
    candidate = 0;
    while (candidate + core <= CUBE_PLANES_PER_CUBE) {
        if (count_planes[candidate] == 0) {
            sector_out = 0;
            status = huge_bullet(candidate, 0, request,
                                 (void *)&ctx, &sector_out);
            if (status == GEAR_SUCCESS) {
                huge_last_start(&start_out[0], &start_out[1],
                                &start_out[2]);
                return 1;
            }
        }
        candidate += 1;
    }
    /* Second sweep, the 2026-10-26 amendment: seams run
     * 1 .. 31 - (core - 1), so the head plane and the
     * trailer plane both always exist. */
    width = core - 1;
    seam  = 1;
    while (seam + width <= CUBE_PLANES_PER_CUBE - 1) {
        sector_out = 0;
        status = huge_split_call(seam, request, &ctx,
                                 &sector_out);
        if (status == GEAR_SUCCESS) {
            huge_last_start(&start_out[0], &start_out[1],
                            &start_out[2]);
            return 1;
        }
        seam += 1;
    }
    return 0;
}

int32_t huge_search(uint32_t request,
                    const uint32_t *count_planes,
                    const uint32_t *count_rows,
                    const uint32_t *words,
                    uint32_t *plane_out, uint32_t *row_out,
                    uint32_t *sector_out)
{
    uint32_t start[3];

    if (!huge_in_range(request)) {
        *plane_out  = 0;
        *row_out    = 0;
        *sector_out = 0;
        return HUGE_FAIL;
    }
    if (!huge_planes_driver(request, count_planes, count_rows,
                            words, start)) {
        *plane_out  = 0;
        *row_out    = 0;
        *sector_out = 0;
        return GEAR_EXHAUST;
    }
    *plane_out  = start[0];
    *row_out    = start[1];
    *sector_out = start[2];
    return GEAR_SUCCESS;
}
