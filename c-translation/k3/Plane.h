/* Plane.h -- transcription of Plane.py, the author's
 * design; the Python is the specification and this code
 * reproduces it exactly. Transcription only: no original
 * content. */

#ifndef ISCANDAR_PLANE_H
#define ISCANDAR_PLANE_H

#include <stdint.h>

#include "Geometry.h"
#include "Row.h"

/* class Plane: "A view over 32 consecutive row words of the
 * flat cube store." */
#define PLANE_ROWS_PER_PLANE    GEOMETRY_ARITY
#define PLANE_SECTORS_PER_PLANE \
    (PLANE_ROWS_PER_PLANE * ROW_SECTORS_PER_ROW)

/* A Plane holds no words of its own: the store pointer and
 * the base offset of its first row (plane * ARITY). Reads
 * and writes go through to the store, as in the Python. */
typedef struct {
    uint32_t *words;
    uint32_t  base;
} Plane;

/* Plane(words, plane=0): seat the view on its plane. */
__declspec(dllexport)
void plane_init(Plane *plane, uint32_t *words,
                uint32_t plane_index);

/* plane[i]: the row word at i, as a Row (masked on the way
 * out, as the Python's Row(...) construction masks). The
 * Python raises IndexError outside 0..31; the engine
 * boundary converts that to KEYRANGE (Status.py), and *out
 * is not written. */
__declspec(dllexport)
int32_t plane_get(const Plane *plane, uint32_t i,
                  uint32_t *out);

/* plane[i] = value: store Row(value) at i. Same boundary
 * conversion as plane_get. */
__declspec(dllexport)
int32_t plane_set(Plane *plane, uint32_t i, uint32_t value);

/* len(plane): always ARITY. */
__declspec(dllexport)
uint32_t plane_len(const Plane *plane);

/* i in plane: 1 when 0 <= i < ARITY, else 0. */
__declspec(dllexport)
int plane_contains(const Plane *plane, uint32_t i);

#endif /* ISCANDAR_PLANE_H */
