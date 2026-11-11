/* Small.c -- see Small.h. Transcription of Small.py; the
 * Python is the specification. */

#include "Small.h"

#include "Gears.h"
#include "Geometry.h"
#include "Row.h"
#include "status.h"

void small_range(uint32_t *lo_out, uint32_t *hi_out)
{
    *lo_out = ROW_SECTORS_PER_ROW + 1;
    *hi_out = ROW_SECTORS_TWEEN_ROWS;
}

int32_t small_bullet(uint32_t plane, uint32_t row,
                     uint32_t request, void *ctx,
                     uint32_t *sector_of_anchor)
{
    const uint32_t *words = (const uint32_t *)ctx;
    uint32_t global_row;
    uint32_t anchor;
    uint32_t ancillary;
    uint32_t tail_len;
    uint32_t head_len;
    uint32_t sector;

    /* Global row 1023 has no later row, so it cannot be an
     * anchor. Out-of-domain length fails the same way. */
    global_row = plane * GEOMETRY_ARITY + row;
    if (global_row > 1022) {
        return SMALL_FAIL;
    }
    if (request < ROW_SECTORS_PER_ROW + 1
            || request > ROW_SECTORS_TWEEN_ROWS) {
        return SMALL_FAIL;
    }

    anchor    = words[global_row];
    ancillary = words[global_row + 1];

    /* P: anchor tail run ending at sector 31. P = 0 means
     * the anchor does not reach the row edge, so this pair
     * cannot join, however much free space the pair holds
     * elsewhere. Edge-join only is the semantic intent. */
    tail_len = 0;
    for (sector = 32; sector-- > 0; ) {
        if (((anchor >> sector) & 1u) == 0) {
            tail_len += 1;
        } else {
            break;
        }
    }

    /* Q: ancillary head run starting at sector 0. Q = 0
     * means the ancillary does not start at the row edge,
     * so this pair cannot join. */
    head_len = 0;
    for (sector = 0; sector < 32; sector++) {
        if (((ancillary >> sector) & 1u) == 0) {
            head_len += 1;
        } else {
            break;
        }
    }

    /* Logical pair certification, not a free-sum gate:
     * both edges must be present, and together they must
     * reach the request. */
    if (tail_len < 1 || head_len < 1) {
        return SMALL_FAIL;
    }
    if (tail_len + head_len < request) {
        return SMALL_FAIL;
    }

    /* Conviction: start at the anchor tail's start, so the
     * block runs forward from there into the ancillary
     * head. */
    *sector_of_anchor = 32 - tail_len;
    return GEAR_SUCCESS;
}

int32_t small_search(uint32_t request,
                     const uint32_t *count_planes,
                     const uint32_t *count_rows,
                     const uint32_t *words,
                     uint32_t *plane_out, uint32_t *row_out,
                     uint32_t *sector_out)
{
    /* Gears.scan_small: bind once, then the gear. The ctx
     * handed to the bullet is the words array, as the
     * Python's bullet closure supplies it. */
    planes_small_gear_bind(small_bullet, (void *)words);
    return planes_small_gear(count_planes, count_rows,
                             request, sector_out, row_out,
                             plane_out);
}
