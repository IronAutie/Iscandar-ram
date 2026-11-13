# Intermediate data for Paper 3

Each section of the paper points to the artifacts here, so the steps
can be recreated from the repository. Status is stated per artifact.

- `census-2026-11-07.md` — the sift census (measured). Points from: The Census.
- `control-trace-2026-11-03.md` — the control trace ledger (measured): event counts, completeness counters, size distribution, and the trace's location and hashes. The raw trace itself is not duplicated here; see the pointer in that file. Points from: The Control Trace.
- `compiled-subset-2026-11-10.md` — the compiled-subset enumeration (measured): every patch point marked compiled in, modular, or absent against Fubuki's configuration (the enumeration was read 2026-11-10 against the 6.12.113 stock package config; Fubuki's build config is 6.12.107, byte-checked 2026-11-12; see the correction note in the artifact), with gating evidence from the 6.12.116 Makefiles. Points from: The Compiled Subset.
- `reconciliation-2026-11-10.md` — the reconciliation (run 2026-11-10): the sift, the control trace, the compiled subset, and the dye read against each other family by family under Andrew's rulings of 2026-11-10, naming the center each family will be served at and the families it will not, each by name. A comparison of the existing artifacts; no new measurement. Points from: The Reconciliation.
- `dye-test-2026-11-08.md` — the dye protocol and results (measured). Points from: The Dye.
- `../paper-3-patch-points-DRAFT.md` — the patch-point list (draft). Points from: Patch Points.

- `fubuki-kernel-config-6.12.113-deb13-amd64.txt` — the stock
  trixie package configuration (added 2026-11-10 at Andrew's
  direction, then taken as Fubuki's own configuration; that
  identification was corrected on 2026-11-12: Fubuki's kernel is
  the stock trixie package 6.12.107+deb13-amd64, unmodified, and
  its ELBE build's own exported `telezart-kconfig` is the
  6.12.107 config, held on telesart in the ELBE build
  directories, SHA-256
  9b460f1003db4d417288e9c14caecfa14245dab07186df948fef0f3661e38ceb).
  Provenance of this copy: extracted from the Debian package
  `linux-image-6.12.113+deb13-amd64` (version 6.12.113-1) fetched
  from snapshot.debian.org, package file SHA-1
  a82b3cd8e26b5fd2b6564e83d170b7deafaf2c4c; the config file's
  SHA-256 is
  4ea0eceb2fe0b9c301fb4e43dc76b4cd700b6edea6aa30b6fba97c9111006796.
  The cross-check was run on 2026-11-12 and the two copies
  differ, as the corrected identification above records.
  Points from: The Compiled Subset.

Not yet present:
- The rerunnable sift script, to be added when the sift is next
  rerun. (The compiled-subset enumeration of 2026-11-10 was a
  Makefile-and-config reading, done by hand and recorded in
  `compiled-subset-2026-11-10.md`; no script was used or is
  owed for it.)
- The Fubuki boot trace is not duplicated here: it lives in the
  iscandar-k4 repository (data/telezart-trace-boot-complete-2026-11-03.txt.gz,
  SHA-256 8b2a07f7926585c5bdaf28e3ea2da456a36c16222729478f53bcef28122d06d2).
  Sections point across to it.
- The page-exercise ledger format, to be added when the exerciser is written.
- The prediction sheet (Andrew, 2026-11-09): the exerciser's predicted counts, peak, and end state, written before the run; the audit's predicted-versus-actual table is built from it.
