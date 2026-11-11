#!/usr/bin/env python3
"""Generate the admission vectors for the C transcription of
the request modes and the Engine (the modes tranche).

The Python is the specification and the oracle: this script
drives the Python Engine (allocator.Engine) and the Python
mode table through a fixed operation script and writes two
files:

  engine_ops.txt            the operation script itself
  engine_vectors_expected.txt
                            the result of every operation,
                            one line per operation, each line
                            carrying the operation's outcome
                            and a fingerprint of the engine
                            state after it (count_overall,
                            sum of count_rows, sum of
                            count_planes, sum of the sector
                            words).

EngineCheck.c replays engine_ops.txt against the C Engine
and prints the same lines; the diff between its output and
engine_vectors_expected.txt must be empty.

The script is partly adaptive: some operations (an unmark
of a block an earlier call returned, the terminal operation
of a fill) are chosen from the Python's actual results.
That is safe for admission: if the C placements diverge,
the replayed operation lands differently and the diff
shows it. Divergence amplifies; it never hides.

Operation script lines:
  reset                 a fresh Engine (kept in a slot;
                        later resets keep the earlier ones)
  use I                 switch the current engine to slot I
  call R                Engine(R)
  mark S C              Engine.mark(MPUPointer.load(S, C))
  unmark S C            Engine.unmark(MPUPointer.load(S, C))
  search K R            modes[K].search(R, counters...) on
                        the current engine's counters
  table K               the class name the table maps K to
                        (the .get fallback for out-of-table K)

Result lines mirror the operation, then the outcome:
  PTR start count word  an MPUPointer was returned
  STATUS value          a status int was returned
  NONE                  None was returned
  TUPLE st p r s        a mode search's uniform tuple
  NAME                  a class name, for table
then " | fp overall sumrows sumplanes sumwords".
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from allocator import Engine
from Modes import Modes
from MPUPointer import MPUPointer

HERE = os.path.dirname(os.path.abspath(__file__))
OPS_PATH = os.path.join(HERE, "engine_ops.txt")
EXPECTED_PATH = os.path.join(HERE, "engine_vectors_expected.txt")

ops = []
expected = []
engine = None
engines = []
current = -1
modes = Modes()


def fingerprint():
    return "{} {} {} {}".format(
        engine.count_overall,
        sum(engine.count_rows),
        sum(engine.count_planes),
        sum(engine.sectors))


def record(op_line, outcome):
    ops.append(op_line)
    expected.append("{} -> {} | fp {}".format(
        op_line, outcome, fingerprint()))


def do_reset():
    global engine, current
    engine = Engine()
    engines.append(engine)
    current = len(engines) - 1
    record("reset", "OK")


def do_use(index):
    global engine, current
    engine = engines[index]
    current = index
    record("use {}".format(index), "OK")


def render_call(result):
    if result is None:
        return "NONE"
    if isinstance(result, MPUPointer):
        return "PTR {} {} {}".format(result.start, result.count,
                                     int(result))
    return "STATUS {}".format(int(result))


def do_call(request):
    result = engine(request)
    record("call {}".format(request), render_call(result))
    return result


def do_mark(start, count):
    status = engine.mark(MPUPointer.load(start, count))
    record("mark {} {}".format(start, count),
           "STATUS {}".format(int(status)))
    return status


def do_unmark(start, count):
    status = engine.unmark(MPUPointer.load(start, count))
    record("unmark {} {}".format(start, count),
           "STATUS {}".format(int(status)))
    return status


def do_search(key, request):
    result = modes[key].search(request, engine.count_planes,
                               engine.count_rows, engine.sectors,
                               engine.count_overall)
    if result is None:
        outcome = "NONE"
    else:
        outcome = "TUPLE {} {} {} {}".format(*[int(v) for v in result])
    record("search {} {}".format(key, request), outcome)


def do_table(key):
    name = type(modes.get(key, modes.nothing)).__name__
    ops.append("table {}".format(key))
    expected.append("table {} -> {} | fp {}".format(
        key, name, fingerprint()))


def main():
    # The dispatch table itself: the boundary keys
    # test_engine.py pins, plus out-of-table fallbacks.
    do_reset()
    for key in (0, 1, 8, 9, 32, 33, 64, 65, 1024, 1025,
                32766, 32767, 32768, 32769, 40000):
        do_table(key)

    # Boundary calls, each on a fresh engine.
    for request in (0, 1, 8, 9, 32, 33, 64, 65, 1024, 1025,
                    32767, 32768, 32769):
        do_reset()
        do_call(request)
    # The whole cube, released, taken again.
    do_reset()
    do_call(32768)
    do_call(1)
    do_unmark(0, 32768)
    do_call(32768)
    do_unmark(0, 32768)

    # A mixed sequence on one engine, with interleaved
    # unmarks of blocks the calls actually returned.
    do_reset()
    held = []
    for request in (1, 8, 9, 32, 33, 64, 65, 100, 1024, 1025,
                    2000):
        result = do_call(request)
        if isinstance(result, MPUPointer):
            held.append(result)
    do_call(32767)  # more than remains: the step-1 gate
    for pointer in held[::2]:
        do_unmark(pointer.start, pointer.count)
    do_call(5000)
    do_call(1025)

    # Direct mark/unmark: overlay, double free, the
    # boundary guard, whole-cube marks.
    do_reset()
    do_mark(0, 100)
    do_mark(50, 100)      # overlay
    do_mark(100, 50)
    do_unmark(0, 100)
    do_unmark(0, 100)     # double free
    do_mark(32700, 100)   # runs past the end: FAIL at the gate
    do_mark(32767, 1)
    do_unmark(32767, 1)
    do_mark(0, 32768)
    do_unmark(0, 32768)

    # Fill to exhaustion at each request mode's top size;
    # the terminal operation is the first non-pointer.
    for size in (8, 32, 64, 1024):
        do_reset()
        for _ in range(5000):
            if not isinstance(do_call(size), MPUPointer):
                break
    # Huge fills and near-fills.
    do_reset()
    for _ in range(40):
        if not isinstance(do_call(1025), MPUPointer):
            break
    do_reset()
    do_call(32767)
    do_call(1025)
    do_reset()
    for _ in range(6):
        if not isinstance(do_call(8192), MPUPointer):
            break
    do_call(1)

    # Deterministic streams (a fixed LCG), with adaptive
    # immediate unmarks. The second stream runs on a second
    # engine in the same process, so the gears' cursor
    # carryover across Engine instances is exercised; the
    # third returns to the first engine.
    def stream(index, rounds, span, seed):
        if index != current:
            do_use(index)
        state = seed
        for _ in range(rounds):
            state = (state * 1103515245 + 12345) & 0x7FFFFFFF
            request = 1 + state % span
            result = do_call(request)
            if isinstance(result, MPUPointer) and (state >> 8) % 3 == 0:
                do_unmark(result.start, result.count)

    do_reset()
    first = current
    stream(first, 600, 1100, 12345)
    do_reset()
    second = current
    stream(second, 600, 1100, 12345)
    stream(first, 200, 1100, 777)
    do_reset()
    stream(current, 200, 32767, 999)

    # Targeted scenarios. The placements named here are the
    # unique placements the setups leave; the diff is the
    # assertion.
    # Large: an 80-sector hole at 1608..1687.
    do_reset()
    do_mark(0, 1608)
    do_mark(1688, 31080)
    do_call(80)
    # Huge split: 2000 across a 24-sector island at 5120.
    do_reset()
    do_mark(0, 5120)
    do_mark(5120, 24)
    do_mark(7144, 24)
    do_mark(7168, 25600)
    do_call(2000)
    # Huge with room in total but no contiguous span: two
    # 600-sector holes, so the driver exhausts.
    do_reset()
    do_mark(0, 1000)
    do_mark(1600, 18400)
    do_mark(20600, 12168)
    do_call(1025)
    # Small: an anchor whose pair crosses a plane boundary
    # (row 31's ancillary is row 32, in the next plane).
    do_reset()
    do_mark(0, 1008)
    do_mark(1056, 31712)
    do_call(33)
    # Tiny: row 0 is left with a short first run (sectors
    # 0..9) and a long tail run (16..31); where the sweep
    # starts depends on the gears' cursor, which the
    # streams above have moved -- the replay is the test.
    do_reset()
    do_mark(10, 6)
    do_call(16)
    # CubeCheck on a dirty engine, then cleaned.
    do_reset()
    do_mark(0, 1)
    do_call(32768)
    do_unmark(0, 1)
    do_call(32768)

    # Mode searches in situ, on the current engine's
    # counters: fresh first, then dirtied.
    do_reset()
    do_search(0, 0)
    do_search(1, 1)
    do_search(9, 9)
    do_search(33, 33)
    do_search(65, 65)
    do_search(1025, 2000)
    do_search(32768, 32768)
    do_mark(0, 100)
    do_search(1025, 2000)
    do_search(32768, 32768)
    do_search(65, 100)

    with open(OPS_PATH, "w") as fh:
        fh.write("\n".join(ops) + "\n")
    with open(EXPECTED_PATH, "w") as fh:
        fh.write("\n".join(expected) + "\n")
    print("wrote {} ops, {} expected lines".format(
        len(ops), len(expected)))


if __name__ == "__main__":
    main()
