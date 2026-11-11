"""Generate the expected vectors for the C foundation tranche.

Python GENERATES the oracle: this script exercises the Python
foundation modules (Geometry, Coordinate, Bijection,
MPUPointer, Nothing, Error) and writes the canonical result
lines to foundation_vectors_expected.txt. FoundationCheck.c
runs the identical operations against the C transcriptions
and prints the identical lines; the diff between its output
and this file must be empty.

The operation script here and in FoundationCheck.c is one
script in two languages: the same operations, in the same
order, with the same line formats. Boundary values are the
class edges of the design: 0, 1, 8, 9, 32, 33, 64, 65, 1024,
1025, 32767, 32768.

Refusals: the Python raises ValueError; the generator catches
it and prints the per-site status the C returns for the same
refusal (the POINTER_* codes in Status.py, the canonical
home; each case below names its site). Any other exception
is a generator bug and is allowed to propagate.

Usage: python gen_foundation_vectors.py   (from the engine
folder; needs bitstruct for MPUPointer, so use the venv)
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from Geometry import Geometry
from Coordinate import Coordinate
from Bijection import Bijection
from MPUPointer import MPUPointer, MOTOROLA, INTEL
from Nothing import Nothing
from Error import Error
from Status import Status


BOUNDARY = [0, 1, 8, 9, 32, 33, 64, 65, 1024, 1025, 32767, 32768]
COUNTS = [1, 8, 9, 32, 33, 64, 65, 1024, 1025, 32767, 32768]
STARTS = [0, 1, 8, 9, 32, 33, 64, 65, 1024, 1025, 32767]

lines = []


def fields_line(prefix, word, f):
    lines.append(
        f"{prefix} -> word=0x{word:08X} "
        f"v={f.virtual} r={f.reserved} start={f.start} count={f.count}")


# --- Geometry -------------------------------------------------
lines.append(
    f"geometry arity={Geometry.ARITY} dimensions={Geometry.DIMENSIONS} "
    f"sectors_per_byte={Geometry.SECTORS_PER_BYTE}")

# --- MPUPointer class constants -------------------------------
lines.append(
    f"mpu max_word={MPUPointer.MAX_WORD} "
    f"max_start={MPUPointer.MAX_START} max_count={MPUPointer.MAX_COUNT}")

# --- Coordinate: displacement ---------------------------------
for (s, r, p) in [(0, 0, 0), (1, 0, 0), (31, 0, 0), (0, 1, 0),
                  (0, 0, 1), (5, 7, 3), (31, 31, 31)]:
    c = Coordinate(sector=s, row=r, plane=p)
    lines.append(
        f"coord disp sector={s} row={r} plane={p} -> {c.displacement}")

# --- Coordinate: equality -------------------------------------
for (a, b) in [((1, 2, 3), (1, 2, 3)), ((1, 2, 3), (1, 2, 4)),
               ((0, 0, 0), (0, 0, 1)), ((31, 31, 31), (31, 31, 31))]:
    eq = int(Coordinate(*a) == Coordinate(*b))
    lines.append(
        f"coord eq ({a[0]},{a[1]},{a[2]})==({b[0]},{b[1]},{b[2]}) -> {eq}")

# --- Bijection ------------------------------------------------
bij = Bijection()
for d in BOUNDARY:
    c = bij.to_coordinate(d)
    lines.append(
        f"bijection to_coordinate {d} -> "
        f"sector={c.sector} row={c.row} plane={c.plane}")
for d in BOUNDARY:
    back = bij.to_displacement(bij.to_coordinate(d))
    lines.append(f"bijection roundtrip {d} -> {back}")
for (s, r, p) in [(0, 0, 0), (31, 31, 31), (1, 0, 1), (0, 31, 0)]:
    d = bij.to_displacement(Coordinate(sector=s, row=r, plane=p))
    lines.append(
        f"bijection to_displacement sector={s} row={r} plane={p} -> {d}")

# --- MPUPointer.load, Motorola --------------------------------
for c in COUNTS:
    p = MPUPointer.load(0, c)
    fields_line(f"mpu load start=0 count={c} reserved=0 order=motorola",
                int(p), p.unload())
for s in STARTS:
    p = MPUPointer.load(s, 1)
    fields_line(f"mpu load start={s} count=1 reserved=0 order=motorola",
                int(p), p.unload())

# --- MPUPointer.load, Intel -----------------------------------
for (s, c) in [(0, 1), (0, 32768), (32767, 1), (32767, 32768),
               (1025, 9)]:
    p = MPUPointer.load(s, c, order=INTEL)
    fields_line(f"mpu load start={s} count={c} reserved=0 order=intel",
                int(p), p.unload(INTEL))

# --- MPUPointer.load, reserved --------------------------------
p = MPUPointer.load(0, 1, reserved=1)
fields_line("mpu load start=0 count=1 reserved=1 order=motorola",
            int(p), p.unload())
p = MPUPointer.load(100, 200, reserved=1, order=INTEL)
fields_line("mpu load start=100 count=200 reserved=1 order=intel",
            int(p), p.unload(INTEL))

# --- MPUPointer.load_coordinate -------------------------------
for (d, c) in [(0, 1), (1, 8), (1024, 1024), (32767, 32768),
               (1025, 9)]:
    p = MPUPointer.load_coordinate(bij.to_coordinate(d), c)
    fields_line(f"mpu load_coordinate disp={d} count={c}",
                int(p), p.unload())

# --- MPUPointer adoption --------------------------------------
for w in [0, 1, 0x80000000, 0x40000000, 0x0000FFFF, 0xFFFFFFFF,
          0xDEADBEEF]:
    p = MPUPointer(w)
    lines.append(
        f"mpu adopt word=0x{w:08X} -> "
        f"v={p.virtual} r={p.reserved} start={p.start} count={p.count}")

# --- MPUPointer.coordinate ------------------------------------
for (s, c, order, name) in [(1025, 9, MOTOROLA, "motorola"),
                            (32767, 1, INTEL, "intel")]:
    p = MPUPointer.load(s, c, order=order)
    co = p.coordinate(order)
    lines.append(
        f"mpu coordinate start={s} count={c} order={name} -> "
        f"sector={co.sector} row={co.row} plane={co.plane}")

# --- MPUPointer refusals --------------------------------------
def refuse(desc, code, thunk):
    try:
        thunk()
    except ValueError:
        lines.append(f"mpu refuse {desc} -> status={code}")
        return
    raise AssertionError(f"expected ValueError: {desc}")


refuse("load start=32768 count=1", Status.POINTER_LOAD_START_FAIL,
       lambda: MPUPointer.load(32768, 1))
refuse("load start=-1 count=1", Status.POINTER_LOAD_START_FAIL,
       lambda: MPUPointer.load(-1, 1))
refuse("load start=0 count=0", Status.POINTER_LOAD_COUNT_FAIL,
       lambda: MPUPointer.load(0, 0))
refuse("load start=0 count=32769", Status.POINTER_LOAD_COUNT_FAIL,
       lambda: MPUPointer.load(0, 32769))
refuse("load start=0 count=1 reserved=2",
       Status.POINTER_LOAD_RESERVED_FAIL,
       lambda: MPUPointer.load(0, 1, reserved=2))
refuse("load start=0 count=1 order=bogus",
       Status.POINTER_LOAD_ORDER_FAIL,
       lambda: MPUPointer.load(0, 1, 0, "bogus"))
refuse("adopt word=4294967296", Status.POINTER_ADOPT_WORD_FAIL,
       lambda: MPUPointer(1 << 32))
refuse("adopt word=-1", Status.POINTER_ADOPT_WORD_FAIL,
       lambda: MPUPointer(-1))
refuse("unload order=bogus", Status.POINTER_UNLOAD_ORDER_FAIL,
       lambda: MPUPointer(0).unload("bogus"))
refuse("load_coordinate disp=32768 count=1",
       Status.POINTER_LOAD_START_FAIL,
       lambda: MPUPointer.load_coordinate(bij.to_coordinate(32768), 1))

# --- Nothing ----------------------------------------------------
rng = list(Nothing().range())
lines.append(
    f"nothing range -> lo={rng[0]} hi={rng[-1]} len={len(rng)}")
for req in [0, 7, 32768]:
    lines.append(f"nothing call request={req} -> {Nothing()(req)}")
silent = Nothing().search(123, None, None, None)
lines.append(f"nothing search -> {silent}")

# --- Error ------------------------------------------------------
rng = list(Error().range())
lines.append(
    f"error range -> lo={rng[0]} hi={rng[-1]} len={len(rng)}")
st, pl, rw, sc = Error().search(0, None, None, None)
lines.append(
    f"error search -> status={st} plane={pl} row={rw} sector={sc}")


out = os.path.join(HERE, "foundation_vectors_expected.txt")
with open(out, "w") as f:
    f.write("\n".join(lines) + "\n")
print(f"gen_foundation_vectors.py: wrote {out} ({len(lines)} lines)")
