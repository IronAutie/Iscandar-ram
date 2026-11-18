# Task 6 binding draft (for Andrew's stet, 2026-11-17)

The pieces the staged build left unwritten, drafted whole so
they can be ruled on as one package: the bridge core's
binding body, the layer's region mode, the span choices,
and the changes that follow in the boot units and scripts.
Nothing here is applied. Decisions are numbered; each wants
a stet or a correction.

RULING 2026-11-17: Decision 1 is taken as (B), in
Andrew's words: "I accept B. Efficiency later." The
resident-efficiency question the 512 MiB reservation
raises is deferred by his word; function first.
Decisions 2 to 5 stand as drafted. The open point is
RULED 2026-11-17, in Andrew's words: "let's measure the
violations first for what we need to fix next round."
The evidence run proceeds without the token fix; the
finish books' invariant count is the measurement the
next round is designed from.

## Decision 1: the arena span, and the guest's size

The staged core turns a granted offset into a kernel page
by linear arithmetic on iscandar_arena_phys. For that page
to be real, three things must hold at once: the offset's
page has a struct page, the buddy allocator never hands it
to anyone else, and the layer's own arena mapping is that
same memory (the extent ledger stands in the arena, per the
compile-time map; it is not a layer-side copy).

That forces a physically reserved arena span as large as
the largest offset the Beam can grant: the full arena. Two
configurations close:

(A) Guest at 512 MiB as drawn, arena cut to 2 cubes: a
256 MiB span at phys 0x10000000, header ISCANDAR_ARENA_CUBES
to 2, net served 65,498. Every settled 4-cube figure falls:
the self-test raise line, the constants in the image, the
manuscript's 130,988.

(B) Arena kept at 4 cubes, the guest enlarged: -m 1024, a
512 MiB span at phys 0x20000000 (the whole second half of
the guest), net served 130,988 and every other settled
figure stands. What changes is a boot parameter and one
paragraph of the compile-time map: the map's resident
arithmetic was drawn against the control boot's 512 MiB,
but the evidence run's claim is the recorded load borne
without a panic, not the control machine's RAM size. The
stock system and the layer live in the low 512 MiB, whose
measured whole-system occupancy is 75.9 MiB.

Recommendation: (B). It is the only option that touches no
result, no figure, and no line of the manuscript.

Under either: the channel is 512 KiB at phys 0x1FFE0000,
the top of the low half, in both configurations the same.

## Decision 2: the core reserves its own spans

iscandar_channel_setup and iscandar_arena_setup gain one
line each: memblock_reserve of their span. The __setup
handlers run while memblock is still the allocator of
record, so the reservation precedes the buddy allocator
entirely: the spans keep their struct pages (the offset
arithmetic needs them) and the buddy never sees a page of
either. No memmap= on the boot line, nothing for the boot
to forget.

## Decision 3: user space reaches the spans through the core

/dev/mem cannot serve: under STRICT_DEVMEM it refuses
System RAM, and the spans must stay System RAM to keep
their struct pages. So the core registers one misc device,
/dev/iscandar, whose mmap maps the channel at file offset
0 and the arena at file offset ISCANDAR_CHANNEL_BYTES,
both by remap_pfn_range. The layer's region mode opens
this device instead of a file. No new config dependency,
no e820 surgery, and the only hands that can map the spans
are hands the core itself serves.

## The core body (appended to mm/iscandar_bridge.c)

Replaces the staged closing comment; the comment's contract
is kept verbatim: the read's first line is exactly "state
ready" or "state prefix", the write accepts "raise", and
with Decision 6 below, "stop".

```c
/* Binding body (Task 6). The spans arrive by boot
 * parameter and are reserved by the __setup handlers
 * above while memblock still rules; what follows maps
 * the channel, publishes the proc file, and runs the
 * arena self-check, all at postcore: after mm_init, so
 * the earliest prefix notes (the kmalloc caches are born
 * in mm_init, before any initcall) find iscandar_ctrl
 * NULL and are dropped by the staged check. The registry
 * does not depend on those notes: the raise enumerates
 * the live caches. The prefix counters are therefore
 * exact from the mapping on, and undercount the first
 * moments before it; that boundary is stated here rather
 * than hidden. */

static int iscandar_selfcheck_rc = -ENODEV;
static u64 iscandar_timeouts;

/* Bridged-call latency, caller-visible: from the call's
 * entry (before the serializing mutex) to its response.
 * Microseconds. Samples past the cap are counted apart
 * and kept out of the sumsq, so the accumulators cannot
 * overflow on a 5 s timeout's square. All updates happen
 * under iscandar_call_lock, which serializes the calls
 * themselves, so the statistics need no lock of their
 * own. */
#define ISCANDAR_LAT_CAP_US	100000	/* 100 ms */
static u64 iscandar_lat_n;
static u64 iscandar_lat_sum_us;
static u64 iscandar_lat_sumsq_us;
static u64 iscandar_lat_min_us = U64_MAX;
static u64 iscandar_lat_max_us;
static u64 iscandar_lat_slow;

static void iscandar_lat_note(u64 t0_ns)
{
	u64 dt_us = div_u64(ktime_get_ns() - t0_ns, 1000);

	iscandar_lat_n++;
	if (dt_us > ISCANDAR_LAT_CAP_US) {
		iscandar_lat_slow++;
		return;
	}
	iscandar_lat_sum_us += dt_us;
	iscandar_lat_sumsq_us += dt_us * dt_us;
	if (dt_us < iscandar_lat_min_us)
		iscandar_lat_min_us = dt_us;
	if (dt_us > iscandar_lat_max_us)
		iscandar_lat_max_us = dt_us;
}
```

In iscandar_bridge_call, two lines change: `u64 t0 =
ktime_get_ns();` at entry, iscandar_lat_note(t0) on the
success path, and iscandar_timeouts++ beside the existing
timeout pr_err.

```c
/* The arena self-check: the chain's last link, proven by
 * the kernel alone before any raise. Offset one page is a
 * fixture page (the ledger's second page): never granted,
 * so it is safe scratch. Write a pattern through the
 * offset's kernel pointer, read it back through the
 * offset's struct page. Agreement means offset, page, and
 * memory are one thing. */
static int __init iscandar_arena_selfcheck(void)
{
	u64 offset = ISCANDAR_PAGE_BYTES;
	volatile u32 *via_ptr;
	struct page *pg;
	u32 *via_page;
	u32 pattern = ISCANDAR_CHANNEL_MAGIC;

	if (!iscandar_arena_base)
		return -ENODEV;
	via_ptr = iscandar_offset_to_ptr(offset);
	pg = iscandar_offset_to_page(offset);
	if (!via_ptr || !pg)
		return -ENODEV;
	via_page = page_address(pg);
	if (!via_page)
		return -ENODEV;
	*via_ptr = pattern;
	smp_wmb();
	if (*via_page != pattern)
		return -EIO;
	*via_ptr = 0;
	return 0;
}

/* The raise: the layer's control block must carry the
 * magic and the version; then the bridge opens, and the
 * caches born during the prefix are registered by
 * enumerating the live cache list (mm's own slab_caches,
 * under slab_mutex; this file lives in mm/ and includes
 * "slab.h"). Registration is the first bridged traffic,
 * so ready is set before the enumeration. Allocations
 * from a cache not yet enumerated, in the raise window,
 * take the centers' unregistered path, which is the
 * prefix posture: stock-served. New cache creation waits
 * on slab_mutex for the enumeration's duration, bounded
 * by the registry cap at one bridged call each. */
static int iscandar_raise(void)
{
	struct kmem_cache *s;
	int registered = 0, failed = 0;

	if (!iscandar_ctrl)
		return -ENODEV;
	if (iscandar_ready)
		return 0;
	if (iscandar_ctrl->magic != ISCANDAR_CHANNEL_MAGIC
	    || iscandar_ctrl->version != ISCANDAR_CHANNEL_VERSION)
		return -EPROTO;
	WRITE_ONCE(iscandar_ready, true);
	mutex_lock(&slab_mutex);
	list_for_each_entry(s, &slab_caches, list) {
		if (iscandar_cache_lookup(s))
			continue;
		if (iscandar_cache_register(s, s->object_size,
					    s->align, s->flags))
			failed++;
		else
			registered++;
	}
	mutex_unlock(&slab_mutex);
	pr_info("iscandar: raise: %d caches registered, %d failed\n",
		registered, failed);
	return failed ? -ENOSPC : 0;
}

static ssize_t iscandar_bridge_write(struct file *file,
				     const char __user *buf,
				     size_t count, loff_t *ppos)
{
	char word[16];

	if (count >= sizeof(word))
		return -EINVAL;
	if (copy_from_user(word, buf, count))
		return -EFAULT;
	word[count] = '\0';
	if (strncmp(word, "raise", 5) == 0)
		return iscandar_raise() ?: (ssize_t)count;
	if (strncmp(word, "stop", 4) == 0) {
		if (!iscandar_ctrl)
			return -ENODEV;
		WRITE_ONCE(iscandar_ctrl->stop, 1);
		return count;
	}
	return -EINVAL;
}

static int iscandar_bridge_show(struct seq_file *m, void *v)
{
	u64 n = iscandar_lat_n - iscandar_lat_slow;
	u64 mean = 0, var = 0;
	int i;

	seq_printf(m, "state %s\n",
		   iscandar_ready ? "ready" : "prefix");
	if (!iscandar_ctrl) {
		seq_puts(m, "binding absent: no channel span\n");
		return 0;
	}
	seq_printf(m, "arena self-check: %s\n",
		   iscandar_selfcheck_rc == 0 ? "PASS" : "FAIL");
	seq_printf(m, "prefix log written %u, overflow %u\n",
		   iscandar_ctrl->prefix_log_written,
		   iscandar_ctrl->prefix_overflow);
	for (i = 0; i < ISCANDAR_CENTER_COUNT; i++)
		if (iscandar_ctrl->prefix_calls[i])
			seq_printf(m, "prefix calls center %d: %u\n",
				   i, iscandar_ctrl->prefix_calls[i]);
	seq_printf(m, "slab page watch: %lld\n",
		   (long long)atomic64_read(&iscandar_slab_page_calls));
	seq_printf(m, "bridge timeouts: %llu\n",
		   (unsigned long long)iscandar_timeouts);
	if (n) {
		mean = div_u64(iscandar_lat_sum_us, n);
		var = div_u64(iscandar_lat_sumsq_us, n);
		var = var > mean * mean ? var - mean * mean : 0;
	}
	seq_printf(m, "bridge latency us: n %llu slow %llu "
		   "mean %llu var %llu min %llu max %llu\n",
		   (unsigned long long)iscandar_lat_n,
		   (unsigned long long)iscandar_lat_slow,
		   (unsigned long long)mean,
		   (unsigned long long)var,
		   (unsigned long long)(iscandar_lat_n
			&& iscandar_lat_min_us != U64_MAX
			? iscandar_lat_min_us : 0),
		   (unsigned long long)iscandar_lat_max_us);
	return 0;
}

static int iscandar_bridge_open(struct inode *inode,
				struct file *file)
{
	return single_open(file, iscandar_bridge_show, NULL);
}

static const struct proc_ops iscandar_bridge_proc_ops = {
	.proc_open	= iscandar_bridge_open,
	.proc_read	= seq_read,
	.proc_write	= iscandar_bridge_write,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

/* /dev/iscandar: file offset 0 maps the channel, file
 * offset ISCANDAR_CHANNEL_BYTES maps the arena. */
static int iscandar_dev_mmap(struct file *file,
			     struct vm_area_struct *vma)
{
	u64 off = (u64)vma->vm_pgoff << PAGE_SHIFT;
	u64 phys, size;

	if (off == 0) {
		phys = iscandar_channel_phys;
		size = ISCANDAR_CHANNEL_BYTES;
	} else if (off == ISCANDAR_CHANNEL_BYTES) {
		phys = iscandar_arena_phys;
		size = ISCANDAR_ARENA_BYTES;
	} else {
		return -EINVAL;
	}
	if (!phys || (vma->vm_end - vma->vm_start) > size)
		return -EINVAL;
	return remap_pfn_range(vma, vma->vm_start,
			       PHYS_PFN(phys), 
			       vma->vm_end - vma->vm_start,
			       vma->vm_page_prot);
}

static const struct file_operations iscandar_dev_fops = {
	.owner	= THIS_MODULE,
	.mmap	= iscandar_dev_mmap,
};

static struct miscdevice iscandar_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "iscandar",
	.fops	= &iscandar_dev_fops,
};

static int __init iscandar_bridge_init(void)
{
	if (iscandar_channel_phys) {
		iscandar_ctrl = memremap(iscandar_channel_phys,
					 ISCANDAR_CHANNEL_BYTES,
					 MEMREMAP_WB);
		if (!iscandar_ctrl) {
			pr_err("iscandar: channel memremap failed\n");
			return -ENOMEM;
		}
		iscandar_req_ring = (struct IscandarBridgeRecord *)
			((char *)iscandar_ctrl + ISCANDAR_PAGE_BYTES);
		iscandar_resp_ring = (struct IscandarBridgeRecord *)
			((char *)iscandar_ctrl + ISCANDAR_PAGE_BYTES
			 + (size_t)ISCANDAR_CHANNEL_REQ_PAGES
			   * ISCANDAR_PAGE_BYTES);
		misc_register(&iscandar_dev);
	}
	if (iscandar_arena_phys || iscandar_arena_base)
		iscandar_selfcheck_rc = iscandar_arena_selfcheck();
	proc_create("iscandar_bridge", 0644, NULL,
		    &iscandar_bridge_proc_ops);
	return 0;
}
postcore_initcall(iscandar_bridge_init);
```

Two additions outside this block: `#include "slab.h"`,
`<linux/miscdevice.h>`, `<linux/ktime.h>`, `<linux/seq_file.h>`
at the core's head; memblock_reserve lines in the two
__setup handlers (Decision 2); and iscandar_arena_base set
in iscandar_arena_setup as phys_to_virt of the span.

## Decision 4: the layer's region mode

`iscandar-layer --serve-region`: read iscandar_channel=
and iscandar_arena= from /proc/cmdline (one home for the
addresses: the boot line), open /dev/iscandar, mmap the
channel at offset 0 and the arena at offset
ISCANDAR_CHANNEL_BYTES in place of the anonymous arena.
Attach discipline, differing from the file mode in exactly
the ways the kernel's prefix requires:

- No memset of the channel, ever. The layer writes only
  the control fields it owns: magic, version, arena_base
  (its own mapping's base, as in file mode), arena_cubes,
  channel_pages, the four ring indices at zero, the served
  and refusal counters at zero, stop at zero.
- prefix_calls, prefix_log_written, prefix_overflow and
  the whole prefix log region are the kernel's writing;
  the layer does not touch them.
- The rings are zeroed (they are empty by construction
  before the raise: bridge_call refuses until ready).
- The state region (Beam structs) stays an anonymous
  mapping, as the compile-time map seats it.

File mode stays as it is, for the rig.

## Decision 5: the stop word and the snapshot move

In region mode the finish script can no longer dd a stop
word into a file. Instead: `echo stop > /proc/iscandar_bridge`
(the core sets the control block's stop field, Decision 3's
write handler), and the layer, on seeing stop and before
it lowers, writes the channel snapshot itself to
/root/iscandar-results/channel-snapshot.bin from its own
mapping: the same artifact, harvested by the one process
that holds the region. iscandar-finish keeps its shape
otherwise; its stop-word dd block is replaced by the proc
write, and its snapshot copy becomes a check that the
layer's snapshot landed.

Unit changes: iscandar-layer.service drops the channel.bin
ExecStartPre and serves --serve-region; the raise and
exerciser units are unchanged; the link-check gains the
two new inline bodies to verify.

## Open point, named and bounded: prefix-born vmalloc frees

At arena base 0x20000000 the vfree door's page test is
exact for bridge-born areas. An area born in the prefix
(stock pages) and freed after the raise is a different
matter in one respect: its pages are not in the arena
span, so the door correctly leaves it stock. That holds.
What does not hold is the mirror case in the slab centers:
a slab page granted by the buddy during the prefix for a
cache that is registered at the raise, then freed after
the raise through the bridged free path, would be offered
to the layer as a grant it never made. The layer's ledger
bin reads zero at that offset and the free is reported as
an invariant violation, not silently absorbed: the design
catches it. The draft's posture for the evidence run is
to let it be caught and counted, turning the hole into a
measurement in the finish books; the permanent fix
(recording stock-born tokens at the alloc centers during
the prefix, the prefix log's token field already waits
for them) is named here for the next round if the count
is nonzero. Ruling wanted: accept the posture, or hold
the run for the fix.

## Boot line consequences (for Task 8, not yet composed)

iscandar_channel=0x1FFE0000, iscandar_arena=0x20000000,
guest -m 1024 under Decision 1(B). No memmap= (Decision 2).
The channel ends exactly where the arena span begins, and
74,537 pages of grant headroom stand between the recorded
load's peak (56,535 pages granted) and the arena's top, so
no grant of the recorded load can reach the channel's
pages, which lie outside the arena span in any case.
