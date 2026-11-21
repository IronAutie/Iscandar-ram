# Bench packet: closing the coverage audit

Prepared 2026-11-19 for the next telesart session. The
audit draft (COVERAGE-AUDIT-DRAFT.md) is the checklist;
this packet is the commands, in the bench's order, one
at a time as always. Every command is a read. Nothing
here changes the tree.

Working directory for all of it:

```
cd ~/linux-6.12.116
```

Expected: the kernel tree, bridge-patched as it stands.

## 1. The closure enumeration (slab freelist)

```
grep -n "c->freelist\|get_freepointer\|->cpu_slab" mm/slub.c | head -80
```

Expected: every function that touches the per-CPU
freelist directly. Each name on the list must land in
the audit as a center, door'd, funneled (alloc_bulk),
or named stock. A name that lands nowhere is the
finding the enumeration exists to catch.

## 2. alloc_lru re-verification

```
grep -n "kmem_cache_alloc_lru" mm/slub.c mm/slab.h include/linux/slab.h
```

Then read the function body the grep names. Expected:
a thin wrapper over slab_alloc_node, the lru travelling
as the center's own parameter, which closes audit item
1 as read against the tree.

## 3. alloc_bulk re-verification (the funnel target)

```
grep -n "alloc_bulk\|alloc_from_pcs" mm/slub.c | head -20
```

Then read the bulk body. Expected: its own drain over
the per-CPU freelist, not a funnel into slab_alloc_node.
This read fixes the funnel hunk's ground: which function
holds the drain in 6.12.116, and its exact shape.

## 4. free_percpu re-read

```
grep -n "free_percpu\|pcpu_alloc_noprof" mm/percpu.c | head
```

Then read both bodies. Expected: the alloc door at the
center and the free door above the chunk search, as
patch 0004 placed them, which closes audit item 5.

## 5. The gen_pool census

```
grep -rn "gen_pool_create\|gen_pool_add" --include=*.c . | grep -v "lib/genalloc" | head -40
```

Expected: the pool creators in the tree. Against
Fubuki's compiled subset (the .config's built-ins),
each pool gets its backing classification recorded in
the audit: ordinary RAM (served under the backing rule)
or a device region (stock). That classification is the
expectation the T6 runtime test later checks.

## 6. kvmalloc, for the record

```
grep -n "__kvmalloc_node_noprof\|void kvfree" mm/util.c | head
```

Expected: the fallback and its free, as the audit
describes. The address-range half of the verification
runs later, on the kernel build's vmlinux: the arena
section's addresses read against the vmalloc range,
from the linked image, before the first boot.

## After the reads

The audit is re-issued without the word draft, with
items 1, 2, 4, and 5 closed by these reads, the
enumeration's list folded into its table, and the
gen_pool census recorded. Verification step 3 is then
done, and the round proceeds to the first boot.
