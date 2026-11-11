/* Error.c -- see Error.h. Transcription of Error.py; the
 * Python is the specification. */

#include "Error.h"

#include "status.h"

void error_range(uint32_t *lo_out, uint32_t *hi_out)
{
    /* range(SECTORS_PER_CUBE): 0 .. SECTORS_PER_CUBE - 1. */
    *lo_out = 0;
    *hi_out = (uint32_t)ERROR_SECTORS_PER_CUBE - 1;
}

int32_t error_search(uint32_t *plane_out, uint32_t *row_out,
                     uint32_t *sector_out)
{
    /* (Status.FAIL, 0, 0, 0): the uniform search shape, so
     * the breakpoint for a table fault is this return. */
    *plane_out  = 0;
    *row_out    = 0;
    *sector_out = 0;
    return FAIL;
}
