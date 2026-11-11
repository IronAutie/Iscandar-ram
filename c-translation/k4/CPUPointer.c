/* CPUPointer.c -- see CPUPointer.h for the contract.
 *
 * The pack/unpack arithmetic below was checked field by field
 * against the Python's bitstruct formats:
 *   MOTOROLA 'u1u1p17u30u15' pack(virtual, reserved, start, stored)
 *   INTEL    'u15u30p17u1u1' pack(stored, start, reserved, virtual)
 * Both name the same 64 bits; the Intel form reads its fields
 * from the low end upward. */

#include "CPUPointer.h"
#include "K4Status.h"

int32_t cpu_pointer_create(const BeamCoordinate *coordinate,
                           uint32_t count, uint32_t reserved,
                           int order, CPUPointer *out)
{
    K3Coordinate local;
    int64_t displacement;
    int64_t start;
    uint32_t stored;

    if (reserved > 1) {
        return K4_FAIL;
    }
    if (count < 1 || count > K4_CPU_MAX_COUNT) {
        return K4_FAIL;
    }

    beam_coordinate_k3(coordinate, &local);
    displacement = k3_coordinate_displacement(&local);
    start = (int64_t)coordinate->cube * 32768 + displacement;
    if (start < 0 || start > (int64_t)K4_CPU_MAX_START) {
        return K4_FAIL;
    }

    stored = count - 1;
    if (order == K4_ORDER_INTEL) {
        out->word = ((uint64_t)stored << 49)
                  | ((uint64_t)(uint32_t)start << 19)
                  | ((uint64_t)reserved << 1)
                  | 1u;  /* virtual: born virtual, always */
    } else if (order == K4_ORDER_MOTOROLA) {
        out->word = (1ull << 63)  /* virtual: born virtual */
                  | ((uint64_t)reserved << 62)
                  | ((uint64_t)(uint32_t)start << 15)
                  | (uint64_t)stored;
    } else {
        return K4_FAIL;
    }
    return K4_SUCCESS;
}

int32_t cpu_pointer_adopt(uint64_t word, CPUPointer *out)
{
    out->word = word;
    return K4_SUCCESS;
}

int32_t cpu_pointer_unload(const CPUPointer *p, int order,
                           CPUPointerFields *out)
{
    uint64_t w = p->word;
    uint32_t stored;

    if (order == K4_ORDER_INTEL) {
        stored = (uint32_t)(w >> 49);
        out->start = (uint32_t)((w >> 19) & 0x3FFFFFFFull);
        out->reserved_bit = (uint32_t)((w >> 1) & 1u);
        out->virtual_bit = (uint32_t)(w & 1u);
    } else if (order == K4_ORDER_MOTOROLA) {
        out->virtual_bit = (uint32_t)(w >> 63);
        out->reserved_bit = (uint32_t)((w >> 62) & 1u);
        out->start = (uint32_t)((w >> 15) & 0x3FFFFFFFull);
        stored = (uint32_t)(w & 0x7FFFull);
    } else {
        return K4_FAIL;
    }
    out->count = stored + 1;
    return K4_SUCCESS;
}

uint32_t cpu_pointer_virtual(const CPUPointer *p)
{
    CPUPointerFields f;
    (void)cpu_pointer_unload(p, K4_ORDER_MOTOROLA, &f);
    return f.virtual_bit;
}

uint32_t cpu_pointer_reserved(const CPUPointer *p)
{
    CPUPointerFields f;
    (void)cpu_pointer_unload(p, K4_ORDER_MOTOROLA, &f);
    return f.reserved_bit;
}

uint32_t cpu_pointer_start(const CPUPointer *p)
{
    CPUPointerFields f;
    (void)cpu_pointer_unload(p, K4_ORDER_MOTOROLA, &f);
    return f.start;
}

uint32_t cpu_pointer_count(const CPUPointer *p)
{
    CPUPointerFields f;
    (void)cpu_pointer_unload(p, K4_ORDER_MOTOROLA, &f);
    return f.count;
}

int32_t cpu_pointer_coordinate(const CPUPointer *p, int order,
                               BeamCoordinate *out)
{
    CPUPointerFields f;
    int32_t status;
    uint32_t local;
    int32_t cube;
    int32_t plane;
    int32_t row;
    int32_t sector;

    status = cpu_pointer_unload(p, order, &f);
    if (status != K4_SUCCESS) {
        return status;
    }

    /* Bijection.to_coordinate on the local displacement:
     * plane = d / 1024, row = (d % 1024) / 32, sector = d % 32. */
    cube = (int32_t)(f.start / 32768u);
    local = f.start % 32768u;
    plane = (int32_t)(local / 1024u);
    row = (int32_t)((local % 1024u) / 32u);
    sector = (int32_t)(local % 32u);

    beam_coordinate_init(out, cube, sector, row, plane);
    return K4_SUCCESS;
}

uint64_t cpu_pointer_word(const CPUPointer *p)
{
    return p->word;
}

int cpu_pointer_equal(const CPUPointer *a, const CPUPointer *b)
{
    return a->word == b->word;
}
