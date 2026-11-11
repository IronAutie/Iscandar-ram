/* Nothing.c -- see Nothing.h. Transcription of Nothing.py;
 * the Python is the specification. */

#include "Nothing.h"

#include "status.h"

void nothing_range(uint32_t *lo_out, uint32_t *hi_out)
{
    /* inclusive(0, 0): range(0, 0 + 1). */
    *lo_out = 0;
    *hi_out = 0;
}

int32_t nothing_search(int *result_present)
{
    /* Return None, silent, no sweep: nothing placed, nothing
     * marked, nothing failed. This assignment is the whole
     * body on purpose: the breakpoint place for the silent
     * path, as the Python's docstring says of its own body. */
    *result_present = 0;
    return SUCCESS;
}

uint32_t nothing_call(uint32_t request)
{
    (void)request;
    return 0;
}
