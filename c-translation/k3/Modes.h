/* Modes.h -- transcription target of Modes.py.
 *
 * Modes builds the dispatch table: every request size in
 * the cube's range maps to the mode that answers it. The
 * Python's dict is built by successive merges in a fixed
 * order -- Error's full range first, then Message
 * (Nothing), then the request modes, then CubeCheck last,
 * each later merge overriding the earlier entries for its
 * keys -- so the final table is: key 0 -> Nothing, keys
 * 1..32767 -> the request mode of their class, key 32768
 * -> CubeCheck. That is the whole content of the merge
 * order once it has run, and it is what modes_get
 * reproduces: the fallback (Nothing, for a key outside
 * every range) matches the Engine's .get(request, nothing)
 * fallback in the Python.
 *
 * CubeCheck is engine-reachable through this table at
 * request 32768, so its search is transcribed here as a
 * mode; CubeCheck.py itself is check instrumentation and
 * is not transcribed.
 */

#ifndef ISCANDAR_MODES_H
#define ISCANDAR_MODES_H

#include <stdint.h>

#include "Mode.h"

/* modes_count: the size of the Python's merged table,
 * 32769 entries, keys 0..32768 -- the same figure
 * test_table_size_and_keys pins. */
uint32_t modes_count(void);

/* modes_has_key: 1 when key is in the table's key range
 * 0..32768, else 0. */
int modes_has_key(uint32_t key);

/* modes_get: the mode serving key; falls back to the
 * Nothing mode for any key outside the table, exactly as
 * the Python Engine's modes.get(request, nothing) does.
 * The returned pointer is to static storage owned by
 * this module. */
const Mode *modes_get(uint32_t key);

/* modes_nothing: the table's Nothing mode (the fallback),
 * so the Engine can compare a served mode against it the
 * way the Python compares against its Nothing instance. */
const Mode *modes_nothing(void);

#endif /* ISCANDAR_MODES_H */
