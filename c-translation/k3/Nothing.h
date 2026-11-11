/* Nothing.h -- transcription of Nothing.py, the author's
 * design; the Python is the specification and this code
 * reproduces it exactly. Transcription only: no original
 * content. */

#ifndef ISCANDAR_NOTHING_H
#define ISCANDAR_NOTHING_H

#include <stdint.h>

/* class Nothing (Mode): "silent mode when bounds exceeded in
 * cube." Nothing is request 0, and the engine fallback for an
 * out-of-table request. It places nothing, marks nothing, and
 * fails nothing. Only Nothing.py's own definitions are
 * transcribed here; the Mode machinery (the gears, the scan)
 * is not part of this tranche. */

/* Nothing.range(): inclusive(0, 0) -- the integers 0 through
 * 0, both ends included: the single value 0. Reported as the
 * inclusive bounds. */
__declspec(dllexport)
void nothing_range(uint32_t *lo_out, uint32_t *hi_out);

/* Nothing.search(...): the Python returns None, silently,
 * with no sweep -- not a gear tuple and not a status. Engine
 * returns that None directly, without unpacking it. A C
 * function cannot return None, so the absence is reported on
 * its own channel: *result_present is set to 0 and nothing
 * else is written. The return is SUCCESS in the narrow sense
 * the header for this tranche defines for it: the silent path
 * was taken, and the silent path is not a failure. */
__declspec(dllexport)
int32_t nothing_search(int *result_present);

/* Nothing.__call__(request): returns 0, for any request. */
__declspec(dllexport)
uint32_t nothing_call(uint32_t request);

#endif /* ISCANDAR_NOTHING_H */
