/* Cube.c -- see Cube.h. Transcription of Cube.py; the
 * Python is the specification. */

#include "Cube.h"

#include <string.h>

#include "status.h"

void cube_init(Cube *cube)
{
    memset(cube->words, 0, sizeof(cube->words));
}

void cube_init_words(Cube *cube, const uint32_t *words)
{
    memcpy(cube->words, words, sizeof(cube->words));
}

int32_t cube_get_plane(const Cube *cube, uint32_t p,
                       Plane *out)
{
    if (p >= GEOMETRY_ARITY) {
        return KEYRANGE;
    }
    /* The view shares the store: the const cast is the
     * Python's own arrangement, where a Plane taken from a
     * Cube reads and writes the cube's words. */
    plane_init(out, (uint32_t *)cube->words, p);
    return SUCCESS;
}

int32_t cube_set_plane(Cube *cube, uint32_t p,
                       const uint32_t *rows)
{
    uint32_t base;
    uint32_t i;

    if (p >= GEOMETRY_ARITY) {
        return KEYRANGE;
    }
    base = p * GEOMETRY_ARITY;
    for (i = 0; i < GEOMETRY_ARITY; i++) {
        cube->words[base + i] = row_new(rows[i]);
    }
    return SUCCESS;
}

uint32_t cube_len(const Cube *cube)
{
    (void)cube;
    return GEOMETRY_ARITY;
}

int cube_contains(const Cube *cube, uint32_t p)
{
    (void)cube;
    return p < GEOMETRY_ARITY;
}
