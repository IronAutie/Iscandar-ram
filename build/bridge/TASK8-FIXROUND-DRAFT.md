# Task 8 draft: the fix round (registration rollback,
# layer-death degradation, unit web, span assertion)

Status: RULED as drafted and BUILT here, 2026-11-18
late evening (Andrew: "let's build"). Patch
0008-fix-round.patch (D1, D2) is written and verified
by fresh application on a reconstructed post-0007
core, byte-identical, 551 to 638 lines; the unit web
(D3) is edited in the unit files, the exerciser's file
of record, and the ELBE XML (including the
wants-symlink farm, which was the finish unit's real
pull-in); link-check carries the D3 asserts inside
boot-order and the span identity as its twelfth check,
and stands 12/12 here. Not yet compiled against the
kernel tree and not yet benched: the bench round runs
D0 first, then the negative, the true boot, and the
degrade proof. Nothing committed; the working tree
awaits Andrew's word. The text below is the draft as
ruled, kept for the record. Written after the
forensic boot of 2026-11-18 (see BENCH-RUN-SHEET.md,
"Bench findings, 2026-11-18 late evening"): the bridge
served 32 cache registrations, the layer process died
after completing serve 32 and before serving 33, the
ready flag stood with 32 caches committed, and the
system starved on five-second bridge timeouts returning
NULL. The kernel-side facts below are read from the
patches as built, not from design memory.

## What the code says today

- `iscandar_raise` (patch 0006) sets
  `WRITE_ONCE(iscandar_ready, true)` BEFORE enumerating
  `slab_caches`, counts registration failures and walks
  on, and never clears ready on failure. A failed raise
  leaves the bridge open for business with a partial
  registry and, tonight, a dead server.
- `iscandar_bridge_call` (patch 0005) refuses with
  -ENODEV unless ready stands, so the registrations
  themselves need the flag the alloc doors use. There
  is no separate raise window.
- `iscandar_cache_unregister` exists (patch 0005) and
  is unused by the raise path. Rollback has its tool.
- The finish unit carries both
  `After=iscandar-exerciser.service` and
  `WantedBy=multi-user.target`, which is the ordering
  cycle Boot C's journal showed (the exerciser's start
  job deleted) and which let the finish unit start
  alongside the layer, ungated.

## D0: name the killer first (diagnostic, not design)

Before any fix is built, one bench re-run in the
posture that makes deaths speak: layer-only masks, the
layer started by hand in the foreground on the console
instead of as a service, the raise written by hand.

Capture comes first, and it is a host-side change, not
a guest change. Tonight the kernel stacks and watchdog
banners existed only on a flooding screen and in a
RAM ring buffer that died with the guest; the journal
persisted nothing past the layer's first breath. So
the QEMU launch spells its serial chardev out instead
of using the -nographic shorthand, with two changes:
the chardev carries a logfile, so every byte the guest
prints lands in a file on telesart that survives the
guest, the emulator, and the flood; and the monitor
moves to its own unix socket, so the guest's output
can never again stand between the operator and the
monitor prompt. In shape:

```
-chardev stdio,id=ser,logfile=console-<boot>.log
-serial chardev:ser
-monitor unix:/tmp/fubuki-mon,server,nowait
-display none
```

The monitor is then reached from a second terminal
with `socat - UNIX-CONNECT:/tmp/fubuki-mon`. Guest
interaction is otherwise unchanged. The first read
after a death is a grep of the captured log on the
host (segfault, out of memory, killed process, soft
lockup, blocked-for-more-than, rcu stall, iscandar),
with `dmesg` in the guest as the same witness read
live when a shell survives. The re-run also shows the
raise thread's fate, the one fact tonight left
unexplained (publication froze at req_tail 33, so the
enumerating thread stopped too). D0's result can
change D2's shape; the rest of the round does not wait
on it.

## D1: raise semantics, confirm-then-commit

Recommendation: a separate raise window.

- A new core flag, `iscandar_raising`. `bridge_call`
  admits a call when ready OR raising stands; the
  alloc doors continue to admit only on ready.
- `iscandar_raise` sets raising, enumerates, and on
  the FIRST registration failure aborts: it unrolls
  by calling `iscandar_cache_unregister` for every
  cache registered in this pass, clears raising, and
  returns the error. Ready never stood; the system is
  in prefix posture, untouched.
- On full success it clears raising and sets ready.
  Only then do the doors open.

Options: (A) as above. (B) Keep enumerating past
failures and clear ready at the end if any failed:
still minutes of five-second timeouts against a dead
layer during the walk; rejected. (C) Partial registry
stands (tonight's behavior); rejected by the evidence.

Safety of the unroll: during the raise window the
doors never opened, so no cache in the pass has served
a single allocation. Unregistering them frees nothing
and strands nothing.

## D2: layer death after a successful raise

The bridge must degrade to stock, never to NULL.
Recommendation: a consecutive-timeout trip in
`bridge_call`, plus a deliberate leak rule for frees.

- `bridge_call` counts consecutive timeouts; any
  success resets the count. At three, the core clears
  ready (a WRITE_ONCE, taken under no new lock, so the
  lock order against slab_mutex is never inverted) and
  prints the closure. The doors then take stock paths
  for every new allocation.
- The kernel registry entries are left standing but
  inert: the doors consult them only under ready.
- Frees are the hard edge. An object born in the arena
  must never be handed to the stock free path, which
  would return an arena page to the buddy allocator
  (double ownership, corruption). So the free doors
  keep their arena test independent of ready: an arena
  pointer with the layer gone is deliberately leaked,
  counted in the bridge read, and never freed. Bounded
  leak of whatever was live at the death, in exchange
  for never corrupting stock. The arena is reserved
  memory; leaked pages cost capacity, nothing else.

Options: (A) as above. (B) A heartbeat word the layer
bumps each poll, checked by the doors: more machinery,
a timestamp read on allocation paths, and a race at
the death boundary; rejected. (C) A systemd trigger on
the layer unit's death writing stop: the write travels
through the very system that is starving; rejected.

## D3: the unit web

- `iscandar-finish.service` loses
  `WantedBy=multi-user.target` and keeps
  `After=iscandar-exerciser.service`.
- `iscandar-exerciser.service` gains
  `Wants=iscandar-finish.service`, so the finish unit
  is pulled in by the exerciser alone and ordered
  after it. The target can no longer reach the finish
  unit around the exerciser, and the cycle
  (finish, exerciser, target, finish) cannot form.
- Link-check asserts both halves: no WantedBy in the
  finish unit, the Wants present in the exerciser.

## D4: the span identity, asserted

Link-check gains a twelfth check, computed from the
same build constants that generate the boot line:

channel base + channel bytes == arena base

The 2026-11-18 channel error (a 96-page overlap,
latent in prefix posture, fatal at first traffic)
becomes a build failure instead of a bench discovery.

## Shape and acceptance

- Patch 0008 against the bridge core (D1, D2), unit
  file edits (D3), link-check extension (D3, D4). The
  shared header, the control block layout, the canary
  protocol, and the engine text are untouched.
- Desk acceptance: link-check 12/12; the layer
  self-test unchanged at 847 checks, 0 failures (the
  layer's serving code does not change in this round).
- Bench acceptance, in order: D0's foreground re-run;
  the negative boot repeated; the true boot; and a new
  negative, the degrade proof: after a successful
  raise, kill the layer by hand (kill -9) and the
  system must stay alive on stock memory, the bridge
  read showing the closure and the leaked-grant count.

## Carried open

Why publication froze at req_tail 33 tonight (the
raise thread's own fate), and the layer's killer.
D0 exists to answer both before the fixes are tuned.
