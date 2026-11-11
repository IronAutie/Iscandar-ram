/* Nano.c -- see Nano.h. Transcription of Nano.py; the
 * Python is the specification. */

#include "Nano.h"

#include "Geometry.h"
#include "status.h"

void nano_range(uint32_t *lo_out, uint32_t *hi_out)
{
    *lo_out = 1;
    *hi_out = GEOMETRY_SECTORS_PER_BYTE;
}

int32_t nano_bullet(uint32_t plane, uint32_t row,
                    uint32_t request, void *ctx,
                    uint32_t *sector_out)
{
    const uint32_t *words = (const uint32_t *)ctx;
    uint32_t global_row;
    uint32_t s;
    uint32_t pos;

    /* Boundary check, as in Listing 17: GlobalRow past
     * 1023 fails. Out-of-domain length fails the same way. */
    global_row = plane * GEOMETRY_ARITY + row;
    if (global_row > 1023) {
        return NANO_FAIL;
    }
    if (request < 1 || request > GEOMETRY_SECTORS_PER_BYTE) {
        return NANO_FAIL;
    }

    s = words[global_row];

    /* Center function, position order (MSB first): the
     * smallest pos whose mask of L clear bits finds nothing
     * set. The mask formula is the paper's:
     * ((1 << L) - 1) << (32 - L - pos). */
    for (pos = 0; pos + request <= 32; pos++) {
        uint32_t mask = ((1u << request) - 1u)
                        << (32 - request - pos);
        if ((s & mask) == 0) {
            *sector_out = 31 - pos - request + 1;
            return GEAR_SUCCESS;
        }
    }

    return NANO_FAIL;
}
