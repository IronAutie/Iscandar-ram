/* Large.c -- see Large.h. Transcription of Large.py; the
 * Python is the specification. */

#include "Large.h"

#include "Geometry.h"
#include "Plane.h"
#include "Row.h"
#include "status.h"

/* The most recent conviction's full start, (plane, row,
 * sector) -- self.last_start in the Python. */
static uint32_t last_plane;
static uint32_t last_row;
static uint32_t last_sector;

void large_range(uint32_t *lo_out, uint32_t *hi_out)
{
    *lo_out = ROW_SECTORS_TWEEN_ROWS + 1;
    *hi_out = PLANE_SECTORS_PER_PLANE;
}

uint32_t large_trailing_run(uint32_t word)
{
    uint32_t run = 0;
    uint32_t sector;

    for (sector = 32; sector-- > 0; ) {
        if (((word >> sector) & 1u) == 0) {
            run += 1;
        } else {
            break;
        }
    }
    return run;
}

uint32_t large_leading_run(uint32_t word)
{
    uint32_t run = 0;
    uint32_t sector;

    for (sector = 0; sector < 32; sector++) {
        if (((word >> sector) & 1u) == 0) {
            run += 1;
        } else {
            break;
        }
    }
    return run;
}

static int large_in_range(uint32_t request)
{
    return request >= ROW_SECTORS_TWEEN_ROWS + 1
        && request <= PLANE_SECTORS_PER_PLANE;
}

/* _probe: one core start. On a fit, write the start
 * coordinate (plane, row, sector) and return 1; else
 * return 0. core_start is a global row index. */
static int large_probe(int32_t core_start, uint32_t request,
                       const uint32_t *words,
                       uint32_t *start_out)
{
    uint32_t core;
    uint32_t remainder;
    int32_t  head_row;
    int32_t  tail_row;
    int32_t  g;
    uint32_t head_len;
    uint32_t head_take;
    uint32_t remaining;

    if (!large_in_range(request)) {
        return 0;
    }
    core      = request / GEOMETRY_ARITY;
    remainder = request % GEOMETRY_ARITY;
    if (core_start < 0 || core_start + (int32_t)core > 1024) {
        return 0;
    }
    for (g = core_start; g < core_start + (int32_t)core; g++) {
        if (words[g] != 0) {
            return 0;
        }
    }
    if (remainder == 0) {
        start_out[0] = (uint32_t)core_start / GEOMETRY_ARITY;
        start_out[1] = (uint32_t)core_start % GEOMETRY_ARITY;
        start_out[2] = 0;
        return 1;
    }
    head_row = core_start - 1;
    tail_row = core_start + (int32_t)core;
    if (core_start == 0) {
        /* Scenario (a): no head; the following row's
         * leading sectors must satisfy the remainder. */
        if (tail_row >= 1024) {
            return 0;
        }
        if (large_leading_run(words[tail_row]) < remainder) {
            return 0;
        }
        start_out[0] = 0;
        start_out[1] = 0;
        start_out[2] = 0;
        return 1;
    }
    if (core_start + (int32_t)core - 1 == 1023) {
        /* Scenario (b): no tail; the previous row's
         * trailing sectors must satisfy the remainder. */
        if (head_row < 0) {
            return 0;
        }
        if (large_trailing_run(words[head_row]) < remainder) {
            return 0;
        }
        start_out[0] = (uint32_t)head_row / GEOMETRY_ARITY;
        start_out[1] = (uint32_t)head_row % GEOMETRY_ARITY;
        start_out[2] = 32 - remainder;
        return 1;
    }
    /* Scenario (c): both rows exist. Right oracle first, as
     * many trailing sectors as will fit; then the Left
     * oracle for what remains owed. */
    head_len  = large_trailing_run(words[head_row]);
    head_take = head_len < remainder ? head_len : remainder;
    remaining = remainder - head_take;
    if (remaining == 0) {
        start_out[0] = (uint32_t)head_row / GEOMETRY_ARITY;
        start_out[1] = (uint32_t)head_row % GEOMETRY_ARITY;
        start_out[2] = 32 - head_take;
        return 1;
    }
    if (tail_row >= 1024) {
        return 0;
    }
    if (large_leading_run(words[tail_row]) < remaining) {
        return 0;
    }
    if (head_take > 0) {
        start_out[0] = (uint32_t)head_row / GEOMETRY_ARITY;
        start_out[1] = (uint32_t)head_row % GEOMETRY_ARITY;
        start_out[2] = 32 - head_take;
        return 1;
    }
    start_out[0] = (uint32_t)core_start / GEOMETRY_ARITY;
    start_out[1] = (uint32_t)core_start % GEOMETRY_ARITY;
    start_out[2] = 0;
    return 1;
}

/* _rows_driver: the row level handler for one launching
 * plane. The window sum is built once for the first start
 * in the plane, then rolled one row at a time; no loop
 * traverses the core per candidate. */
static int large_rows_driver(uint32_t plane, uint32_t request,
                             const uint32_t *count_rows,
                             const uint32_t *words,
                             uint32_t *start_out)
{
    uint32_t core;
    uint32_t first;
    int32_t  window_sum;
    uint32_t core_start;
    uint32_t g;

    core  = request / GEOMETRY_ARITY;
    first = plane * GEOMETRY_ARITY;
    if (first + core > 1024) {
        return 0;
    }
    window_sum = 0;
    for (g = first; g < first + core; g++) {
        window_sum += (int32_t)count_rows[g];
    }
    core_start = first;
    while (core_start < first + GEOMETRY_ARITY) {
        if (core_start + core > 1024) {
            break;
        }
        if (window_sum == 0) {
            if (large_probe((int32_t)core_start, request,
                            words, start_out)) {
                return 1;
            }
        }
        if (core_start + core < 1024) {
            window_sum += (int32_t)count_rows[core_start + core]
                        - (int32_t)count_rows[core_start];
        }
        core_start += 1;
    }
    return 0;
}

/* _probe_split: one core start of the second pass, the
 * split-edge case, f = core - 1 blank rows. The head row's
 * trailing run is taken whole toward the budget (budget =
 * remainder + arity); the tail row must cover the rest
 * with its leading run. */
static int large_probe_split(int32_t core_start,
                             uint32_t request,
                             const uint32_t *words,
                             uint32_t *start_out)
{
    uint32_t core;
    int32_t  width;
    int32_t  g;
    int32_t  budget;
    int32_t  head_take;
    int32_t  owed;
    int32_t  tail_row;
    int32_t  start;

    if (!large_in_range(request)) {
        return 0;
    }
    core  = request / GEOMETRY_ARITY;
    width = (int32_t)core - 1;
    if (core_start < 0 || core_start + width > 1024) {
        return 0;
    }
    for (g = core_start; g < core_start + width; g++) {
        if (words[g] != 0) {
            return 0;
        }
    }
    budget    = (int32_t)request - width * GEOMETRY_ARITY;
    head_take = 0;
    if (core_start > 0) {
        head_take = (int32_t)large_trailing_run(
            words[core_start - 1]);
    }
    owed = budget - head_take;
    if (owed <= 0) {
        return 0;
    }
    tail_row = core_start + width;
    if (tail_row >= 1024) {
        return 0;
    }
    if ((int32_t)large_leading_run(words[tail_row]) < owed) {
        return 0;
    }
    start = core_start * GEOMETRY_ARITY - head_take;
    start_out[0] = (uint32_t)start / PLANE_SECTORS_PER_PLANE;
    start_out[1] = ((uint32_t)start % PLANE_SECTORS_PER_PLANE)
                   / GEOMETRY_ARITY;
    start_out[2] = (uint32_t)start % GEOMETRY_ARITY;
    return 1;
}

/* _rows_driver_split: the second-pass row handler, the
 * first pass's rolling window at width core - 1. */
static int large_rows_driver_split(uint32_t plane,
                                   uint32_t request,
                                   const uint32_t *count_rows,
                                   const uint32_t *words,
                                   uint32_t *start_out)
{
    uint32_t core;
    int32_t  width;
    uint32_t first;
    int32_t  window_sum;
    uint32_t core_start;
    uint32_t g;

    core  = request / GEOMETRY_ARITY;
    width = (int32_t)core - 1;
    first = plane * GEOMETRY_ARITY;
    if (width < 1 || first + (uint32_t)width > 1024) {
        return 0;
    }
    window_sum = 0;
    for (g = first; g < first + (uint32_t)width; g++) {
        window_sum += (int32_t)count_rows[g];
    }
    core_start = first;
    while (core_start < first + GEOMETRY_ARITY) {
        if (core_start + (uint32_t)width > 1024) {
            break;
        }
        if (window_sum == 0) {
            if (large_probe_split((int32_t)core_start, request,
                                  words, start_out)) {
                return 1;
            }
        }
        if (core_start + (uint32_t)width < 1024) {
            window_sum += (int32_t)count_rows[core_start
                                              + (uint32_t)width]
                        - (int32_t)count_rows[core_start];
        }
        core_start += 1;
    }
    return 0;
}

/* _planes_driver: two passes over the launchings in
 * ascending plane order. Pass one under the launch gate
 * (the free sectors of the starting plane plus the next
 * plane must reach the request; for plane 31 its own free
 * alone). Pass two, the 2026-10-26 amendment, runs only
 * when pass one finds nothing: the same launchings with
 * the split rows driver, under the launch gate plus the
 * free count of the single head row before the launching
 * plane. */
static int large_planes_driver(uint32_t request,
                               const uint32_t *count_planes,
                               const uint32_t *count_rows,
                               const uint32_t *words,
                               uint32_t *start_out)
{
    uint32_t plane;
    uint32_t free_here;
    uint32_t free_next;
    uint32_t head_allowance;

    for (plane = 0; plane < GEOMETRY_ARITY; plane++) {
        free_here = PLANE_SECTORS_PER_PLANE - count_planes[plane];
        free_next = 0;
        if (plane + 1 < GEOMETRY_ARITY) {
            free_next = PLANE_SECTORS_PER_PLANE
                        - count_planes[plane + 1];
        }
        if (free_here + free_next < request) {
            continue;
        }
        if (large_rows_driver(plane, request, count_rows,
                              words, start_out)) {
            return 1;
        }
    }
    for (plane = 0; plane < GEOMETRY_ARITY; plane++) {
        free_here = PLANE_SECTORS_PER_PLANE - count_planes[plane];
        free_next = 0;
        if (plane + 1 < GEOMETRY_ARITY) {
            free_next = PLANE_SECTORS_PER_PLANE
                        - count_planes[plane + 1];
        }
        head_allowance = 0;
        if (plane > 0) {
            head_allowance = ROW_SECTORS_PER_ROW
                - count_rows[plane * GEOMETRY_ARITY - 1];
        }
        if (free_here + free_next + head_allowance < request) {
            continue;
        }
        if (large_rows_driver_split(plane, request, count_rows,
                                    words, start_out)) {
            return 1;
        }
    }
    return 0;
}

int32_t large_bullet(uint32_t plane, uint32_t row,
                     uint32_t request, void *ctx,
                     uint32_t *sector_out)
{
    const uint32_t *words = (const uint32_t *)ctx;
    uint32_t start[3];

    if (!large_probe((int32_t)(plane * GEOMETRY_ARITY + row),
                     request, words, start)) {
        return LARGE_FAIL;
    }
    last_plane  = start[0];
    last_row    = start[1];
    last_sector = start[2];
    *sector_out = start[2];
    return GEAR_SUCCESS;
}

void large_last_start(uint32_t *plane_out, uint32_t *row_out,
                      uint32_t *sector_out)
{
    *plane_out  = last_plane;
    *row_out    = last_row;
    *sector_out = last_sector;
}

int32_t large_search(uint32_t request,
                     const uint32_t *count_planes,
                     const uint32_t *count_rows,
                     const uint32_t *words,
                     uint32_t *plane_out, uint32_t *row_out,
                     uint32_t *sector_out)
{
    uint32_t start[3];

    if (!large_in_range(request)) {
        *plane_out = 0;
        *row_out   = 0;
        *sector_out = 0;
        return LARGE_FAIL;
    }
    if (!large_planes_driver(request, count_planes, count_rows,
                             words, start)) {
        *plane_out = 0;
        *row_out   = 0;
        *sector_out = 0;
        return GEAR_EXHAUST;
    }
    *plane_out  = start[0];
    *row_out    = start[1];
    *sector_out = start[2];
    return GEAR_SUCCESS;
}
