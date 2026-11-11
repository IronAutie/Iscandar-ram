"""gen_k4_expected.py -- the Python oracle for the k=4 C test.

Python generates the oracle; the C is tested against it. This
script exercises the Python modules under test (Beam.py,
BeamCoordinate.py, CPUPointer.py) over a fixed operation script
and writes the transcript to test_k4_expected.txt. The C driver
(test_k4.c) runs the identical script and its output is diffed
against this file; the diff must be empty.

No substitution is made on either side: Beam.py runs as it
stands, with one real Python k=3 Engine (allocator.py) per
cube, and the C driver runs Beam.c over the real k=3 C engine
behind the CubeEngine adapter. The transcript therefore
admits Beam plus engine together, placements included. See
CubeEngine.h for the boundary.

Layout note: Beam.py locates the k=3 engine at ../iscandar,
which on this machine does not resolve (the engine lives in
~/workspace/iscandar/iscandar-engine). The generator supplies
that directory on sys.path itself; no module is modified.
"""

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ENGINE = HERE.parent / "iscandar" / "iscandar-engine"
sys.path.insert(0, str(ENGINE))
sys.path.insert(0, str(HERE))

from BeamCoordinate import BeamCoordinate
from CPUPointer import CPUPointer
from MPUPointer import MOTOROLA, INTEL
import Beam as BeamModule

ORDERS = [MOTOROLA, INTEL]
lines = []


def bc_line(i, c):
    lines.append(f"BC {i} c={c.cube} s={c.sector} r={c.row} "
                 f"p={c.plane} disp={c.k3().displacement}")


BCS = [BeamCoordinate(0, 0, 0, 0),
       BeamCoordinate(2, 3, 4, 5),
       BeamCoordinate(31, 31, 31, 31),
       BeamCoordinate(5, 40, 0, 0),
       BeamCoordinate(7, 0, 33, 0),
       BeamCoordinate(0, 0, 0, 0)]
for i, c in enumerate(BCS):
    bc_line(i, c)
for a, b in [(0, 5), (0, 1), (1, 1), (3, 4)]:
    lines.append(f"BCEQ {a} {b} = {1 if BCS[a] == BCS[b] else 0}")

CREATES = [(1, 100, 0, 0), (1, 100, 1, 0), (1, 100, 0, 1),
           (1, 100, 1, 1), (2, 32768, 0, 0), (0, 1, 0, 0),
           (3, 5, 0, 0), (4, 10, 0, 0), (1, 32768, 0, 1)]
for i, (bi, count, reserved, order) in enumerate(CREATES):
    p = CPUPointer(BCS[bi], count, reserved, ORDERS[order])
    co = p.coordinate()
    f = p.unload(ORDERS[order])
    lines.append(
        f"CPU {i} word=0x{p.word:016X} v={p.virtual} r={p.reserved} "
        f"start={p.start} count={p.count} "
        f"coord=({co.cube},{co.sector},{co.row},{co.plane}) "
        f"ov={f.virtual} or={f.reserved} ostart={f.start} "
        f"ocount={f.count}")

XCOORDS = [BeamCoordinate(32768, 0, 0, 0), BeamCoordinate(0, -1, 0, 0)]
BAD = [(1, 0, 0, MOTOROLA), (1, 32769, 0, MOTOROLA),
       (1, 10, 2, MOTOROLA), ("X0", 10, 0, MOTOROLA),
       ("X1", 10, 0, MOTOROLA), (1, 10, 0, 99)]
for i, (bi, count, reserved, order) in enumerate(BAD):
    coord = XCOORDS[int(bi[1])] if isinstance(bi, str) else BCS[bi]
    try:
        p = CPUPointer(coord, count, reserved, order)
        lines.append(f"CPUX {i} -> word=0x{p.word:016X}")
    except ValueError:
        lines.append(f"CPUX {i} -> FAIL")

ADOPTS = [0x0000000000000000, 0xFFFFFFFFFFFFFFFF,
          0x8000000000000063, 0x0000000000000001,
          0xDEADBEEF12345678]
for i, w in enumerate(ADOPTS):
    p = CPUPointer.adopt(w)
    co = p.coordinate()
    f = p.unload(INTEL)
    lines.append(
        f"ADOPT {i} word=0x{p.word:016X} v={p.virtual} "
        f"r={p.reserved} start={p.start} count={p.count} "
        f"coord=({co.cube},{co.sector},{co.row},{co.plane}) "
        f"iv={f.virtual} ir={f.reserved} istart={f.start} "
        f"icount={f.count}")

BEAMS = [
    (2, [("A", 100), ("A", 100), ("A", 200), ("F", 1), ("A", 50),
         ("A", 32768), ("A", 1), ("F", 4), ("F", 4), ("A", 40000),
         ("A", 0), ("A", -5), ("A", 32417), ("F", 0), ("F", 2),
         ("F", 3), ("F", 5), ("F", 6), ("A", 32768), ("A", 32768),
         ("A", 1)]),
    (1, [("A", 500), ("A", 32768), ("F", 0), ("A", 32768),
         ("A", 1)]),
    (0, [("A", 10)]),
    (-3, [("A", 10)]),
    (4, [("A", 32768), ("A", 32768), ("A", 32768), ("A", 32768),
         ("A", 32768)]),
]
for bi, (arg, ops) in enumerate(BEAMS):
    beam = BeamModule.Beam(arg)
    lines.append(f"BEAMINIT {bi} arg={arg} card={beam.cardinality} "
                 f"supported={beam.sectors_supported}")
    produced = []
    for op, val in ops:
        if op == "A":
            p = beam.allocate(val)
            if p is None:
                lines.append(f"BM{bi} A req={val} -> NONE "
                             f"overall={beam.count_overall} "
                             f"lead={beam.lead}")
            else:
                produced.append(p)
                lines.append(f"BM{bi} A req={val} -> 0x{p.word:016X} "
                             f"overall={beam.count_overall} "
                             f"lead={beam.lead}")
        else:
            st = beam.free(produced[val])
            lines.append(f"BM{bi} F #{val} -> status={st} "
                         f"overall={beam.count_overall} "
                         f"lead={beam.lead}")

out = HERE / "test_k4_expected.txt"
out.write_text("\n".join(lines) + "\n")
print(f"wrote {out} ({len(lines)} lines)")
