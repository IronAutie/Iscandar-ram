# Intermediate data for Paper 3

Each section of the paper points to the artifacts here, so the steps
can be recreated from the repository. Status is stated per artifact.

- `census-2026-11-07.md` — the sift census (measured). Points from: The Census.
- `dye-test-2026-11-08.md` — the dye protocol and results (measured). Points from: The Dye.
- `../paper-3-patch-points-DRAFT.md` — the patch-point list (draft). Points from: Patch Points.

- `fubuki-kernel-config-6.12.113-deb13-amd64.txt` — Fubuki's kernel
  configuration (added 2026-11-10 at Andrew's direction). Fubuki's
  kernel is the stock trixie package, unmodified, and its ELBE
  build exports this config as the `telezart-kconfig` artifact.
  Provenance of this copy: extracted from the Debian package
  `linux-image-6.12.113+deb13-amd64` (version 6.12.113-1) fetched
  from snapshot.debian.org, package file SHA-1
  a82b3cd8e26b5fd2b6564e83d170b7deafaf2c4c; the config file's
  SHA-256 is
  4ea0eceb2fe0b9c301fb4e43dc76b4cd700b6edea6aa30b6fba97c9111006796.
  It has not yet been cross-checked byte for byte against the
  telesart artifact itself. Points from: The Compiled Subset.

Not yet present:
- The rerunnable sift and enumeration scripts, to be added with that run.
- The Fubuki boot trace is not duplicated here: it lives in the
  iscandar-k4 repository (data/telezart-trace-boot-complete-2026-11-03.txt.gz,
  SHA-256 8b2a07f7926585c5bdaf28e3ea2da456a36c16222729478f53bcef28122d06d2).
  Sections point across to it.
- The page-exercise ledger format, to be added when the exerciser is written.
- The prediction sheet (Andrew, 2026-11-09): the exerciser's predicted counts, peak, and end state, written before the run; the audit's predicted-versus-actual table is built from it.
