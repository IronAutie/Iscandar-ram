#!/usr/bin/env python3
"""Insert ISCANDAR_STEP() counters into the kr12-scratch work
copies of the engine sources, at exactly the sites the
atomicity simulation counted (atomic-sim/instrument.py,
same anchors, same counts). The sim's three counters
(search, commit, gate) are one counter here: the
termination table reports their sum per call, and the sum
is what KR12 measures.

Loop sites: the STEP line goes in as the first statement
of the loop body, at the loop head's indent plus four.
Pattern sites mirror the sim's three patterns. Every
insertion is anchored and counted; a mismatch stops the
run. After this script, the work copies differ from the
pristine copies by the counter lines, the steps include,
and nothing else from this script."""

import re, sys
from pathlib import Path

SRC = Path(__file__).resolve().parent / "work"

LOOPS = {  # file -> [line_number_in_original]  (atomic-sim/instrument.py)
    "Nano.c":  [40],
    "Tiny.c":  [55, 85],
    "Small.c": [48, 60],
    "Large.c": [28, 43, 83, 175, 179, 225, 277, 281, 321, 336],
    "Huge.c":  [74, 85, 134, 182, 244, 271, 302, 326, 361, 461, 525, 543],
    "Commit.c": [133, 145, 178, 189],
    "Engine.c": [214],
    "Beam.c":   [83],
    "PairsRowDriver.c": [70, 130],
    "Modes.c":  [121],
}

def fail(msg):
    print(f"INSTRUMENT FAIL: {msg}", file=sys.stderr); sys.exit(1)

def indent_of(line):
    return line[: len(line) - len(line.lstrip())]

def instrument_loops(fname, sites):
    p = SRC / fname
    lines = p.read_text().split("\n")
    for lineno in sorted(sites, reverse=True):
        head = lines[lineno - 1]
        if "for" not in head and "while" not in head:
            fail(f"{fname}:{lineno} is not a loop head: {head!r}")
        j = lineno - 1
        while "{" not in lines[j]:
            j += 1
            if j - (lineno - 1) > 2:
                fail(f"{fname}:{lineno} brace not found within 2 lines")
        lines.insert(j + 1, f"{indent_of(head)}    ISCANDAR_STEP();")
    p.write_text("\n".join(lines))
    print(f"{fname}: {len(sites)} loop sites")

def instrument_pattern(fname, pattern, where, expected, label):
    p = SRC / fname
    lines = p.read_text().split("\n")
    hits = [i for i, ln in enumerate(lines) if re.search(pattern, ln)]
    if len(hits) != expected:
        fail(f"{fname}: {label} pattern matched {len(hits)}, expected {expected}")
    for i in reversed(hits):
        if where == "after":
            lines.insert(i + 1, "    ISCANDAR_STEP();")
        else:
            lines.insert(i, f"{indent_of(lines[i])}ISCANDAR_STEP();")
    p.write_text("\n".join(lines))
    print(f"{fname}: {len(hits)} {label} sites")

def add_include(fname):
    p = SRC / fname
    lines = p.read_text().split("\n")
    for i, ln in enumerate(lines):
        if ln.startswith("#include"):
            lines.insert(i, '#include "iscandar_steps.h"')
            break
    else:
        fail(f"{fname}: no #include found")
    p.write_text("\n".join(lines))

for fname, sites in LOOPS.items():
    instrument_loops(fname, sites)

instrument_pattern("RowsCheck.c", r"^row_\d\d:$", "after", 32, "row block")
instrument_pattern("PlanesCheck.c",
                   r"status = \(\*lane\+\+\)\(count_planes, count_rows, request,",
                   "before", 32, "lane invocation")
instrument_pattern("PlaneSmallDriver.c", r"/\* Trial \d+:", "after", 32, "trial")

for fname in list(LOOPS) + ["RowsCheck.c", "PlanesCheck.c", "PlaneSmallDriver.c"]:
    add_include(fname)

print("instrumentation complete")
