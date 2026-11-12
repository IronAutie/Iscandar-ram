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

### MEASURED: the compiled-subset enumeration (run 2026-11-10)

Run at Andrew's direction, on his order-of-execution ruling: the compiled subset comes before the dye in the evidence chain, because which wrappers exist in Fubuki's build is a configuration question. Method: each patch point mapped to its gating CONFIG in the 6.12.116 Makefiles, the symbols read from Fubuki's 6.12.113 stock config (fetched from the trixie package the same day). Artifact: `data/compiled-subset-2026-11-10.md`. Headline: every first-ring center is compiled in; nothing in the set is modular; the single absence is `cma_alloc` (CONFIG_CMA unset in the stock trixie amd64 config), which makes the design's CMA question moot for Fubuki. The dye test's conditions (SLUB=y, SLUB_TINY unset) match Fubuki's configuration class, so its expansion counts transfer.

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

Executed (2026-11-10): at Andrew's direction the Control Trace section was laid down in the manuscript before Patch Points, in the itemized style of the Sift section, using the draft's five-item candidate opening verbatim. The trace statistics itemize is not yet in the section; the checked numbers stand in the draft file. Pushed to Overleaf the same day at Andrew's go, after a source download and diff verified the two copies differed only by this section and comment bookkeeping: Overleaf recompiled clean, 0 errors, 5 pages, Control Trace standing as section 5. Repo and Overleaf match again.

Drift found and repaired (2026-11-10): Andrew had removed the Census section's math node in Overleaf (its indents differed from the rest of the paper), leaving the statistics in a plain itemize; the repo copy still carried the display block. Checked in Overleaf at Andrew's direction, then the repo copy was synced to the Overleaf form at his go. Consequence for the control trace draft: its open question about a math node for the trace statistics is settled by this precedent. Trace statistics take a plain itemize, no math node.

Access decision (Andrew, 2026-11-10): this VM does not join his Tailscale network yet. He is not ready for it; the join happens during the actual test (the automatic boot rehearsal). Until then, telesart-side material moves by the kit route or by figures Andrew reads off that machine, such as the telezart-kconfig checksum.

Section 6 added (2026-11-10): The Reconciliation now stands in the manuscript between The Control Trace and Patch Points, in the itemized style, after Andrew was shown the draft and directed the addition. Its status note in the manuscript states, at Andrew's correction the same day, that the section is a prediction for the later run of the reconciliation step: both input ledgers are measured, and the section states in advance what the run will do and find. Pushed to Overleaf 2026-11-10 at Andrew's word, after a diff verified the copies differed by this section alone: recompiled clean, 0 errors, 6 pages, copies match again.

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

Addendum 2026-11-10 (later): the Compiled Subset section was drafted, shown to Andrew in chat, inserted as section 7 at his direction (commit 619e1f0), and pushed to Overleaf at his go after a source download diffed section 7 as the only difference. Overleaf verifies: outline shows The Compiled Subset between The Reconciliation and Patch Points; compile clean (0 errors, 0 warnings); paper still 6 pages. Repository and Overleaf identical. Patch Points is now section 8 and The Dye Test section 9.

Correction (Andrew, 2026-11-10): the patch-at-the-centers method is to be described as arrived at as a thought experiment, not as blackboard work. The Patch Points section draft shown this day is amended accordingly before insertion: its method item reads "arrived at as a thought experiment" in place of any blackboard attribution.

Addendum 2026-11-10 (later still): the Patch Points section was drafted, shown to Andrew, and inserted as section 8 at his direction (commit d1172e7). It carries the thought-experiment provenance for the method per his correction. Pushed to Overleaf the same evening at his go, after a source download diffed the section as the only difference; verified there: outline shows Patch Points with content between The Compiled Subset and The Dye Test, compile clean, paper now 7 pages. Repository and Overleaf identical.

Decision (Andrew, 2026-11-10): the patch and translation layer build is left for later. The gate is his three open design rulings (page allocator substrate or bypass; oversize kmalloc; cache geometry at the funnel) and the reconciliation run as a formal step, in that order. No build work is to start until he returns to it.

Addendum 2026-11-10 (evening): the Dye Test section was drafted from a discussion in which Andrew stated the experiment's provenance (proposed by the author, from experience debugging embedded systems) and its purpose (locating potentially missing gates between header-carried source and the executable, down to the least particular), approved the explicit gates formulation, and directed insertion as section 9. Pushed to Overleaf at his go after a diff showed the section as the only difference; verified there: outline shows The Dye Test with content between Patch Points and Automatic Boot and the Page Exerciser, compile clean, paper now 8 pages. Repository and Overleaf identical.

Addendum 2026-11-10 (evening): the Automatic Boot and the Page Exerciser section was drafted, shown to Andrew, and inserted as section 10. At his choice, the section stands on the exerciser pattern's shape only; the exact counts enter the paper later as part of the prediction sheet, and the candidate pattern remains unapproved in its draft file. The step itself is designed, not executed, and the ledger records his instruction that it not be executed yet. Pushed to Overleaf at his go after a diff showed the section as the only difference; verified there: outline shows the section between The Dye Test and Live Interposition, compile clean, paper remains 8 pages. Repository and Overleaf identical.

Addendum 2026-11-10 (evening): the Live Interposition section was discussed and inserted as section 11, as design only, carrying the standing discipline that nothing in it is described as working. The build remains deferred per Andrew's decision. Not yet pushed to Overleaf.

Discussion criterion (Andrew, 2026-11-10): the main criterion is fixed in advance, in two binary halves: is this a direct replacement, and does it run without a panic? Speed and paging coverage are NOT critical unless they contribute to a panic. They are recorded in Measurements but carry no verdict of their own; the only path by which they can affect the outcome is through the panic half of the criterion.

Design decision (Andrew, 2026-11-10): the Python engine code is to be translated into C in both the k=3 and the k=4 sections, the C living outside the Python library, to keep the allocation kernel transparent. The translation follows the standing admission rule: C is admitted only where it reproduces the Python and paper oracle outputs exactly; the Python remains the oracle and the offline tooling. Open reconciliation: section 11 as inserted describes the translation layer in user space, per the earlier ledger design; whether this decision also moves the translation layer into the kernel is awaiting Andrew's word, and section 11 is not to be edited on that point until he gives it.

Ruling (Andrew, 2026-11-10): the open reconciliation above is resolved in favor of the earlier design. The translation layer stays in user space. Andrew's standing preference, stated the same day: he always prefers user space runtime tools where available, and he ran device libraries in userland deliberately in his earlier engineering. Section 11 therefore stands as inserted, with the translation layer in user space; the C translation decision covers the engine code in the k=3 and k=4 sections, outside the Python library.

Ruling (Andrew, 2026-11-10): the reclassification offered above is declined. In his words: Python speaking mmap is too slow for a memory allocator. The mmap protocol point stands as a fact about the interface, but it does not settle the serving rate, and the C translation decision of this same day stands as originally recorded: the engine, in both the k=3 and k=4 sections, is translated into C outside the Python library; the Python remains the oracle and the offline tooling.

Clarification (Andrew, 2026-11-10): the mmap caller is C. The user-space translation layer is C code, and it is that C code which calls mmap to raise and hold the shared arena. No Python sits anywhere in the serving arrangement, neither as the mmap caller nor as a mediator; the Python's roles are oracle and offline tooling only.

Clarification (Andrew, 2026-11-10): the Python's role is narrower than "oracle and tooling." Python may be used only to GENERATE the oracle tables; it does not USE them here. In the live interposition no Python reads or applies the tables; the C is admitted against the generated tables ahead of time, and at run time the serving arrangement is C end to end.

Work order (Andrew, 2026-11-10): translate the Python into C, then test the k=3 and the k=4 code; nothing further proceeds until that passes. This partially lifts the build deferral for the transcription and its admission tests only: the kernel patch, the bridge, and the interposition build remain deferred behind his rulings and the reconciliation, as recorded. The transcription follows the standing rules of the project: the design is the author's, the C is a transcription admitted only where it reproduces the Python-generated oracle outputs exactly, and Python is used to generate the oracle tables, not to use them in the serving arrangement.

Addendum 2026-11-10 (night): the Discussion section was inserted into the manuscript from the draft shown to Andrew when the criterion was fixed, with one added item at his direction ("the conversion to C needs to be part of the discussion"): the replacement judged is the C transcription of the engine, not the Python model; the Python serves only to generate the oracle outputs against which the C is admitted, and no Python runs in the serving arrangement. The C admission the item rests on is complete: k=3 foundation, request modes, and Engine (7,523 oracle operations, empty diff; per-site pointer codes 20 to 25), and the k=4 Beam re-run over the real k=3 C engine (68-line transcript, empty diff). The verdict item remains blank by design. Andrew directed the push to Overleaf the same night; per his standing guard, the upload carries the Discussion change only, diffed against a fresh Overleaf source download first.
Push verified the same night: Overleaf outline shows Discussion with content between Measurements and Conclusion; compile clean (0 errors, 0 warnings); paper now 9 pages. The upload carried the Discussion change only; Live Interposition remains the one section where the repository is ahead of Overleaf.

Addendum 2026-11-10 (night): at Andrew's direction a Caveats section was inserted between Live Interposition and Measurements, recording the PC rig's concessions: the oracle tables simulated by files mapped read-only through mmap (real system: ROM or flash, as in his 80188 design), and the arena simulated by an mmap raise backed by page-allocator pages (real system: a static memory chip at a fixed address). The section states what the simulation preserves (fixed mapping, read-only tables, uniform lookup, exact fit, refusal semantics) and what it does not (hardware timing), and fixes the claim width accordingly. In the repository only; not yet pushed to Overleaf. The substrate ruling it rests on: Andrew confirmed option two (boot-carved bypass) does not deliver, both because the user-space C raises the arena by mmap and because the centers live in the page world; substrate stands, with the arena raise named as the Discussion's exemption.
Andrew's supporting judgment for the substrate ruling (2026-11-10, night): by using option one we will not miss any allocations. Recorded as stated: with the page allocator standing underneath, every allocation has a server, the redirected ones at the centers and the rest through stock, so the paper's completeness claim (enumerated families, cross-checked by sift, trace, and dye) is preserved; a bypass carve would risk families routing around the arena unnoticed, the same shape as the oversize kmalloc hole the patch-point list already names.
Ruling (Andrew, 2026-11-10, night): for oversize kmalloc, option one. __kmalloc_large_noprof is patched as its own center and oversize requests are served by Iscandar from the arena as exact-fit runs, freed by the inverse bijection at the free door; there is no exemption on that door. Confirmed by Andrew after the option was read back to him ("Let it be so"). With this, two of the three gating rulings are settled: substrate for the page allocator, and oversize kmalloc patched separately. Remaining before the build: the cache-geometry ruling, the early-boot ordering and second-ring scope items, and the reconciliation run as a formal step.
Ruling (Andrew, 2026-11-10, night): for the named caches, choice one. A cache crosses the bridge as a shape of request, size and alignment only; the engine never learns that caches exist, and all cache bookkeeping (geometry table, constructor run once per run on its first grant, RCU quarantine, per-cache reuse of returned runs, per-cache caps) lives in the translation layer. Andrew's framing, recorded as stated: this accommodation is an expansion of Iscandar, and it must be identified as such in a future paper; it is not to be folded silently into the engine's own claims. With this, all three gating rulings are settled (substrate; oversize kmalloc patched as its own center; caches as requests). Remaining before the build: early-boot ordering, second-ring scope, and the reconciliation run as a formal step.
Ruling (Andrew, 2026-11-10, night): for early-boot ordering, the latter. The earliest calls pass through stock and are logged as a named prefix; the arena is not raised before the first redirected call. Grounds as discussed and accepted: the raise is a user-space act and no user space exists during early boot; the funnel's first customers are its own construction; and a failure at that point would be a panic at the least diagnosable moment of the boot. The prefix is an exemption named under the Discussion's definition, and by the completeness standard a logged allocation is not a missed one. Remaining before the build: second-ring scope, and the reconciliation run as a formal step.
Ruling (Andrew, 2026-11-10, night): for second-ring scope, the middle path. The first ring plus the side centers (mempool, percpu, gen_pool) are redirected; the page allocator stays substrate. CMA is moot, being absent from Fubuki's build. Andrew's framing, recorded as stated: everything else does not belong to a memory allocator outside a PC; the page allocator's further duties (reclaim, watermarks, feeding the page cache and user processes) are virtual-memory operating system duties, not an allocator's charter, and in the rig the substrate stands in for device memory in bulk while Iscandar does all parcelling. With this, every gating ruling is settled. Remaining before the build: the reconciliation run as a formal step, which is work rather than a ruling.
Addendum 2026-11-10 (night): at Andrew's direction ("Add to caveats"), a substrate item was added to the Caveats section: in the rig the stock page allocator supplies the arena's pages, standing in for bulk device memory, and parcels nothing for any caller; its further duties are a PC operating system's virtual-memory duties, not an allocator's charter, and stand outside the replacement; what the paper replaces is every allocator the kernel names. Repository only, with the rest of the Caveats section, awaiting his push word.
Addendum 2026-11-10 (night): at Andrew's direction, a further Caveats item was added after the substrate item, stating what an RTOS is not required to provide in virtual memory terms (no per-process address spaces, demand paging, page cache, or reclaim, and no defect in the absence), and that Iscandar is not a virtual memory replacement: it translates no addresses and administers no address spaces; where a VM system exists it stays stock, and where none exists none is missed. Iscandar's charter is allocation only, deterministic and exact parcelling with refusal. Repository only, awaiting his push word.
Addendum 2026-11-10 (night): the reconciliation was run as a formal step at Andrew's direction ("Do this in background"), in the background, and its artifact stands at data/reconciliation-2026-11-10.md, added to the data manifest. It reads the sift, the control trace, the compiled subset, and the dye against each other family by family under tonight's rulings. Headline outcome: of the twelve family rows, nine end redirected at a named center (the kmalloc spellings, named caches crossing as requests, the free doors, krealloc by composition, oversize kmalloc at its own center with no exemption, vmalloc with its reservation half stock by design, mempool through its backings, percpu, and gen_pool with its served set recorded as open); the page-direct family ends substrate, the declared exception, with DMA-coherent allocations served through the substrate path and the early-boot prefix stock and logged; CMA is moot by measured absence from Fubuki's build; memblock and vmap/ioremap stand out of scope as before. Gaps recorded in the artifact, not papered over: the dye-versus-sift expansion count (8 against 6, accounted exactly), the source-versus-runtime inversion on the slab family, the unmeasured families (vmalloc, page-direct, oversize split, percpu, gen_pool, the cross-center doors), the absent figures (no sift counts for krealloc or gen_pool spellings, no cache split inside the 706,419, no stated KMALLOC_MAX_CACHE_SIZE), the 6.12.116 against 6.12.113 version skew, the alloc_slab_page watch point, and gen_pool's unnamed served set. Manuscript section 6 remains a prediction until Andrew stett's updated text; no manuscript text was touched by the run. With this, every item that gated the build is complete: the build (patch, bridge, translation layer) is next when Andrew directs it.

Addendum 2026-11-11: at Andrew's direction ("synch the manuscript to overleaf") the repository manuscript was pushed to Overleaf in full, after a fresh source download was diffed against the repo copy and the difference verified to be exactly one block: the Live Interposition body replacing its stub comment, and the Caveats section after it. Upload replaced main.tex by overwrite; the project holds the single file. Verified in Overleaf: compile clean (0 errors, 0 warnings), paper now 10 pages, outline shows Caveats between Live Interposition and Measurements. Repository and Overleaf identical again. Noted in passing: the Overleaf project name in its top bar reads "Full Speed Ahead!" while the document title inside, the title of record, is "Full Steam Ahead!"; the project name was not changed.

Design note 2026-11-11 (Andrew, in session): deallocation across the bridge. A real pointer destroys the base-plus-length covenant a CPUPointer carries: the address recovers the start arithmetically (base subtraction, sector shift), but the count is nowhere in the value, and occupancy marks do not delimit abutting runs. Ruling in formation, stated by Andrew: the translation layer keeps an extent ledger indexed by the start sector itself, one slot per sector, storing the length only (two bytes; the count field is fifteen bits, and zero is Nothing, no grant starts here). Soundness rests on the construction invariant that all allocated pointers are disjoint and every grant starts from its own sector, so a slot has at most one live writer by construction. At the test arena (the Fubuki configuration of record: 512 MiB arena, 4 KiB grain, 131,072 units across 4 cubes of 32,768) the ledger is 131,072 by 2 bytes, 256 KiB beside the arena's 512 MiB, raised with the arena and fixed for its lifetime. Correction 2026-11-11: an earlier form of this note said 512 KiB for the arena; the checked figure is 512 MiB at the 4 KiB page grain, and the ledger's weight depends on the grain in this way. At free time the CPUPointer word is reassembled from index plus slot and the engine's ordinary inverse bijection runs; a zero slot at a free is a foreign, interior, or double free, an invariant violation. Not yet in the manuscript: section 11 text awaits Andrew's word.

Placement ruling 2026-11-11 (Andrew, in session): the extent ledger exists only as part of the price of supporting a full OS. In the original RTOS applications, holding the CPUPointer is the application's responsibility: the word circulates whole in the caller's hands, the covenant is intact, and no ledger exists or is needed. The ledger is therefore an artifact of the interposition arrangement, belonging to the translation layer beside the substrate and the early-boot prefix as a named price of this paper's setting, not a part of Iscandar or its claims.

Design note 2026-11-11 (Andrew, in session): where the extent ledger lives. Two choices: preallocate its pages separately, outside the arena, as arrangement overhead; or allocate its pages as overhead inside the Iscandar-controlled memory itself. Andrew records his precedent: on the 80188 design, relocating the RAM page, he did the latter, the table carried as overhead within the controlled memory. Consequences of the latter, stated for the record: at the test arena the ledger is 64 pages of the 131,072 units, a permanent pre-grant at the head of the store made at raise time, so the served capacity is the gross figure less 64 units and the paper must state net against gross; the ledger's own units are never freed and their slots never consulted in service; the real-system picture stays one device, a single static chip at a fixed address holding both arena and ledger, with no second store to house the table. Confirmed by Andrew the same day: there are always 32,768 pages in a cube, and against that figure the 64-page carve is reasonable. Checked: 64 of 32,768 is 0.195 percent of cube 0; 64 of the arena's 131,072 is 0.049 percent.

Procedure ruling 2026-11-11 (Andrew, in session): the ledger is set up the way the arena tables were set up on the 80188. There, the tables were relocated into the beginning of the SRAM and their sectors and counts were then marked manually. The same is to be done for the ledger, as additional OS support: at arena raise, before any serving, the table is placed at the beginning of the controlled memory (the first 64 pages of cube 0 at the test configuration), and the serving arrangement marks its sectors and sets the counts by hand, so the engine's books open already balanced with the fixture in place. The ledger is never the product of a request and never freed; it stands in the marks from the first moment, and run accounting reads it as permanently live from the start.

Relocation ruling 2026-11-11 (Andrew, in session): two areas are to be relocated into Iscandar itself, pre-allocated and marked manually at arena raise, in the 80188 manner. (1) The extent ledger: 64 pages at the head of cube 0, as recorded above. (2) The maps and counters: each cube's own serving state, its 4,096-byte sector bitmap and its counters (4,096 bytes of row counts, 128 of plane counts, the overall counter and its mirror, 8,328 bytes in all), placed at that cube's head: 3 pages per cube at the 4 KiB grain, 12 pages across the four-cube beam, each cube carrying its own books in the space it describes, its fixture sectors and counts marked by hand so its books open balanced. Total fixture overhead: 76 pages of 131,072, 0.058 percent; served capacity net 130,996 pages. The transcription already permits the placement: the cube store is a view seated on a caller's array, so relocation is a seating decision at raise time, not a change to the engine. Placement as recorded is per cube for the maps and counters; the alternative, one central region of 73 pages beside the ledger in cube 0, stands noted should Andrew redirect it.

Refinement 2026-11-11 (Andrew, in session), superseding the placement detail above: one relocated memory area per cube. Each cube's single area at its head holds everything relocated for that cube: its slice of the extent ledger (32,768 entries at 2 bytes, 16 pages) and its maps and counters (3 pages), 19 pages per cube, 76 pages across the beam, the same total as before, now uniform per cube. The ledger is thereby kept per cube as well: a cube's extent entries live with that cube. Andrew's stated purpose: this will help diagnose a severe failure, one cube's whole fixture standing in one place for inspection. The central-region alternative recorded above is closed.

Correspondence note 2026-11-11 (Andrew, in session): each cube corresponds, by math, to a base address in the simulated SRAM. A cube is 32,768 pages at 4 KiB, 128 MiB; cube c's base is the SRAM base plus c times 128 MiB, the four cubes tiling the 512 MiB arena contiguously, and a grant's address is base plus cube offset plus start times the grain: the address form of B_4(c,x) = c times 32,768 plus B_3(x). No table mediates the correspondence; it is arithmetic, as the start recovery is. Each cube's relocated area stands at its cube base, so its address is known by the same formula before the arena is raised.

Addendum 2026-11-11 (afternoon): at Andrew's direction ("make this all so, and write it up in Caveats"), the design session's rulings were written into the manuscript. Rulings as made in session, in order: the extent ledger exists because a real pointer destroys the CPUPointer's base-plus-length covenant (start recovered by arithmetic, length nowhere in the value); the ledger is one bin per page, length stuffed into the starting page's bin, the CPUPointer completed at deallocation by the reverse math plus the bin; the ledger is a price of supporting a full OS only, the RTOS applications holding the word themselves; each cube has one relocated area at its head (16 pages of ledger slice, 3 pages of maps and counters, 19 of 32,768; 76 of 131,072 arena-wide; net served 130,996 pages), pre-allocated and marked manually in the 80188 manner, for single-area diagnosis of a severe failure; each cube corresponds by math to a base address in the simulated SRAM; and the translation layer's own working memory and the bridge channel stand reserved in Fubuki's compile-time memory map (Andrew's yes, after the recommendation was put to him). In the manuscript, the Caveats section is retitled, at his dictation, to "Accomodations for Operating System as an Application" (dictated with one m; spelling corrected to Accommodations at his word the same day, before the Overleaf sync), and three items were inserted before its closing items: the ledger and the covenant, the relocated areas and their figures, and the compile-time map fixtures. No other section was touched. Pushed to Overleaf the same day at his word ("fix spelling, then push"), after a fresh download diffed the change as the retitle, three comment lines, and the three items only: verified there, compile clean (0 errors, 0 warnings), paper now 11 pages, outline shows Accommodations for Operating System as an Application between Live Interposition and Measurements. Repository and Overleaf identical again. The session's earlier design notes recording "not yet in the manuscript" for the ledger are superseded by this addendum; section 11's own text is unchanged.

Addendum 2026-11-11 (later): the standing-of-accommodations item, drafted in chat at Andrew's request, shown to him, and stetted by him, was inserted after the ledger item and pushed to Overleaf at his word the same day; the pre-push diff was the single added item. Verified there: compile clean (0 errors, 0 warnings), paper remains 11 pages, copies identical.

Addendum 2026-11-11 (later): sections 2 and 6 stetted by Andrew. Section 2 (Prior Results) stands as drafted 2026-11-08, its status comment now recording his stet. Section 6 (The Reconciliation) was updated from its prediction form to report the run of 2026-11-10, the updated text drafted from the artifact, shown to him in chat, and stetted: the dispositions (nine redirected, one substrate, one moot, residuals out of scope, early-boot prefix stock and logged), the inversion, the internal check (812,106 less 754,350 less 56,020 equals 1,736, exact), and the gaps as recorded in the artifact. With this, no manuscript section awaiting his stet remains among those drafted.

Addendum 2026-11-11 (later still): the section 2 and 6 stets were pushed to Overleaf at Andrew's direction the same day; the pre-push diff was the Prior Results status line and the Reconciliation block only. Verified there: compile clean (0 errors, 0 warnings), paper now 12 pages, all 17 headings unchanged, copies identical.

Addendum 2026-11-11 (evening): the exerciser pattern is FINISHED at Andrew's direction ("lets finish the exerciser pattern, and make it a boot-up app"), resolving the candidate draft's two open points. Scale factor 16 on every Phase A count (peak 9,216 pages, 36 MiB); anchors generalized: the 64 and 128 page classes are held whole through Phases B and C. Finished pattern and prediction sheet: paper-3-exerciser-pattern.md (standard run 5,072 allocations and 5,072 frees; extended 11,120 and 11,120). The draft is marked superseded. The program is written and tested: exerciser/exerciser.c (single file, libc only; mmap blocks, per-page serial stamps, stamp verification before every release, a wrong stamp stops the run with a failure status), built and run on this machine 2026-11-11: ledger check, standard run, and extended run all matched their prediction sheets exactly, exit 0. Boot-up packaging: exerciser/iscandar-exerciser.service (systemd oneshot, console menu via /dev/console with the 10-second default, ledger at /var/log/iscandar-exerciser/ledger.txt), Makefile with install, README covering ELBE image integration. Two stale spots in manuscript section 10 await a stet pass: its status comment still calls the pattern unapproved, and its boundary item says the page-allocator ruling "remains open" though Andrew settled it (substrate) on 2026-11-10.

Addendum 2026-11-11 (evening, later): section 10 resolved as stet at Andrew's word and pushed to Overleaf the same day: its status comment now records the finished pattern and the boot-up application, and its boundary item reads the exerciser's page-level requests as served through the page allocator standing underneath as substrate by ruling, in place of the open question. Verified there: compile clean (0 errors, 0 warnings), 12 pages, all 17 headings unchanged, copies identical.

Addendum 2026-11-11 (evening, last): the alloc_slab_page watch point is CLOSED by Andrew's ruling: bypass outright. His stated ground: the pages serving translated requests are Iscandar allocations, so no second serving remains for SLUB to bookkeep over. The ruling accords with his 2026-11-10 ruling that all cache bookkeeping lives in the translation layer. The reconciliation artifact's open item 6 is annotated closed, with the evidence run's check recorded there: slab page allocations attributable to redirected caches are zero after the early-boot prefix, and any that appear are a leak in the patch. One manuscript consequence stands open for his stet: section 6's gaps item still names the watch point as one "no input settles and which the build must answer"; it is now answered by ruling, and the item's wording waits on his word. (Precision kept in the record: the translation layer's own working pages remain the exception by his compile-time map ruling, reserved at build, not Iscandar-serviced; the bypass rests on the served pages, not on the layer's own.)

Addendum 2026-11-11 (evening, last): the cache registry's seat, ruled by Andrew on Muse's suggestion. Asked whether the translation layer's own working pages should be produced by Iscandar, Muse answered no (the bootstrap: the layer must hold memory before the first grant can be recorded; the failure floor: the server must keep standing when a grant fails; and the audit's line between the layer and its load), and suggested the registry as the boundary case: it is data about the served world, not machinery. Andrew ruled it as suggested. The registry is seated inside the governed memory at the raise, among the fixtures, under a hard cap fixed by the compiled subset's count of the caches Fubuki's configuration can create; a creation past the cap is refused, a finding. Documented in the manuscript's Accommodations section at his direction, as a new item placed after the extent ledger item and before the standing item, so the standing item's scope covers it. The layer's own working memory stands as ruled this morning: reserved in the compile-time map, outside the arena.

Addendum 2026-11-11 (evening, last): the cache registry item was pushed to Overleaf the same day; the pre-push diff was the single added item. Verified there: compile clean (0 errors, 0 warnings), 12 pages, all 17 headings unchanged, copies identical. Andrew then probed the ruling (fixed cap versus expansion through Iscandar allocation); Muse's answer of record: the fixed fixture is proper, as part of the provable ceiling, with the bound taken from the compiled subset at build time; a future system needing more moves the bound, it does not dissolve it. No change resulted.

Addendum 2026-11-11 (evening, last): section 6's gaps item corrected under Andrew's stet: the watch point line now reads that alloc_slab_page was settled by no input and answered by ruling, bypass outright, SLUB keeping no bookkeeping over Iscandar grants. With this correction, no manuscript wording known to be stale remains; every section stands stetted and current.
