# The Iscandar page exerciser, as a boot-up application

The exerciser is the controlled workload of Paper 3's automatic
boot step: a fixed program of page allocations whose every count
is known in advance, so the run can be audited against a
prediction sheet computed from the program alone. The pattern
of record is `../paper-3-exerciser-pattern.md`.

## What it does at boot

Installed and enabled, `iscandar-exerciser.service` runs the
program once at boot, after the system reaches multi-user, with
its console on /dev/console. The program opens with the menu
(10 second timeout): the standard run is the default, the
extended run (more churn) is the second option, ledger check
only the third, and exit without exercising the fourth. The
evidence boot is unattended, so the timeout takes the standard
run; the menu exists for the rehearsals. The program's ledger
is appended to /var/log/iscandar-exerciser/ledger.txt and echoed
to the console and the journal.

Two workloads share the evidence boot: the system's own
startup, uncontrolled, and the exerciser, controlled to the
last allocation. A separate finish service on the evidence rig
orders itself after this one; it exports the trace and the
ledger and shuts the guest down. That service belongs to the
rig, not to this package.

## Files

- `exerciser.c`: the program, one file, libc only.
- `iscandar-exerciser.service`: the systemd unit.
- `Makefile`: build, check (ledger check plus a full standard
  run, both against ordinary memory on the build machine), and
  install targets.

## Install

On the guest: `make install` places the binary in
/usr/local/sbin, the unit in /etc/systemd/system, and enables
the service. In the ELBE build of the Fubuki image, the same
three files are added to the image's file set and the unit
enabled in the image, preserving the image's lineage; nothing
in the program depends on how it was installed.

The mode may also be passed directly, bypassing the menu:
`iscandar-exerciser standard | extended | check | none`.
Exit status: 0 for a completed run matching its prediction,
2 for an allocation failure or a totals mismatch (a finding),
3 for a stamp verification failure (an invariant violation,
recorded in the ledger at the point of failure).
