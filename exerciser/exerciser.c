/*
 * iscandar-exerciser: the controlled page-exercising application
 * for Paper 3 ("Debian to Iscandar: Full Steam Ahead!").
 *
 * The pattern is fixed in advance and is the pattern of record
 * in paper-3-exerciser-pattern.md in the Iscandar-ram repo:
 * Phase A fill, Phase B halve (anchors standing), Phase C churn,
 * Phase D drain to zero. The program prints its prediction sheet,
 * computed from its own class tables, before it allocates
 * anything; the audit reads predicted against actual and the
 * prediction is never edited after a run.
 *
 * Every page of every block is stamped on allocation with the
 * block's allocation serial and the page's index in the block.
 * Before each release the stamps of everything held are
 * verified. A stamp that reads back wrong is an invariant
 * violation: the program records it in its ledger, stops, and
 * exits with a failure status. No retries, no fallbacks.
 *
 * Menu (console, 10 second timeout, default the standard run):
 *   1  standard run (3 churn cycles)
 *   2  extended run (9 churn cycles)
 *   3  ledger check only (print the prediction sheet, allocate
 *      nothing)
 *   4  exit without exercising
 * The choice may also be given as argv[1]: standard, extended,
 * check, none.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <unistd.h>

#define NCLASSES 8
#define ANCHOR_FROM 6 /* classes 64 and 128 pages are the anchors */

static const int class_pages[NCLASSES] = {1, 2, 4, 8, 16, 32, 64, 128};
static const int class_count[NCLASSES] = {1024, 512, 256, 128,
                                          64, 32, 16, 16};

typedef struct {
    char *addr;
    int cls;
    uint64_t serial;
} Block;

static Block *held = NULL;
static size_t held_n = 0, held_cap = 0;
static long page_size = 4096;
static uint64_t next_serial = 1;

static long live_blocks = 0, live_pages = 0, peak_pages = 0;
static long alloc_by_class[NCLASSES] = {0}, free_by_class[NCLASSES] = {0};
static long total_allocs = 0, total_frees = 0;

static FILE *ledger = NULL;

static void log_line(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    if (ledger) {
        va_start(ap, fmt);
        vfprintf(ledger, fmt, ap);
        va_end(ap);
        fflush(ledger);
    }
    fflush(stdout);
}

static void held_push(Block b)
{
    if (held_n == held_cap) {
        held_cap = held_cap ? held_cap * 2 : 4096;
        held = realloc(held, held_cap * sizeof(Block));
        if (!held) { perror("realloc"); exit(2); }
    }
    held[held_n++] = b;
}

static Block held_take(size_t i)
{
    Block b = held[i];
    memmove(&held[i], &held[i + 1], (held_n - i - 1) * sizeof(Block));
    held_n--;
    return b;
}

/* Allocate one block of class cls, stamp every page, hold it. */
static int alloc_block(int cls)
{
    long bytes = (long)class_pages[cls] * page_size;
    char *addr = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        log_line("FAILURE allocation refused: class %d pages (mmap)\n",
                 class_pages[cls]);
        return -1;
    }
    Block b = {addr, cls, next_serial++};
    for (int p = 0; p < class_pages[cls]; p++)
        *(uint64_t *)(addr + (long)p * page_size) =
            (b.serial << 16) | (uint64_t)p;
    held_push(b);
    alloc_by_class[cls]++;
    total_allocs++;
    live_blocks++;
    live_pages += class_pages[cls];
    if (live_pages > peak_pages) peak_pages = live_pages;
    return 0;
}

static void free_block(Block b)
{
    munmap(b.addr, (long)class_pages[b.cls] * page_size);
    free_by_class[b.cls]++;
    total_frees++;
    live_blocks--;
    live_pages -= class_pages[b.cls];
}

/* Verify the stamps of everything held. 0 if all read back. */
static int verify_all(const char *where)
{
    for (size_t i = 0; i < held_n; i++) {
        Block *b = &held[i];
        for (int p = 0; p < class_pages[b->cls]; p++) {
            uint64_t want = (b->serial << 16) | (uint64_t)p;
            uint64_t got =
                *(uint64_t *)(b->addr + (long)p * page_size);
            if (got != want) {
                log_line("INVARIANT VIOLATION at %s: block serial %llu, "
                         "class %d pages, page %d: expected %llu, "
                         "found %llu\n",
                         where, (unsigned long long)b->serial,
                         class_pages[b->cls], p,
                         (unsigned long long)want,
                         (unsigned long long)got);
                return -1;
            }
        }
    }
    return 0;
}

static void print_prediction(int cycles)
{
    long a_blocks = 0, a_pages = 0, b_blocks = 0, b_pages = 0;
    for (int c = 0; c < NCLASSES; c++) {
        a_blocks += class_count[c];
        a_pages += (long)class_count[c] * class_pages[c];
    }
    for (int c = 0; c < ANCHOR_FROM; c++) {
        b_blocks += class_count[c] / 2;
        b_pages += (long)(class_count[c] / 2) * class_pages[c];
    }
    long tot = a_blocks + (long)cycles * b_blocks;
    log_line("Prediction sheet (computed from the pattern alone)\n");
    log_line("  allocations %ld, frees %ld\n", tot, tot);
    log_line("  peak live %ld pages; live after Phase B %ld pages; "
             "end state 0 pages\n",
             a_pages, a_pages - b_pages);
    log_line("  allocations by class:");
    for (int c = 0; c < NCLASSES; c++) {
        long n = class_count[c] +
                 (c < ANCHOR_FROM ? (long)cycles * class_count[c] / 2 : 0);
        log_line(" %d pages x %ld;", class_pages[c], n);
    }
    log_line("\n");
}

static int run_pattern(int cycles)
{
    size_t phase_a_held;
    print_prediction(cycles);

    /* Phase A: fill, in class order. */
    for (int c = 0; c < NCLASSES; c++)
        for (int i = 0; i < class_count[c]; i++)
            if (alloc_block(c) != 0) return 2;
    phase_a_held = held_n;
    log_line("Phase A done: %ld blocks, %ld pages live\n",
             live_blocks, live_pages);

    /* Phase B: verify, then halve the non-anchor classes,
       freeing every odd-indexed block within its class. */
    if (verify_all("Phase B entry") != 0) return 3;
    {
        int seen[NCLASSES] = {0};
        size_t i = 0;
        while (i < held_n) {
            int cls = held[i].cls;
            int idx = seen[cls]++;
            if (cls < ANCHOR_FROM && (idx % 2 == 1)) {
                free_block(held_take(i));
            } else {
                i++;
            }
        }
    }
    (void)phase_a_held;
    log_line("Phase B done: %ld blocks, %ld pages live\n",
             live_blocks, live_pages);

    /* Phase C: churn. Each cycle re-allocates exactly the set
       freed in Phase B, verifies everything held, then frees
       the cycle's blocks. */
    for (int cycle = 1; cycle <= cycles; cycle++) {
        size_t cycle_start = held_n;
        for (int c = 0; c < ANCHOR_FROM; c++)
            for (int i = 0; i < class_count[c] / 2; i++)
                if (alloc_block(c) != 0) return 2;
        if (verify_all("Phase C cycle") != 0) return 3;
        while (held_n > cycle_start)
            free_block(held_take(cycle_start));
        log_line("Phase C cycle %d done: %ld blocks, %ld pages live\n",
                 cycle, live_blocks, live_pages);
    }

    /* Phase D: verify, then drain to zero. */
    if (verify_all("Phase D entry") != 0) return 3;
    while (held_n > 0)
        free_block(held_take(0));
    log_line("Phase D done: %ld blocks, %ld pages live\n",
             live_blocks, live_pages);

    log_line("Actual: allocations %ld, frees %ld, peak live %ld pages, "
             "end state %ld pages\n",
             total_allocs, total_frees, peak_pages, live_pages);
    log_line("Allocations by class:");
    for (int c = 0; c < NCLASSES; c++)
        log_line(" %d pages x %ld;", class_pages[c], alloc_by_class[c]);
    log_line("\nFrees by class:     ");
    for (int c = 0; c < NCLASSES; c++)
        log_line(" %d pages x %ld;", class_pages[c], free_by_class[c]);
    log_line("\n");

    {
        /* Compare the actual totals against the prediction
           computed from the same class tables, as printed on
           the sheet above. A mismatch is a finding. */
        long a_blocks = 0, a_pages = 0, b_blocks = 0;
        for (int c = 0; c < NCLASSES; c++) {
            a_blocks += class_count[c];
            a_pages += (long)class_count[c] * class_pages[c];
        }
        for (int c = 0; c < ANCHOR_FROM; c++)
            b_blocks += class_count[c] / 2;
        long predicted = a_blocks + (long)cycles * b_blocks;
        if (total_allocs != predicted || total_frees != predicted ||
            live_pages != 0 || peak_pages != a_pages) {
            log_line("Result: actual totals differ from the "
                     "prediction; a finding.\n");
            return 2;
        }
    }
    log_line("Result: run complete, actual matches prediction, "
             "balanced, end state zero.\n");
    return 0;
}

static int choose_mode(const char *arg)
{
    if (arg) {
        if (strcmp(arg, "standard") == 0) return 1;
        if (strcmp(arg, "extended") == 0) return 2;
        if (strcmp(arg, "check") == 0) return 3;
        if (strcmp(arg, "none") == 0) return 4;
        return 1;
    }
    if (isatty(STDIN_FILENO)) {
        char buf[32];
        fd_set rf;
        struct timeval tv;
        printf("Iscandar page exerciser\n"
               "  1  standard run (default)\n"
               "  2  extended run\n"
               "  3  ledger check only\n"
               "  4  exit without exercising\n"
               "Choice (10 seconds): ");
        fflush(stdout);
        FD_ZERO(&rf);
        FD_SET(STDIN_FILENO, &rf);
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        if (select(STDIN_FILENO + 1, &rf, NULL, NULL, &tv) > 0 &&
            fgets(buf, sizeof buf, stdin)) {
            if (buf[0] >= '1' && buf[0] <= '4') return buf[0] - '0';
        }
        printf("\n");
    }
    return 1;
}

int main(int argc, char **argv)
{
    int mode, rc;
    page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) page_size = 4096;

    ledger = fopen("/var/log/iscandar-exerciser/ledger.txt", "a");
    if (!ledger) ledger = fopen("iscandar-exerciser-ledger.txt", "a");

    mode = choose_mode(argc > 1 ? argv[1] : NULL);
    switch (mode) {
    case 2:
        log_line("Exerciser: extended run\n");
        rc = run_pattern(9);
        break;
    case 3:
        log_line("Exerciser: ledger check only\n");
        print_prediction(3);
        rc = 0;
        break;
    case 4:
        log_line("Exerciser: exiting without exercising\n");
        rc = 0;
        break;
    default:
        log_line("Exerciser: standard run\n");
        rc = run_pattern(3);
        break;
    }
    if (ledger) fclose(ledger);
    return rc;
}
