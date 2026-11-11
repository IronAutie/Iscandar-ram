/* Plane.c -- see Plane.h. Transcription of Plane.py; the
 * Python is the specification. */

#include "Plane.h"

#include "status.h"

void plane_init(Plane *plane, uint32_t *words,
                uint32_t plane_index)
{
    plane->words = words;
    plane->base  = plane_index * GEOMETRY_ARITY;
}

int32_t plane_get(const Plane *plane, uint32_t i,
                  uint32_t *out)
{
    if (i >= GEOMETRY_ARITY) {
        return KEYRANGE;
    }
    *out = row_new(plane->words[plane->base + i]);
    return SUCCESS;
}

int32_t plane_set(Plane *plane, uint32_t i, uint32_t value)
{
    if (i >= GEOMETRY_ARITY) {
        return KEYRANGE;
    }
    plane->words[plane->base + i] = row_new(value);
    return SUCCESS;
}

uint32_t plane_len(const Plane *plane)
{
    (void)plane;
    return GEOMETRY_ARITY;
}

int plane_contains(const Plane *plane, uint32_t i)
{
    (void)plane;
    return i < GEOMETRY_ARITY;
}
