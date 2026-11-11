/* K4Status.h -- status codes for the k=4 C transcription.
 *
 * The values mirror Status.py, the ONE canonical home for status
 * codes in the k=3 engine (see status.h there, generated from
 * Status.py by gen_status.py). They are copied here so the k=4 C
 * files stand alone in this repository; they must never drift
 * from Status.py. If Status.py changes, this file changes with
 * it, by hand, in the same commit.
 *
 * Doctrine (project law): a failure returns a status code. It
 * never crashes, never aborts, is never retried silently, and is
 * never papered over by a fallback. */

#ifndef ISCANDAR_K4_STATUS_H
#define ISCANDAR_K4_STATUS_H

#define K4_SUCCESS       0  /* Status.py: SUCCESS */
#define K4_FAIL          1  /* Status.py: FAIL */
#define K4_GEAR_EXHAUST  2  /* Status.py: GEAR_EXHAUST */
#define K4_FAIL_OVERLAY  4  /* Status.py: FAIL_OVERLAY */
#define K4_GEAR_SUCCESS  8  /* Status.py: GEAR_SUCCESS */

#endif /* ISCANDAR_K4_STATUS_H */
