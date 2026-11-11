/* Tiny.c -- see Tiny.h. Transcription of Tiny.py; the
 * Python is the specification. */

#include "Tiny.h"

#include "Geometry.h"
#include "Row.h"
#include "status.h"

void tiny_range(uint32_t *lo_out, uint32_t *hi_out)
{
    *lo_out = GEOMETRY_SECTORS_PER_BYTE + 1;
    *hi_out = ROW_SECTORS_PER_ROW;
}

int32_t tiny_bullet(uint32_t plane, uint32_t row,
                    uint32_t request, void *ctx,
                    uint32_t *sector_out)
{
    const uint32_t *words = (const uint32_t *)ctx;
    uint32_t global_row;
    uint32_t s;
    int32_t  run_start;
    uint32_t run_len;
    int      left_found;
    uint32_t left_start;
    uint32_t left_len;
    uint32_t sector;
    uint32_t tail_len;

    /* Boundary check, as in Listing 18: GlobalRow past
     * 1023 fails. Out-of-domain length fails the same way. */
    global_row = plane * GEOMETRY_ARITY + row;
    if (global_row > 1023) {
        return TINY_FAIL;
    }
    if (request < GEOMETRY_SECTORS_PER_BYTE + 1
            || request > ROW_SECTORS_PER_ROW) {
        return TINY_FAIL;
    }

    s = words[global_row];

    /* Left pass (sector package): scan sectors 0 toward 31;
     * the first (lowest-start) clear run of at least 9
     * sectors wins. Runs under 9 are skipped: 8-runs are
     * not eligible, strict separation of modes (Nano owns
     * 1..8). The sentinel at 32 flushes a run ending at
     * sector 31. */
    run_start  = -1;
    run_len    = 0;
    left_found = 0;
    left_start = 0;
    left_len   = 0;
    for (sector = 0; sector < 33; sector++) {
        if (sector < 32 && ((s >> sector) & 1u) == 0) {
            if (run_len == 0) {
                run_start = (int32_t)sector;
            }
            run_len += 1;
        } else {
            if (run_len >= 9) {
                left_found = 1;
                left_start = (uint32_t)run_start;
                left_len   = run_len;
                break;
            }
            run_len = 0;
        }
    }
    if (left_found && left_len >= request) {
        /* Place at the run's start: its lowest sectors.
         * This holds even when the revealed run happens to
         * be a tail run (pass scoping: Left always places
         * low-side; only the Right pass places ending at
         * sector 31). */
        *sector_out = left_start;
        return GEAR_SUCCESS;
    }
    /* Eligible but short, or none: second bite. */

    /* Right pass (second bite): tail run ending at sector
     * 31. */
    tail_len = 0;
    for (sector = 32; sector-- > 0; ) {
        if (((s >> sector) & 1u) == 0) {
            tail_len += 1;
        } else {
            break;
        }
    }
    if (tail_len >= request) {
        /* Place the requested run ending at sector 31. The
         * leftover, if the tail run was longer, becomes an
         * interior gap. */
        *sector_out = 32 - request;
        return GEAR_SUCCESS;
    }

    return TINY_FAIL;
}
