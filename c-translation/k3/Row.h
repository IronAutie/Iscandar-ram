/* Row.h -- transcription of Row.py, the author's design; the
 * Python is the specification and this header reproduces it
 * exactly. Transcription only: no original content.
 *
 * Row.py is a class of constants and one masking rule, so
 * this is a constants-and-one-function header, in the
 * Geometry.h manner; there is no Row.c. */

#ifndef ISCANDAR_ROW_H
#define ISCANDAR_ROW_H

#include <stdint.h>

#include "Geometry.h"

/* class Row (int): "A bitfield of width A." */
#define ROW_SECTORS_PER_ROW    GEOMETRY_ARITY
#define ROW_SECTORS_TWEEN_ROWS (2 * ROW_SECTORS_PER_ROW)
#define ROW_WIDTH              GEOMETRY_ARITY
#define ROW_MASK               0xFFFFFFFFu

/* Row(value): the word, masked to width. The Python masks an
 * unbounded int; the C takes a uint64_t so every value the
 * Python can be handed in practice is masked the same way. */
static inline uint32_t row_new(uint64_t value)
{
    return (uint32_t)(value & ROW_MASK);
}

#endif /* ISCANDAR_ROW_H */
