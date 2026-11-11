#!/usr/bin/env bash
# run_k4_c_tests.sh -- build the k=4 C transcription and test it
# against the Python-generated oracle.
#
# Python generates the oracle (test_k4_expected.txt) by driving
# the real Python Beam over the real Python k=3 engine; the C
# driver (test_k4.c) runs the identical operation script over
# Beam.c backed by the real k=3 C engine (the CubeEngine
# adapter), and its output must diff empty against the oracle.
# Uses the workspace venv python (bitstruct) unless $PYTHON
# says otherwise.
set -euo pipefail
cd "$(dirname "$0")"

PYTHON="${PYTHON:-$HOME/workspace/.venvs/iscandar-venv/bin/python}"
ENGINE_DIR="$HOME/workspace/iscandar/iscandar-engine"
BUILD_DIR="$(mktemp -d)"
trap 'rm -rf "$BUILD_DIR"' EXIT

# The k=3 engine headers carry __declspec (the author's
# Windows-first declaration form); the prefix header defines
# it away, the repo's established Linux pattern.
printf '#define __declspec(x)\n' > "$BUILD_DIR/declspec.h"

cc -std=c99 -Wall -Wextra -include "$BUILD_DIR/declspec.h" \
    -I"$ENGINE_DIR" -o "$BUILD_DIR/test_k4" \
    test_k4.c CPUPointer.c BeamCoordinate.c Beam.c CubeEngine.c \
    "$ENGINE_DIR/Engine.c" "$ENGINE_DIR/Modes.c" \
    "$ENGINE_DIR/Mode.c" "$ENGINE_DIR/Nano.c" \
    "$ENGINE_DIR/Tiny.c" "$ENGINE_DIR/Small.c" \
    "$ENGINE_DIR/Large.c" "$ENGINE_DIR/Huge.c" \
    "$ENGINE_DIR/Cube.c" "$ENGINE_DIR/Plane.c" \
    "$ENGINE_DIR/Coordinate.c" "$ENGINE_DIR/Bijection.c" \
    "$ENGINE_DIR/MPUPointer.c" "$ENGINE_DIR/Nothing.c" \
    "$ENGINE_DIR/Error.c" "$ENGINE_DIR/Mark.c" \
    "$ENGINE_DIR/Unmark.c" "$ENGINE_DIR/Commit.c" \
    "$ENGINE_DIR/RowsCheck.c" "$ENGINE_DIR/PlanesCheck.c" \
    "$ENGINE_DIR/PairsRowDriver.c" "$ENGINE_DIR/PlaneSmallDriver.c"

"$PYTHON" gen_k4_expected.py
"$BUILD_DIR/test_k4" > test_k4_actual.txt

if diff test_k4_expected.txt test_k4_actual.txt; then
    echo "k=4 C test: PASS (diff empty, $(wc -l < test_k4_expected.txt) lines)"
else
    echo "k=4 C test: FAIL" >&2
    exit 1
fi
