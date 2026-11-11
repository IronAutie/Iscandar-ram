/* MPUPointer.c -- see MPUPointer.h. Transcription of
 * MPUPointer.py; the Python is the specification. */

#include "MPUPointer.h"

#include "Bijection.h"
#include "status.h"

int32_t mpupointer_adopt(uint64_t word, uint32_t *word_out)
{
    /* __new__: not 0 <= word <= MAX_WORD is refused. */
    if (word > (uint64_t)MPUPOINTER_MAX_WORD) {
        return POINTER_ADOPT_WORD_FAIL;
    }
    *word_out = (uint32_t)word;
    return SUCCESS;
}

int32_t mpupointer_load(uint32_t start, uint32_t count,
                        uint32_t reserved, int order,
                        uint32_t *word_out)
{
    uint32_t stored;

    /* Creation always raises Virtual: virtual = 1, below. The
     * Python validates reserved, then start, then count, then
     * dispatches on order; the checks run in that same order
     * here, each returning its own per-site code, so a call
     * violating two rules names the site the Python's first
     * raise names. All checks run before the pack, so a
     * refusal writes nothing. */
    if (reserved != 0 && reserved != 1) {
        return POINTER_LOAD_RESERVED_FAIL;
    }
    if (start > (uint32_t)MPUPOINTER_MAX_START) {
        return POINTER_LOAD_START_FAIL;
    }
    if (count < 1 || count > (uint32_t)MPUPOINTER_MAX_COUNT) {
        return POINTER_LOAD_COUNT_FAIL;
    }
    if (order != MPUPOINTER_MOTOROLA && order != MPUPOINTER_INTEL) {
        return POINTER_LOAD_ORDER_FAIL;
    }

    stored = count - 1;
    if (order == MPUPOINTER_INTEL) {
        /* bitstruct 'u15u15u1u1', packed (stored, start,
         * reserved, virtual) MSB first: stored at bits 31..17,
         * start at bits 16..2, reserved at bit 1, virtual at
         * bit 0 -- the fields declared from the low end up. */
        *word_out = (stored << 17) | (start << 2)
                  | (reserved << 1) | 1u;
    } else {
        /* bitstruct 'u1u1u15u15', packed (virtual, reserved,
         * start, stored) MSB first. */
        *word_out = (1u << 31) | (reserved << 30)
                  | (start << 15) | stored;
    }
    return SUCCESS;
}

int32_t mpupointer_load_coordinate(const Coordinate *coordinate,
                                   uint32_t count,
                                   uint32_t reserved, int order,
                                   uint32_t *word_out)
{
    /* cls.load(coordinate.displacement, count, reserved,
     * order): the displacement walks in as load's start. */
    return mpupointer_load(coordinate_displacement(coordinate),
                           count, reserved, order, word_out);
}

int32_t mpupointer_unload(uint32_t word, int order,
                          MPUPointerFields *fields_out)
{
    uint32_t stored;

    if (order == MPUPOINTER_INTEL) {
        stored              = (word >> 17) & 0x7FFFu;
        fields_out->start   = (word >> 2) & 0x7FFFu;
        fields_out->reserved = (word >> 1) & 1u;
        fields_out->virtual = word & 1u;
    } else if (order == MPUPOINTER_MOTOROLA) {
        fields_out->virtual  = (word >> 31) & 1u;
        fields_out->reserved = (word >> 30) & 1u;
        fields_out->start    = (word >> 15) & 0x7FFFu;
        stored               = word & 0x7FFFu;
    } else {
        return POINTER_UNLOAD_ORDER_FAIL;
    }
    /* The count back in true form: stored + 1. */
    fields_out->count = stored + 1;
    return SUCCESS;
}

uint32_t mpupointer_virtual(uint32_t word)
{
    MPUPointerFields fields;

    /* The Motorola unload of a word cannot fail. */
    (void)mpupointer_unload(word, MPUPOINTER_MOTOROLA, &fields);
    return fields.virtual;
}

uint32_t mpupointer_reserved(uint32_t word)
{
    MPUPointerFields fields;

    (void)mpupointer_unload(word, MPUPOINTER_MOTOROLA, &fields);
    return fields.reserved;
}

uint32_t mpupointer_start(uint32_t word)
{
    MPUPointerFields fields;

    (void)mpupointer_unload(word, MPUPOINTER_MOTOROLA, &fields);
    return fields.start;
}

uint32_t mpupointer_count(uint32_t word)
{
    MPUPointerFields fields;

    (void)mpupointer_unload(word, MPUPOINTER_MOTOROLA, &fields);
    return fields.count;
}

int32_t mpupointer_coordinate(uint32_t word, int order,
                              Coordinate *coordinate_out)
{
    MPUPointerFields fields;
    int32_t status;

    /* Bijection().to_coordinate(self.unload(order).start) */
    status = mpupointer_unload(word, order, &fields);
    if (status != SUCCESS) {
        return status;
    }
    bijection_to_coordinate(fields.start, coordinate_out);
    return SUCCESS;
}
