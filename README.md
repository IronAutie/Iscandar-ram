# Iscandar-ram

Paper 3: *Debian to Iscandar: Full Speed Ahead!*

Subtitle: *Installing the RAM*

The first two papers replayed Debian's recorded allocations through Iscandar. This project takes the next step: a live Debian, its allocator calls redirected in real time to Iscandar, running on QEMU. Iscandar realized as the machine's memory: the allocator as RAM.

The stack of repositories:

- `iscandar`: the k=3 theory and engine (Paper 1, the monograph).
- `iscandar-k4`: the k=4 extension and the boot-replay replication package (Paper 2).
- `Iscandar-ram`: the live system (Paper 3).

Method, in the author's formulation: the recorded trace is all we need as the control; compare it against the source, and patch there, at the allocator centers the exercised calls pass through.

Status: composition started. The patch-point list and dye-test results are in the repository, together with `paper-3-composition.md` and the `paper-3.tex` manuscript skeleton. The live interposition is not built yet.
