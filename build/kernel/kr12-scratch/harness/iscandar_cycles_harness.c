/* SPDX-License-Identifier: GPL-2.0 */
/*
 * iscandar_cycles_harness.c -- the KR12 timing harness.
 *
 * Purpose (Andrew, 2026-11-27): compare the timing VARIANCE of
 * NANO and TINY allocations between Fubuki serving (the Iscandar
 * layer at the doors) and a stock-serving control, in CPU cycles.
 * Not a speed contest: the figures of record are the shape of
 * each distribution, above all the worst-to-median ratio. SLUB
 * and the page allocator are expected to post the lower medians;
 * that expectation is part of the design, not a defect in it.
 *
 * The door is vmalloc/vfree, chosen from the sources (see
 * HARNESS-NOTE.md for the full evidence): when the engine is
 * ready, vm_area_alloc_pages serves the request from
 * iscandar_alloc(nr_pages << PAGE_SHIFT, PAGE_SIZE, 0), so the
 * request lands in its class exactly (total == nr_pages, no
 * alignment shift); when the engine is shut, the same function
 * serves from the stock page allocator. One call site, two
 * servers, identical in both boots. vmalloc rather than vzalloc:
 * no zeroing term on either side.
 *
 * Sizes are the class edges and interiors at this build's
 * calibration (classes in pages, core.c's iscandar_class_of):
 * 4 KiB (1 page, nano), 32 KiB (8 pages, nano top), 64 KiB
 * (16 pages, tiny), 128 KiB (32 pages, tiny top). get_vm_area
 * size is the request page-aligned, the guard page living in
 * the area size only, so nr_pages at the door is exactly
 * size >> PAGE_SHIFT for these power-of-two requests.
 *
 * Method: per size, WARMUP untimed alloc/touch/free pairs,
 * then SAMPLES timed pairs, one block outstanding at a time,
 * so the occupancy the walk sees is stationary. Alloc and free
 * are timed separately, as the core's latency ledger notes
 * them separately. Samples are kept whole and sorted, so the
 * median is exact (the element at index n/2 after an ascending
 * sort), not an approximation from bins.
 *
 * Noise: samples are taken in process context with interrupts
 * as they come, the doors' own terms. No irq-off sampling: the
 * comparison is of door calls as the system actually makes
 * them, and guest interrupt noise lands on both boots alike.
 * The distribution is the defense; the worst is reported as
 * the worst observed under guest conditions, on both sides in
 * the same words.
 *
 * Labelling: the harness reads the engine's state from
 * /proc/iscandar (the state line, the same window the operator
 * reads) before it runs, and prints that state in its header.
 * A log therefore names its own boot: serving (state ready) or
 * control (state closed). If the state cannot be read or is
 * neither, the harness refuses to run. A failed allocation
 * mid-run aborts the run with a finding printed; no retry, no
 * partial table (the house doctrine: failure is debugger time,
 * not a condition to absorb silently).
 *
 * Trigger: insmod the module, then write anything to
 * /proc/iscandar_cycles_harness. Each write runs the full set
 * once. Results go to printk (the console log is the evidence
 * of record) and are also held for reading back from the same
 * proc file until the next run replaces them.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/sort.h>
#include <linux/timex.h>
#include <linux/mutex.h>
#include <linux/string.h>

#define ICH_WARMUP	1000
#define ICH_SAMPLES	10000

struct ich_size {
	size_t		size;
	u32		pages;
	const char	*class_name;
};

static const struct ich_size ich_sizes[] = {
	{ 4096,		1,	"nano" },
	{ 32768,	8,	"nano" },
	{ 65536,	16,	"tiny" },
	{ 131072,	32,	"tiny" },
};

static u64 ich_alloc_samples[ICH_SAMPLES];
static u64 ich_free_samples[ICH_SAMPLES];

/* The report, held for read-back; the console log carries the
 * same lines as they are produced. */
static char ich_report[16384];
static size_t ich_report_len;

static DEFINE_MUTEX(ich_run_lock);

/* Keeps the compiler from deciding the measured calls are
 * decorative: every granted pointer and every touched byte
 * feeds the sink. */
static volatile u64 ich_sink;

static void ich_emit(const char *fmt, ...)
{
	char line[512];
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vsnprintf(line, sizeof(line), fmt, ap);
	va_end(ap);
	if (n < 0)
		return;
	if (n >= (int)sizeof(line))
		n = sizeof(line) - 1;

	pr_info("%s", line);

	if (ich_report_len + n < sizeof(ich_report)) {
		memcpy(ich_report + ich_report_len, line, n);
		ich_report_len += n;
		ich_report[ich_report_len] = '\0';
	}
}

/* The engine's state, from its own window. Returns 0 and sets
 * *boot to "serving" or "control"; refuses (negative) on any
 * state the harness does not recognize, so a run can never be
 * labelled by assumption. The state line is copied out whole
 * for the header. */
static int ich_read_state(const char **boot, char *state_line,
			  size_t line_len)
{
	struct file *f;
	char buf[128];
	loff_t pos = 0;
	ssize_t got;
	char *nl;

	f = filp_open("/proc/iscandar", O_RDONLY, 0);
	if (IS_ERR(f))
		return PTR_ERR(f);
	got = kernel_read(f, buf, sizeof(buf) - 1, &pos);
	filp_close(f, NULL);
	if (got <= 0)
		return got ? (int)got : -EIO;
	buf[got] = '\0';

	nl = strchr(buf, '\n');
	if (nl)
		*nl = '\0';
	strscpy(state_line, buf, line_len);

	if (strnstr(buf, "state ready", sizeof(buf))) {
		*boot = "serving";
		return 0;
	}
	if (strnstr(buf, "state closed", sizeof(buf))) {
		*boot = "control";
		return 0;
	}
	return -EINVAL;
}

static int ich_cmp_u64(const void *a, const void *b)
{
	u64 x = *(const u64 *)a;
	u64 y = *(const u64 *)b;

	if (x < y)
		return -1;
	if (x > y)
		return 1;
	return 0;
}

/* Reduce one sample set in place and print its line. The
 * median is the element at index n/2 after the ascending
 * sort; the ratio is the worst in thousandths of the median,
 * an integer so the log format carries no floats. */
static void ich_report_samples(const char *boot, const struct ich_size *sz,
			       const char *op, u64 *samples)
{
	u64 sum = 0, min, median, mean, worst, ratio_milli;
	int i;

	sort(samples, ICH_SAMPLES, sizeof(u64), ich_cmp_u64, NULL);
	for (i = 0; i < ICH_SAMPLES; i++)
		sum += samples[i];

	min = samples[0];
	median = samples[ICH_SAMPLES / 2];
	worst = samples[ICH_SAMPLES - 1];
	mean = sum / ICH_SAMPLES;
	ratio_milli = median ? worst * 1000 / median : 0;

	ich_emit("ICH1 boot=%s size=%zu pages=%u class=%s op=%s n=%d "
		 "min=%llu median=%llu mean=%llu worst=%llu ratio_milli=%llu\n",
		 boot, sz->size, sz->pages, sz->class_name, op,
		 ICH_SAMPLES,
		 (unsigned long long)min, (unsigned long long)median,
		 (unsigned long long)mean, (unsigned long long)worst,
		 (unsigned long long)ratio_milli);
}

/* One size: warmup, then the timed pairs. Returns 0, or a
 * negative errno with the abort already printed. */
static int ich_run_size(const char *boot, const struct ich_size *sz)
{
	cycles_t t0, t1, t2;
	void *p;
	size_t off;
	int i;

	for (i = 0; i < ICH_WARMUP; i++) {
		p = vmalloc(sz->size);
		if (!p) {
			ich_emit("ICH1 abort boot=%s size=%zu at=warmup "
				 "iter=%d reason=vmalloc-null\n",
				 boot, sz->size, i);
			return -ENOMEM;
		}
		for (off = 0; off < sz->size; off += PAGE_SIZE)
			((volatile char *)p)[off] = (char)(off >> PAGE_SHIFT);
		ich_sink += (uintptr_t)p + ((volatile char *)p)[sz->size - 1];
		vfree(p);
	}

	for (i = 0; i < ICH_SAMPLES; i++) {
		t0 = get_cycles();
		p = vmalloc(sz->size);
		t1 = get_cycles();
		if (!p) {
			ich_emit("ICH1 abort boot=%s size=%zu at=sample "
				 "iter=%d reason=vmalloc-null\n",
				 boot, sz->size, i);
			return -ENOMEM;
		}
		for (off = 0; off < sz->size; off += PAGE_SIZE)
			((volatile char *)p)[off] = (char)(off >> PAGE_SHIFT);
		ich_sink += (uintptr_t)p + ((volatile char *)p)[sz->size - 1];
		t2 = get_cycles();
		vfree(p);
		ich_alloc_samples[i] = t1 - t0;
		ich_free_samples[i] = get_cycles() - t2;
	}

	ich_report_samples(boot, sz, "alloc", ich_alloc_samples);
	ich_report_samples(boot, sz, "free", ich_free_samples);
	return 0;
}

static int ich_run_all(void)
{
	const char *boot = NULL;
	char state_line[96];
	unsigned int s;
	int ret;

	ich_report_len = 0;
	ich_report[0] = '\0';

	ret = ich_read_state(&boot, state_line, sizeof(state_line));
	if (ret) {
		ich_emit("ICH1 abort reason=state-unreadable err=%d\n", ret);
		return ret;
	}

	ich_emit("ICH1 header boot=%s state=\"%s\" unit=cycles "
		 "samples=%d warmup=%d door=vmalloc/vfree\n",
		 boot, state_line, ICH_SAMPLES, ICH_WARMUP);

	for (s = 0; s < ARRAY_SIZE(ich_sizes); s++) {
		ret = ich_run_size(boot, &ich_sizes[s]);
		if (ret)
			return ret;
	}

	ich_emit("ICH1 done boot=%s sink=%llu\n",
		 boot, (unsigned long long)ich_sink);
	return 0;
}

static ssize_t ich_proc_write(struct file *file, const char __user *ubuf,
			      size_t count, loff_t *ppos)
{
	int ret;

	if (mutex_lock_interruptible(&ich_run_lock))
		return -EINTR;
	ret = ich_run_all();
	mutex_unlock(&ich_run_lock);
	return ret ? ret : (ssize_t)count;
}

static ssize_t ich_proc_read(struct file *file, char __user *ubuf,
			     size_t count, loff_t *ppos)
{
	return simple_read_from_buffer(ubuf, count, ppos,
				       ich_report, ich_report_len);
}

static const struct proc_ops ich_proc_ops = {
	.proc_read	= ich_proc_read,
	.proc_write	= ich_proc_write,
	.proc_lseek	= default_llseek,
};

static int __init ich_init(void)
{
	if (!proc_create("iscandar_cycles_harness", 0644, NULL,
			 &ich_proc_ops))
		return -ENOMEM;
	pr_info("iscandar_cycles_harness: ready; write to "
		"/proc/iscandar_cycles_harness to run\n");
	return 0;
}

static void __exit ich_exit(void)
{
	remove_proc_entry("iscandar_cycles_harness", NULL);
}

module_init(ich_init);
module_exit(ich_exit);

MODULE_DESCRIPTION("KR12 cycles harness: vmalloc-door timing distributions, nano and tiny, serving vs control");
MODULE_LICENSE("GPL");
