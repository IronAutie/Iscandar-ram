/* MPUPointer.h -- transcription of MPUPointer.py, the author's
 * design; the Python is the specification and this code
 * reproduces it exactly. Transcription only: no original
 * content.
 *
 * A 32-bit virtual address for the MPU, as a bitfield. The
 * word, in Motorola order (MSB first, the paper's parlance):
 *
 *     Virtual   1 bit   top bit: the word belongs to the
 *                       user's application; it is not to be
 *                       exposed directly to the microprocessor
 *                       for decoding
 *     Reserved  1 bit   not needed today; held against someday
 *     Start    15 bits  starting sector -- a name: 0..32767
 *     Count    15 bits  stored shifted: stored + 1 is the true
 *                       count, so counts run 1..32768, the
 *                       whole cube is representable, and a
 *                       zero count is not
 *
 * The Intel form declares the same fields from the low end
 * upward: Virtual at bit 0, Reserved at bit 1, Start at bits
 * 2..16, stored Count at bits 17..31. Decode order is a code
 * switch, never a bit in the word: the order argument selects
 * Motorola (the default) or Intel allocation.
 *
 * Every pointer is BORN VIRTUAL (the author's consistency
 * rule): creation through mpupointer_load always raises the
 * Virtual flag, the full cube included -- there is no way to
 * ask load for a flag-down word. A word with the flag down can
 * only enter by adoption, mpupointer_adopt, as an existing
 * word read from somewhere -- never by creation.
 *
 * Refusals: the Python raises ValueError for every field or
 * word outside its range, and never masks or truncates
 * silently, because a reliable pointer cannot be built by
 * quietly changing the caller's numbers. The C returns a
 * status instead: one per-site code per distinct refusal
 * site, named in Status.py (POINTER_ADOPT_WORD_FAIL,
 * POINTER_LOAD_RESERVED_FAIL, POINTER_LOAD_START_FAIL,
 * POINTER_LOAD_COUNT_FAIL, POINTER_LOAD_ORDER_FAIL,
 * POINTER_UNLOAD_ORDER_FAIL; added 2026-11-10, superseding
 * the foundation tranche's generic FAIL at these sites). On
 * a refusal the output is not written, in the check-then-set
 * spirit of the commit path: a refused call changes
 * nothing. */

#ifndef ISCANDAR_MPUPOINTER_H
#define ISCANDAR_MPUPOINTER_H

#include <stdint.h>

#include "Coordinate.h"
#include "Geometry.h"

/* The order switch: MPUPointer.MOTOROLA and MPUPointer.INTEL
 * in the Python. Any other value is refused, as the Python
 * refuses an unknown order string. */
#define MPUPOINTER_MOTOROLA 0
#define MPUPOINTER_INTEL    1

/* MPUPointer.MAX_WORD, MAX_START, MAX_COUNT in the Python:
 * ARITY ** 3 is the whole cube, 32768 sectors. */
#define MPUPOINTER_MAX_WORD  0xFFFFFFFFu
#define MPUPOINTER_MAX_START \
    (GEOMETRY_ARITY * GEOMETRY_ARITY * GEOMETRY_ARITY - 1)
#define MPUPOINTER_MAX_COUNT \
    (GEOMETRY_ARITY * GEOMETRY_ARITY * GEOMETRY_ARITY)

/* The Fields namedtuple of the Python: unload's result, with
 * the count back in true form (stored + 1). */
typedef struct {
    uint32_t virtual;
    uint32_t reserved;
    uint32_t start;
    uint32_t count;
} MPUPointerFields;

/* MPUPointer(word): adoption of an existing word. The Python
 * takes an unbounded int and refuses anything outside
 * 0..MAX_WORD, so the C takes a uint64_t and refuses what a
 * uint32_t input could not even express. On SUCCESS *word_out
 * holds the word, unchanged. The refusal is
 * POINTER_ADOPT_WORD_FAIL. */
__declspec(dllexport)
int32_t mpupointer_adopt(uint64_t word, uint32_t *word_out);

/* MPUPointer.load(start, count, reserved=0, order=MOTOROLA):
 * pack named fields into a word, the Virtual flag raised.
 * `count` is the true count. Refuses, each with its own
 * per-site code: reserved not 0 or 1
 * (POINTER_LOAD_RESERVED_FAIL), start outside 0..MAX_START
 * (POINTER_LOAD_START_FAIL), count outside 1..MAX_COUNT
 * (POINTER_LOAD_COUNT_FAIL), unknown order
 * (POINTER_LOAD_ORDER_FAIL). There is deliberately no
 * check that start + count fits the cube, because the Python
 * has none: fields are validated, geometry is not. */
__declspec(dllexport)
int32_t mpupointer_load(uint32_t start, uint32_t count,
                        uint32_t reserved, int order,
                        uint32_t *word_out);

/* MPUPointer.load_coordinate(coordinate, count, ...): pack
 * the block beginning at a Coordinate, flag raised. The
 * coordinate's displacement is load's start, so a coordinate
 * past the last name is refused by load's start check. */
__declspec(dllexport)
int32_t mpupointer_load_coordinate(const Coordinate *coordinate,
                                   uint32_t count,
                                   uint32_t reserved, int order,
                                   uint32_t *word_out);

/* unload(order=MOTOROLA): the word's fields, with the count
 * back in true form. Any 32-bit word unloads; only an unknown
 * order is refused (POINTER_UNLOAD_ORDER_FAIL). */
__declspec(dllexport)
int32_t mpupointer_unload(uint32_t word, int order,
                          MPUPointerFields *fields_out);

/* The fields at a glance, Motorola order: the Python
 * properties virtual, reserved, start, count. Properties
 * cannot fail, so these return the value directly. */
__declspec(dllexport)
uint32_t mpupointer_virtual(uint32_t word);
__declspec(dllexport)
uint32_t mpupointer_reserved(uint32_t word);
__declspec(dllexport)
uint32_t mpupointer_start(uint32_t word);
__declspec(dllexport)
uint32_t mpupointer_count(uint32_t word);

/* coordinate(order=MOTOROLA): the start of this block, as a
 * Coordinate, through the Bijection. Refuses only an unknown
 * order. */
__declspec(dllexport)
int32_t mpupointer_coordinate(uint32_t word, int order,
                              Coordinate *coordinate_out);

#endif /* ISCANDAR_MPUPOINTER_H */
