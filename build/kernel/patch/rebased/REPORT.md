# Rebase report: the kernel-round patch set

Date: 2026-11-20. The stetted staged set
(`../0001`..`0005`) rebased onto a fresh Linux 6.12.116
tree, desk-side, and compile-checked. The rebased
patches are in this directory. The staged set remains
the record of what was stetted; this report is the
record of what the rebase bound and changed, and why.

## Base and method

Base: pristine 6.12.116 (tarball SHA-256
cb3ed3df...511b, the tree of record), extracted fresh
at `~/workspace/kernel-trees/linux-6.12.116-kr` and
committed as the base commit. The five patches were
then applied as five commits, one per staged patch,
in order. Verbatim copies were copied, never retyped:
the shared constants header, the marked mass, the
k=3 and k=4 engine sources, the seven gear bodies.
The four new files of 0001 (the public header,
declspec.h, the directory Makefile, core.c) were
extracted mechanically from the staged patch text, so
the rebased text is the stetted text except where a
binding below says otherwise.

## Bindings and corrections the rebase made

1. The CPUPointer conversions in core.c are bound to
   the admitted API: the start field is the global
   page (`cpu_pointer_start`), and reconstruction goes
   through `cpu_pointer_create` in the Beam's own
   order, K4_ORDER_MOTOROLA, splitting the local
   sector into sector, row, and plane by the
   displacement formula. A failed reconstruction is a
   counted violation at the free door.
2. `iscandar_range_is_ram` is bound to the resource
   tree's own test, `region_intersects` against
   IORESOURCE_SYSTEM_RAM, over the chunk's whole
   range.
3. The canary's third path now goes through the page
   struct with `offset_in_page`, so the three paths
   are genuinely independent (the staged form read
   the same linear address twice on any placement
   where the mass is not page-aligned).
4. The `obj-y += iscandar/` line stands after
   page-alloc.o, not at the Makefile's head: the head
   placement is clobbered by the later `obj-y :=`
   assignment, the bridge round's 0005b lesson.
5. The linker sections stand after `.data` in
   arch/x86/kernel/vmlinux.lds.S, in the AT() form
   this tree uses for its own late sections.
6. Each door file gained an explicit
   `#include <linux/iscandar.h>`. The rebased set is
   self-contained; nothing is left to include order.
7. vm_area_alloc_pages (2.3): the real signature
   takes (nr_pages, pages[]), and its failure
   contract is a count, so the door returns 0 on
   refusal, not the -ENOMEM the staged text carried.
8. Cache setup (1.5): in this tree the size and the
   flags are parameters of __kmem_cache_create_args,
   not members of kmem_cache_args; registration
   reads the parameters. A registration refusal
   destroys the half-built cache and returns NULL,
   the function's own failure form, in place of the
   staged ERR_PTR.
9. The funnel's consequence, found by the build:
   with the error path rerouted through the door'd
   kmem_cache_free_bulk, the static
   __kmem_cache_free_bulk has no caller left in the
   full body. It now stands under CONFIG_SLUB_TINY
   only, where its caller remains. The TINY debt the
   staged set recorded is thereby settled in both
   directions: TINY keeps its twin, the full body
   does not carry a dead one.
10. The mass file is named iscandar_mass.S in the
    tree: kbuild assembles capital-S sources. The
    generator's output is unchanged; only the tree
    copy's name differs.
11. Build seams for the engine text, all in
    mm/iscandar's Makefile and two shim headers, the
    engine text itself unmodified (R9): stdint.h and
    string.h shims onto the kernel's own headers;
    Gears.h force-included directory-wide because the
    gear bodies are thin exports whose prototypes
    live in Gears.h, which they do not include, and
    the kernel build asks (-Wmissing-prototypes);
    per-file warning relief, visible but not errors,
    for the check drivers' table-reached labels and
    the two drivers' headerless entry points.

## Compile check

Config: x86_64 defconfig-derived (SLUB=y,
CONFIG_SLUB_TINY off, SMP=y). The unwinder, IBT, and
retpoline selections were turned off for one reason
only: this VM's system state does not persist package
installs, and objtool's libelf headers had to come
from a local prefix instead. The check's config is
not Fubuki's config; the bench build owns that. The
tools (flex, bison, bc, m4) and the libelf headers
were likewise a local prefix under ~/workspace/.tools,
against the system's libelf runtime.

Results: mm/iscandar/built-in.a builds whole, 28
objects (core, mass, 19 engine, 7 gears). The door
objects build: mm/slub.o, mm/slab_common.o,
mm/vmalloc.o, mm/percpu.o, lib/genalloc.o.

Section check on the mass object: .iscandar_state
is 561,424 bytes, the desk-check figure exactly;
.iscandar_arena is 536,870,912 bytes, NOBITS.

Sleep-free audit on the kernel objects (verification
step 2's remaining half): the engine objects'
undefined symbols are the engine's own
cross-references, memset, and the toolchain's
__stack_chk_fail, nothing else. The core's fast path
calls the spinlock pair only; the mutex appears in
the control-path functions of the same file
(registration, unregistration, the gen_pool gate),
as the design's section 3 rules.

## Link check

DONE, 2026-11-20 evening: a full vmlinux link under
the check config completed clean (one runtime restart
interrupted the first attempt at about a thousand
objects; the build resumed and finished). The linked
image carries the sections where the script put them:
.iscandar_state at 0xffffffff82acb280, 561,424 bytes;
.iscandar_arena at 0xffffffff82b55000, 536,870,912
bytes, NOBITS, page-aligned. No image-size assertion
tripped. Two observations the link bought:

The mass is NOT page-aligned in the image (its
address ends in 0x280). Binding 3 above is therefore
not theoretical: the staged canary's third path would
have read a different word than the witness word on
this placement, and the canary would have shut the
engine on a healthy mass.

The kvmalloc address-range read (coverage audit item
4, the half that runs on the linked image): the arena
occupies 0xffffffff82b55000 to 0xffffffffa2b55000.
The vmalloc range under this config's five-level
paging bases at 0xffa0000000000000 (under four-level,
0xffffc90000000000 with its end at
0xffffe8ffffffffff); under either layout the arena
sits wholly above the vmalloc end, in the kernel
image region. is_vmalloc_addr is therefore false for
every arena address, and kvfree's partition divides
the two populations exactly, as the audit's
composition argument required. The guest half of
item 4 (tests K1 to K3) still rides to verification
step 5.

## What the bench still owns

Apply the rebased series to a fresh tree carrying
the Fubuki config, build, make the ELBE image, and
run verification step 4, the first boot: canary,
ready, state read exact. Nothing in this report is a
boot result.
