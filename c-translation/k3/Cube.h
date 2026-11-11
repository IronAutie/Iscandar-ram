/* Cube.h -- transcription of Cube.py, the author's design;
 * the Python is the specification and this code reproduces
 * it exactly. Transcription only: no original content. */

#ifndef ISCANDAR_CUBE_H
#define ISCANDAR_CUBE_H

#include <stdint.h>

#include "Geometry.h"
#include "Plane.h"
#include "Row.h"

/* class Cube: "A view over the whole flat store: 32 planes
 * x 32 rows." */
#define CUBE_PLANES_PER_CUBE   GEOMETRY_ARITY
#define CUBE_ROWS_PER_CUBE \
    (CUBE_PLANES_PER_CUBE * PLANE_ROWS_PER_PLANE)
#define CUBE_SECTORS_PER_CUBE \
    (CUBE_PLANES_PER_CUBE * PLANE_SECTORS_PER_PLANE)

/* The flat store itself: 1024 unsigned 32-bit row words,
 * index plane * 32 + row, each word holding one row's 32
 * sector bits. Cube.py's .sectors property is this same
 * array under the author's name for it, not a copy; the
 * hand-off to C in the Python is this array's buffer, and
 * here it is simply the array. */
typedef struct {
    uint32_t words[CUBE_ROWS_PER_CUBE];
} Cube;

/* Cube(words=None): a zeroed store when no words are given
 * (array("I", [0]) * 1024 in the Python). */
__declspec(dllexport)
void cube_init(Cube *cube);

/* Cube(words): seat the view on a caller's store, the
 * Python's words argument. */
__declspec(dllexport)
void cube_init_words(Cube *cube, const uint32_t *words);

/* cube[p]: the Plane view over plane p. The Python raises
 * IndexError outside 0..31; the boundary conversion is
 * KEYRANGE, and *out is not seated. */
__declspec(dllexport)
int32_t cube_get_plane(const Cube *cube, uint32_t p,
                       Plane *out);

/* cube[p] = plane: whole-plane assignment, the 32 row
 * values copied in, each masked through Row. A plane
 * assignment of other than 32 values is a ValueError in
 * the Python; the C signature takes exactly 32, so that
 * refusal has no C shape and the count is not a runtime
 * question here. KEYRANGE for a bad plane index. */
__declspec(dllexport)
int32_t cube_set_plane(Cube *cube, uint32_t p,
                       const uint32_t *rows);

/* len(cube): always ARITY. */
__declspec(dllexport)
uint32_t cube_len(const Cube *cube);

/* p in cube: 1 when 0 <= p < ARITY, else 0. */
__declspec(dllexport)
int cube_contains(const Cube *cube, uint32_t p);

#endif /* ISCANDAR_CUBE_H */
