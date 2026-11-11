/* BeamCoordinate.h -- C transcription of BeamCoordinate.py.
 *
 * A BeamCoordinate is a k=3 Coordinate (sector, row, plane) plus
 * the cube it lives in. Transcription notes: the Python class
 * validates nothing -- its constructor stores the four fields as
 * given, and arithmetic (displacement) is done on the stored
 * values. The C struct does the same: fields are int32_t, stored
 * as given, and no function here refuses a value. Range refusal
 * lives one level up, in CPUPointer creation, exactly as in the
 * Python, where CPUPointer.__init__ checks the computed start. */

#ifndef ISCANDAR_K4_BEAM_COORDINATE_H
#define ISCANDAR_K4_BEAM_COORDINATE_H

#include <stdint.h>

/* The k=3 Coordinate a BeamCoordinate rests on (Coordinate.py):
 * one sector, in one row, in one plane. */
typedef struct {
    int32_t sector;
    int32_t row;
    int32_t plane;
} K3Coordinate;

typedef struct {
    int32_t cube;
    int32_t sector;
    int32_t row;
    int32_t plane;
} BeamCoordinate;

/* Constructor: stores the fields as given (BeamCoordinate.py). */
void beam_coordinate_init(BeamCoordinate *out, int32_t cube,
                          int32_t sector, int32_t row, int32_t plane);

/* BeamCoordinate.k3(): the k=3 coordinate inside this one. */
void beam_coordinate_k3(const BeamCoordinate *bc, K3Coordinate *out);

/* Coordinate.displacement (Coordinate.py, Axiom IV Eq. 6):
 * sector + row * 32 + plane * 1024. Computed in 64 bits so the
 * arithmetic matches Python's unbounded ints for any field
 * values the struct can hold. */
int64_t k3_coordinate_displacement(const K3Coordinate *c);

/* BeamCoordinate.__eq__/__hash__ rest on the four fields. */
int beam_coordinate_equal(const BeamCoordinate *a,
                          const BeamCoordinate *b);

#endif /* ISCANDAR_K4_BEAM_COORDINATE_H */
