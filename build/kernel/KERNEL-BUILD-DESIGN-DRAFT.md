# Kernel build design draft: Iscandar in the kernel

Status: STETTED by Andrew, 2026-11-19 18:34. Written
2026-11-19 at Andrew's go ("Go ahead. The user land rig
has served its purpose."). This is the design of record
for the kernel round. Nothing here is built yet.

The userland rig is retired by the same ruling. Its
remaining bench items in the fix round's order (the
negative boot, the true boot, the degrade proof) are
closed by retirement, not completed. The bridge
visibility defect found at D0 stays on the record,
documented and unfixed (section 7); it is a pathology
of the channel, and this design has no channel.

## Rulings at the head

R1. Direction (2026-11-18 22:01, confirmed 2026-11-19
08:50). Userland experiments were good for design but
have their limitations; Iscandar goes back into the
kernel, then is instrumented thoroughly. No steps are
skipped, and failures are documented so they can be
returned to. Lesson of the bridge, for the record:
kernel processes may not call userland handlers.

R2. Atomic interface (2026-11-18 23:15-23:19). In the
kernel, the interface is a synchronous call, not a
conversation: no channel, no cursors, no poll, no
timeout. The fast path (alloc, free) is atomic under
spin_lock_irqsave, spin-guarded and sleep-free. The
control path (registration, teardown) stays in process
context, under a mutex, confirm-then-commit.

R3. The hypothesis and its scope (2026-11-18 23:22),
in Andrew's words: "disable interrupts while the
kernel requests memory from iscandar, and then there
should be no race condition." True for this build
because the callee is code in the caller's context and
there is no second actor; re-entry is made impossible,
not managed. It must never be applied to the bridge,
whose callee is a process that needs the CPU and the
tick: interrupts off across a bridge wait would freeze
the answerer and guarantee the timeout. The bridge is
the counterexample that fixes the scope.

R4. Elevation (2026-11-18 23:24, conditional in his
words, "if that works"). The kernel version is not a
bonus section; it is essential, and it can be made to
work because of the guarantee by formal method that
the worst case growth is bounded: operation cost is a
function of the request and the geometry, independent
of allocation history, and that history-independence
is the precondition licensing interrupts-off atomic
service. If this build benches, Paper 3 restructures:
userland interposition becomes the method, the
in-kernel atomic build becomes the result.

R5. The termination table is the interrupt-latency
budget (simulation 2026-11-19, atomic-sim/REPORT.md).
The predicted worst-case suppression time is worst
steps times the per-step cost on the target, plus an
NMI allowance. The step counts are measured (section
4); the per-step cost is a bench measurement, owed;
the table, not a hope, is what the irqsave section
stands on.

R6. NMI (Andrew's question, 2026-11-19 08:22). An NMI
can interrupt the section but cannot ruin it: NMI
handlers may not allocate, from this allocator or from
stock slab alike, so no NMI can be a second actor
inside the section. An NMI executes no engine steps
and cannot inflate the step count; it stretches the
window's wall time by its handler's duration, which
enters the prediction only as the allowance term in
R5. Parity with the stock allocator: NMI context is
outside every allocator's contract.

R7. Instrumentation kept (2026-11-18 23:39), in
Andrew's words: "We should keep any instrumentation
that helped us." The canary stays, transformed into an
init-time self-check (section 5). Principle:
instrumentation is part of the system, not scaffolding;
witnesses stand at the doors (init, registration, state
read), never inside the allocation walk. His gloss:
"Its a practical way to verify that the theorems are
enforced."

R8. Bootstrap ex nihilo (2026-11-18 23:43, refined
23:52, completed 2026-11-19 00:04). The initial
allocation is not part of the allocation system. The
engine's own storage (spectrum, ledger, registry, beam
state, sized by formula from the build constants)
arrives already marked up as a solid mass in the image
at link time, and there are no initial loops: a
generator emits the mass from the pinned constants,
born self-consistent, its counters agreeing with its
sector marks from the first byte. Init verifies; it
does not fill. In Andrew's phrase: "We mark the map
ex-nihlio [ex nihilo]." Pedigree: his 16-bit
single-board practice,
where the link map was the whole of the law.

R9. One engine text. Conditional compilation at the
platform seams only; the engine source is byte-for-byte
the same in the userland harness and in the kernel,
and the same admission vectors are replayed against
both builds.

R10. Coverage audit before the first boot (2026-11-18
22:40). Every allocation entry point in the source is
enumerated, each marked door'd or deliberately stock,
with its free twin beside it; bulk, node, and lru
variants are named explicitly. His reason, in the
record: "its unlikely as hell, but it only takes one
unpatched window." A missed twin threatens the evidence
as well as the memory: patched-alloc with unpatched-free
leaks, unpatched-alloc with patched-free is turned away
safely by the arena test.

R11. Pre-ready service. Calls that arrive before the
engine is ready fall to stock under the prefix counters,
as they do in the bridge. The ready flag guards a
moment in the boot, not a process.

R12. Failure doctrine (standing). A failure at any
level returns a status, never a crash, never tolerated;
an overlay or a double-free is an invariant violation
and a hard failure, debugger time. No retry loops, no
silent fallbacks.

R13. Posture (standing). Uniprocessor, embedded. In
Andrew's words: "Kernel is another way to say
embedded." The kernel's size, arena included, is a
constant of the deployment.

## 1. Shape of the build

The engine text lands in the kernel tree as built-in
code, not a module: it must serve before modules exist,
and its storage is link-time mass (R8), which only a
built-in can carry. The platform seams, the only places
conditional compilation may appear (R9), are the pointer
order switch, the adoption boundary where a born pointer
is taken up, and the door glue. Everything else is the
shared text.

The doors stand where the bridge patches stood. Patches
0001 to 0005 already enumerated the allocation centers
(build/patch/MANIFEST.md) and named the families not
served (build/patch/NOT-PATCHED.md); that enumeration
is the coverage audit's first draft, to be re-derived
against the tree and closed under R10 before the first
boot.

What disappears with the channel: the bridge core, the
translation layer, the systemd units, the raise, and the
proc verbs as a conversation. What replaces them: direct
calls at the same centers. The proc state read stays,
as a window: it reports on the engine, it is not a
source of anything.

Registration in this build is an in-kernel act. The
registry the layer kept across the channel becomes
bookkeeping beside the doors, written in process context
under the control path's confirm-then-commit (R2), with
the Task 8 rollback shape carried in-process: nothing is
published until the whole act has succeeded, and a
failed act leaves the prior state standing.

## 2. The marked mass

A generator, run at build time from the pinned build
constants, emits the engine's initial state as an
assembly/data file: one contiguous block holding the
per-cube tables (sector words, row and plane counters)
and the beam state, every sector Nothing, fixtures and
reserved regions placed by constant and marked
unavailable, the counters born agreeing with the marks.
Because no loop reconciles them, the mass must be
self-consistent as emitted; that consistency is a desk
check on the data itself (sum the marks, compare the
counters), and the mass can be dumped from the linked
binary without booting.

The arena itself is a named section of the kernel image:
its marked head as initialized data, the remainder
declared, filesz far below memsz, born zero, and zero is
Nothing. This retires the boot-parameter span and its
arithmetic together: there is no command-line claim for
the canary to witness, only the linker's placement to
verify (section 5). The channel-overlap class of error,
which cost a blackboard catch and would have been fatal
at first traffic, has no representation here.

## 3. The atomic discipline at the doors

Each alloc and free door wraps its engine call in the
irqsave section. The section's worst case is the
termination table (section 4), and its interior is
sleep-free by construction and by audit: the symbol
audit run on the harness objects on 2026-11-19 found no
allocator, I/O, syscall, or wait external inside the
boundary, and the same audit is a build gate here,
re-run on the kernel objects.

Atomic-context callers, which the bridge refused, are
served: the section is spin-guarded and nothing inside
it can sleep, so the refusal has no remaining reason.
NMI context remains outside the contract, as it is for
stock (R6).

## 4. Termination table (measured, 2026-11-19)

From the atomicity simulation (atomic-sim/REPORT.md),
one cube, steps as defined there (one scanning-loop
iteration: a row, plane, sector position, or cube
examined):

| class | vectors, worst | trace, worst | exhaust, failure |
|---|---|---|---|
| Nano (1-8) | 86 | 403 | 1,056 |
| Tiny (9-32) | 120 | 61 | 1,056 |
| Small (33-64) | 149 | 204 | 1,055 |
| Large (65-1024) | 253 | 153 | 4,035 |
| Huge (1025-32767) | 3,255 | (none) | 49 |
| CubeCheck (32768) | 2,104 | (none) | 0 |

The largest window measured anywhere is 4,035 steps.
The largest successful window is 3,255 (a Huge request
on an empty cube, 2,104 of it the commit walk). A beam
call tries at most four cubes: the composed bound is
about 18,000 steps, against a worst observed beam call
of 403 steps in the full boot replay (812,106
allocations, 27,883,845 steps in total). Frees cost the
commit walk of the block's interior: at most 2,104
steps (a whole-cube unmark), 12 in the trace.

History-independence, the R4 precondition, was checked
in the same simulation: two histories reaching the same
state gave byte-identical state and exactly equal
per-probe steps and pointers. The one measured
qualification is the Small gear's rotate cursor, which
moves Small probes by at most 35 steps after churn,
inside its 1,055 ceiling.

Still owed, and named here so the table is not read as
complete: the per-step cost on telesart (time a
known-step call, divide), and the analytic per-gear
bound as desk work. The measurements are consistent
with the analytic shape (each gear's sweep is at most
32 by 32 examinations, Large's two passes and window
sums on top), but consistency is not the proof.

## 5. Instrumentation in the build

The canary, transformed per R7: at init, the witness
mass is reached by two independent paths (offset
arithmetic, and the page path), and the two are
compared before ready opens. Disagreement keeps the
engine shut, stock serves, and the refusal prints both
addresses. The canary verifies a mass that was born
marked (R8); it is a check on the linker and the
generator, not on a fill loop that never runs.

The prefix counters run until ready, counting the calls
stock served (R11). The state read publishes the engine
state, the counters, and the closure state in the
bridge's three-way form (closed, ready, prefix).

The latency ledger stands at the doors: cycle counts
sampled at door entry and exit, the worst per class
kept, the walk itself untouched. Divided into
known-step calls, it yields the per-step cost that
closes R5, measured on the bench rather than asserted.

Desk instruments carried over: the link check, extended
with mass consistency and section placement (the span
identity check retires with the channel); the admission
vectors replayed against both builds (R9); the
termination and coverage tables as standing documents.

## 6. Verification order

No step skipped (R1), each with its check:

1. The generator and the mass: desk check on this VM
   (marks summed against counters; the mass dumped from
   the linked image and compared).
2. The engine text admitted in the harness (existing
   vectors, empty diffs), and the kernel objects pass
   the sleep-free audit.
3. The coverage audit closed against the tree (R10).
4. First boot: canary passes, ready opens, the state
   read is exact.
5. First served allocation; the exerciser pattern
   replayed in-kernel.
6. The degrade proof in its kernel form: the engine
   held shut, stock serves, the system lives.
7. The per-step cost measured at the bench; the latency
   budget closed (R5); Paper 3's verdict criterion
   applied (direct replacement, and runs without a
   panic, both binary).

## 7. Carried on the record, not carried into the build

The bridge visibility defect (D0, 2026-11-18): the
layer answered both registrations completely and
correctly, and the kernel's five-second poll never
observed the response inside its window. Candidate
causes on file, in order: the two
mappings of the channel page differing in cacheability;
the poll's busy wait starving the process it waited
for; timer and tick behavior in the window. Unfixed,
and unfixable in place now that the rig is retired. Its
class cannot occur in this design: there is no second
actor, no channel, and no observation window.

The forensic boot's layer death after serve 32
(2026-11-18) remains unnamed on its own terms. It was
the death of a userspace process, a category this build
does not contain.

The userland rig's positive residue is the method note
(paper-3-userland-method-note-2026-11-18.md): three
design errors convicted in userland at the cost of
rebooting a test guest, each of which would have
presented in-kernel as silent corruption or a hang. The
rig was the instrument panel; the panel is read, and
put away.
