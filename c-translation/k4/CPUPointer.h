/* CPUPointer.h -- C transcription of CPUPointer.py.
 *
 * A CPUPointer is a standalone 64-bit word (not an int subclass
 * in C: a struct holding the word, nothing else). The layout,
 * settled at k=4 and matching the Python's bitstruct formats:
 *
 *   Motorola order (the paper's parlance, MSB first):
 *     V(1) R(1) pad(17) start(30) stored(15)
 *     word = V<<63 | R<<62 | start<<15 | stored
 *   Intel order (the same fields declared from the low end):
 *     stored(15) start(30) pad(17) R(1) V(1)
 *     word = stored<<49 | start<<19 | R<<1 | V
 *
 * start is cube over local: start = cube * 32768 + displacement,
 * so its high 15 bits name the cube and its low 15 the sector
 * inside the cube. stored is the true count minus 1, so counts
 * run 1..32768 and the whole cube is representable.
 *
 * A pointer is born two ways only, as in the Python: creation
 * (cpu_pointer_create), which always raises the Virtual flag,
 * and adoption (cpu_pointer_adopt), which takes an existing word
 * as it lies. Validation refuses, never masks: a field outside
 * its range is a K4_FAIL status, and nothing is silently
 * truncated. Every fallible function returns an int32_t status;
 * nothing here crashes or aborts. */

#ifndef ISCANDAR_K4_CPU_POINTER_H
#define ISCANDAR_K4_CPU_POINTER_H

#include <stdint.h>

#include "BeamCoordinate.h"

/* Decode order, a code switch, never a bit in the word
 * (MPUPointer.py: MOTOROLA / INTEL). */
#define K4_ORDER_MOTOROLA 0
#define K4_ORDER_INTEL    1

#define K4_CPU_MAX_START  ((1u << 30) - 1)  /* CPUPointer.MAX_START */
#define K4_CPU_MAX_COUNT  32768u            /* Cube.SECTORS_PER_CUBE */

typedef struct {
    uint64_t word;
} CPUPointer;

/* CPUPointer.Fields: the unloaded word, count in true form. */
typedef struct {
    uint32_t virtual_bit;
    uint32_t reserved_bit;
    uint32_t start;
    uint32_t count;
} CPUPointerFields;

/* Creation: a BeamCoordinate and a count go in, the word is
 * born, Virtual raised. reserved must be 0 or 1, count must be
 * 1..32768, and the computed start must be 0..2^30-1; any
 * violation, or an unknown order, is K4_FAIL and *out is not
 * written. */
int32_t cpu_pointer_create(const BeamCoordinate *coordinate,
                           uint32_t count, uint32_t reserved,
                           int order, CPUPointer *out);

/* Adoption: an existing word enters as it is, flag as it lies.
 * Any 64-bit word is adoptable, so this cannot fail. */
int32_t cpu_pointer_adopt(uint64_t word, CPUPointer *out);

/* unload(order): the word's fields, count back in true form.
 * An unknown order is K4_FAIL. */
int32_t cpu_pointer_unload(const CPUPointer *p, int order,
                           CPUPointerFields *out);

/* The fields at a glance, Motorola order (the Python
 * properties). Infallible: Motorola unload always succeeds. */
uint32_t cpu_pointer_virtual(const CPUPointer *p);
uint32_t cpu_pointer_reserved(const CPUPointer *p);
uint32_t cpu_pointer_start(const CPUPointer *p);
uint32_t cpu_pointer_count(const CPUPointer *p);

/* coordinate(order): the start of this block, as a
 * BeamCoordinate (via Bijection.to_coordinate on the local
 * displacement). An unknown order is K4_FAIL. */
int32_t cpu_pointer_coordinate(const CPUPointer *p, int order,
                               BeamCoordinate *out);

/* __int__ and __eq__. */
uint64_t cpu_pointer_word(const CPUPointer *p);
int cpu_pointer_equal(const CPUPointer *a, const CPUPointer *b);

#endif /* ISCANDAR_K4_CPU_POINTER_H */
