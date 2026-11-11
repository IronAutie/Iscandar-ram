# Composition ledger: Debian to Iscandar: Full Steam Ahead!

## Subtitle: Installing the RAM

## Bibliographic record

- **Title of record:** Debian to Iscandar: Full Steam Ahead!
- **Subtitle:** Installing the RAM
- **Author:** Andrew Jonathan Fine
- **Dedication:** To Jennifer: Everything. Always. Forever.
- **Repository:** Iscandar-ram
- **Status:** Composition started 2026-11-08. No live patched-Debian result is claimed yet.
- **Prior title superseded:** Debian's Voyage To Iscandar

## Statement of the Problem (dictated by Andrew, 2026-11-08)

> In the prior two papers, [1] and [2], we have devised a deterministic memory allocation and then shown it able to carry a typical boot-time page load on an embedded version of Debian. In this paper we demonstrate the techniques needed to associate the memory allocations with real random-access memory so that our example, Fubuki, can run actual applications at full speed.

## The paper in one paragraph

The first Iscandar paper established the deterministic allocator theory at k=3. The second extended the theory to k=4 and replayed a complete recorded Debian boot through the allocator. This third paper takes the step from replay to live execution: identify the allocator centers in the Debian kernel, prepare a checked patch plan, boot Debian automatically into a controlled page-exercising application, and then run Debian with its allocation path redirected through Iscandar under QEMU.

That paragraph states the intended arc. It is not a results claim.

## Evidence already in hand

### SETTLED: the earlier papers

- Paper 1: *An Extension of Buddy Systems to Arbitrary Radix for Deterministic Memory Allocation*. Zenodo DOI: 10.5281/zenodo.23748412.
- Paper 2: *Iscandar Revisited: Expansion to k=4 Theory For Running Debian*. Zenodo DOI: 10.5281/zenodo.24044354.

### MEASURED: the Fubuki boot trace

Fubuki is the ELBE-built Debian 13 system used as the test subject. Its recorded boot trace contains 1,650,424 entries:

- kmalloc: 105,687 allocation events
- kmem_cache_alloc: 706,419 allocation events
- Total allocations: 812,106
- kfree: 166,390 free events
- kmem_cache_free: 671,928 free events

The trace is the control for deciding which allocator families the exercised boot actually used. It armed the kmem family. It did not arm the vmalloc or page-direct families.

### MEASURED: the source sift

The Linux 6.12.116 source tree was searched textually across 59,969 C and header files. The principal counts are:

- kmalloc: 5,501
- kmalloc_node: 67
- kmalloc_array: 1,036
- kzalloc: 21,808
- kcalloc: 4,806
- kmem_cache_alloc: 329
- kmem_cache_alloc_node: 24
- kmem_cache_create: 464
- kfree: 40,597
- kvfree: 1,982
- kmem_cache_free: 963
- alloc_pages: 581
- __get_free_pages: 175
- free_pages: 1,107
- vmalloc: 924
- vzalloc: 976
- vfree: 3,436
- devm_kzalloc: 6,897
- dma_alloc_coherent: 1,872
- alloc_percpu: 291
- mempool_alloc: 300

The sift finds what can be called. The trace shows what the recorded boot did call. Neither replaces the other.

### SETTLED AS A DRAFT: the patch points

The patch-point list is in `paper-3-patch-points-DRAFT.md`. It is scoped to allocator centers, not individual call sites.

- Slab and kmalloc core: 6 patch points.
- vmalloc: the center, the backing step, the free door, and the virtual-range reservation expressly left unpatched.
- Page allocator: 2 patch points, held as second-ring decisions.
- Side centers: mempool, percpu, genalloc, and CMA, each with its scope stated.
- Out of runtime scope: memblock, vmap, and ioremap.

The list was checked against Linux 6.12.116. Fubuki's built kernel package is Linux 6.12.113+deb13. The compiled-subset enumeration must therefore use the stock 6.12.113 configuration and state that version plainly.

### MEASURED: Andrew's dye test

Question: if an allocator helper is inlined and leaves no symbol in the compiled object, can its compiled copies still be found?

Method: insert a distinctive 16-byte pattern into the body of `slab_alloc_node` in `mm/slub.c`, insert a different control pattern into the non-inlined `kmem_cache_alloc_noprof`, compile `mm/slub.o`, and scan the object bytes.

Result:

- `slab_alloc_node` has 6 call sites in the source.
- Its symbol is absent from the compiled object.
- Its dye appears 8 times in `.text`, once for each final inlined expansion.
- The control dye appears exactly once.
- An object built from an unmodified file in the same build contains neither pattern.
- The 8 rather than 6 count is fully accounted for: one source call site sits inside `__do_kmalloc_node`, itself always inlined, and that function expanded into three wrappers. Five direct copies plus three nested copies equal eight.

Conclusion supported by the measurement: a dye count measures final compiled expansions, not source call sites, and each compiled copy can be assigned to the function that contains it.

## Work now in preparation

### IN PREPARATION: automatic boot into a page exerciser

Purpose: produce the missing page-family evidence under a controlled application rather than infer it from the kmem trace.

Protocol:

1. Boot the guest without interactive login.
2. Start a deterministic page-exercising application as a boot service.
3. The application performs a fixed program of page allocations, touches, holds, releases, and churn cycles.
4. The application writes its own ledger of requests, sizes, order, and releases.
5. Arm the page allocation tracepoints from the beginning of boot.
6. Export the trace and the application ledger through the existing finish path.
7. Reconcile the trace against the application ledger.
8. Shut the guest down without manual intervention.

Menu requirement (Andrew, 2026-11-09): the exerciser opens with a menu on the console and waits 10 seconds; on timeout it proceeds with the default, the standard exercise run. Menu shape as recorded: 1) standard exercise (the default), 2) extended exercise with churn, 3) ledger check only, 4) exit without exercising. The timeout matters because the automatic boot is unattended: the menu is for the attended rehearsals, and the default is what the evidence run uses.

The first complete rehearsals can run in a stand-in guest on Muse's VM. The evidence run for the paper must use the ELBE-built Fubuki image on telesart, preserving the image lineage used for Paper 2.

### NOT YET RUN: the compiled-subset enumeration

The stock Debian configuration for Linux 6.12.113+deb13 is obtainable from the published trixie package. The enumeration will mark every patch point as compiled in, modular, or absent for Fubuki's kernel. No telesart sitting is required for this step.

### NOT YET BUILT: the live interposition

Still to build:

- The virtual-to-actual pointer translation layer in user space.
- The kernel patch at the selected centers.
- The bridge from the patched system to the Iscandar arena.
- The patched Debian run under QEMU.

No sentence in the paper may describe these as working until the corresponding run has been measured.

Proposed eleventh step (Andrew, 2026-11-09, tentative): Demonstration. After the audit, the patched Fubuki is shown running actual applications at full speed, which is the payoff the Statement of the Problem promises. If Andrew confirms it, the Introduction's enumeration grows to eleven steps and a Demonstration section follows the Audit, before the Discussion. Not yet in the manuscript.

## Decisions still required from Andrew

1. Page allocator: substrate under the redirected centers, or bypassed inside a boot-carved Iscandar arena?
2. Oversize kmalloc: patch `__kmalloc_large_noprof` separately, or record page-allocator backing as the design?
3. Named caches: how are cache geometry, alignment, and constructors represented when the slab center is redirected?
4. Second ring: which of mempool, percpu, genalloc, CMA, and DMA belong in the first live run?
5. Early boot: must the arena stand before the first redirected call, or may the earliest calls pass through stock code and be logged?
6. Page exerciser: the exact allocation pattern and size classes.

## Section plan

### 1. Introduction

Move from the replay result to the live-system question. State what this paper attempts and what it does not attempt.

Inserted 2026-11-08 at Andrew's direction: the overview list of our techniques, applied in order upon the source code making up Fubuki and upon the ISO under test. The order is: sift, control trace, reconciliation, compiled subset, dye, patch at the centers, translation layer, rebuilt ISO, automatic boot, audit.

### 2. Prior results

Summarize the k=3 theory, the k=4 extension, and the Fubuki replay only as far as the live experiment depends on them.

### 3. The step sections

Andrew's ruling (2026-11-08): each step gets its own section following the Introduction's overall description; the earlier Three instruments grouping is dissolved. First placed: **The Sift, and its Products**, whose first paragraph Andrew dictated on 2026-11-08 from the blackboard text. Remaining step sections, in order: the control trace, the reconciliation, the compiled subset, the dye, the patch at the centers, the translation layer, the rebuilt ISO, the automatic boot, the audit.

Andrew's ruling (2026-11-10): the Sift section stands as its five-item outline. The itemize is the section's settled form, not a placeholder for prose.

Andrew confirmed (2026-11-10): the Census section likewise stands in its itemized form, paragraph plus the math node with its statistics itemized, and is in the paper as it stands.

Andrew's decision (2026-11-10), deferred by his own instruction: a Control Trace section will be added to the manuscript before the Patch Points section. Not yet: do not insert it until he says so. The control trace draft file remains the source for it, still awaiting his stet.

Drift found and repaired (2026-11-10): Andrew had removed the Census section's math node in Overleaf (its indents differed from the rest of the paper), leaving the statistics in a plain itemize; the repo copy still carried the display block. Checked in Overleaf at Andrew's direction, then the repo copy was synced to the Overleaf form at his go. Consequence for the control trace draft: its open question about a math node for the trace statistics is settled by this precedent. Trace statistics take a plain itemize, no math node.

### 3a. Three instruments (superseded grouping)

Present the completeness method:

- The sift reads the source.
- The trace reads the running system.
- The dye reads the compiled artifact.

Each instrument audits a domain the other two cannot settle alone.

### 4. Patch points

Present the allocator centers by family, the doors deliberately left stock, the side centers, and the out-of-scope boundary.

### 5. The dye test

Give the question, the marker method, the control, the measured counts, and the nested-inline accounting.

### 6. Automatic boot and the page exerciser

Describe Fubuki, the boot service, the deterministic application, its ledger, the armed tracepoints, and the export path.

### 7. Live interposition

Describe the translation layer, the patched centers, arena construction, and boot ordering. This section remains an outline until the system runs.

### 8. Measurements

Quality bar (Andrew, 2026-11-09): no embarrassment when the user sees it. The rehearsals exist to spend the surprises in private; what reaches a reader or a user has already been run, reconciled, and explained.

Prediction requirement (Andrew, 2026-11-09): the exerciser emits a prediction sheet before it runs, computed from its fixed program alone: expected allocations and frees by size class, expected peak live pages, expected end state. The audit then prints predicted against actual, quantity by quantity, and every difference is either zero or explained in the text. The prediction is written before the run and not edited after it.

Quantities to fix before the live run:

- Boot completion
- Allocation and free counts by family
- Trace events with no matching ledger or source site
- Source sites that never fire
- Refusals and anomalies
- Peak live objects
- Peak live pages or sectors
- Largest free run at stated checkpoints
- End state reconciliation

### 9. Discussion

Determinism, exact fit, bounded behavior, failure refusal, and the limits of the demonstration. Timing from a software-emulated rehearsal must not be presented as performance evidence.

### 10. Conclusion

To be written only after the live run. No conclusion claim is drafted in advance.

## Composition rule

Every factual paragraph in the manuscript must point back to one of four labels in this ledger: SETTLED, MEASURED, IN PREPARATION, or NOT YET RUN. A paragraph without a label stays out of the manuscript.

## Resumption point (Andrew wrapped up, evening of 2026-11-08)

Manuscript state, identical in the repository and in Overleaf (5 pages, compiles clean): title and subtitle set; Statement of the Problem as dictated; Introduction with the ten techniques enumerated; Prior Results with one paragraph per paper; The Sift, and its Products (itemized opening); The Census, a Byproduct of Earlier Research (paragraph plus its statistics in a plain itemize). Remaining sections are headings awaiting their turns, in step order: the control trace, the reconciliation, the compiled subset, the dye (results already measured), the patch at the centers (list already drafted), the translation layer, the rebuilt ISO, the automatic boot, the audit.

Engineering still open, in his order of business: the compiled-subset enumeration can run on Muse's VM from the stock trixie config whenever he gives the word; the VM-side rehearsal of the automatic boot and page exerciser likewise. The telesart run on Fubuki herself comes last.
