/* platform_pc.c
 * All PC-only code: command line, printing. Nothing else in the project
 * includes stdio. One run prints one CSV row per method.
 *
 * Usage: wearlevel [--header] [--method baseline|array|heap|all]
 *                  [--spares N] [--limit N]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim.h"
#include "ftl.h"
#include "fb.h"
#include "config.h"

#ifndef TRACE_FILE
#define TRACE_FILE "trace.h"
#endif

static void print_header(void)
{
    printf("trace,logical,spares,method,lifetime,max_write,avg_write,cmp_per_write,pool_bytes,checks\n");
}

/* Fixed-point output without floating point: two decimals. */
static void print_row(const char *method, uint16_t spares, const sim_result_t *r, int ok)
{
    uint64_t avg100 = (r->num_phys > 0u) ? (r->sum_wear * 100u) / r->num_phys : 0u;
    uint64_t cmp100 = (r->lifetime > 0u) ? (r->comparisons * 100u) / r->lifetime : 0u;

    printf("%s,%u,%u,%s,%lu,%lu,%lu.%02lu,%lu.%02lu,%lu,%s\n",
           TRACE_FILE, (unsigned)sim_num_logical(), (unsigned)spares, method,
           (unsigned long)r->lifetime, (unsigned long)r->max_wear,
           (unsigned long)(avg100 / 100u), (unsigned long)(avg100 % 100u),
           (unsigned long)(cmp100 / 100u), (unsigned long)(cmp100 % 100u),
           (unsigned long)r->pool_bytes, ok ? "ok" : "FAIL");
}

static int run_one(const char *method, uint16_t spares, uint32_t limit)
{
    sim_result_t r;
    int rc;

    if (strcmp(method, "baseline") == 0) {
        rc = sim_run(&ftl_baseline_ops, 0, 0, limit, &r);
        print_row("baseline", 0, &r, rc == 0);
    } else if (strcmp(method, "array") == 0) {
        rc = sim_run(&ftl_wl_ops, &fb_array_ops, spares, limit, &r);
        print_row("array", spares, &r, rc == 0);
    } else if (strcmp(method, "heap") == 0) {
        rc = sim_run(&ftl_wl_ops, &fb_heap_ops, spares, limit, &r);
        print_row("heap", spares, &r, rc == 0);
    } else {
        fprintf(stderr, "unknown method: %s\n", method);
        return -1;
    }
    if (rc != 0) {
        fprintf(stderr, "%s: %lu check(s) failed\n", method, (unsigned long)r.errors);
    }
    return rc;
}

int main(int argc, char **argv)
{
    const char *method = "all";
    uint16_t spares = 4;
    uint32_t limit = DEFAULT_WRITE_LIMIT;
    int header = 0;
    int rc = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--header") == 0) {
            header = 1;
        } else if (strcmp(argv[i], "--method") == 0 && i + 1 < argc) {
            method = argv[++i];
        } else if (strcmp(argv[i], "--spares") == 0 && i + 1 < argc) {
            spares = (uint16_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc) {
            limit = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else {
            fprintf(stderr, "usage: %s [--header] [--method baseline|array|heap|all] [--spares N] [--limit N]\n", argv[0]);
            return 2;
        }
    }
    if (header) {
        print_header();
    }
    if (strcmp(method, "all") == 0) {
        if (run_one("baseline", spares, limit) != 0) rc = 1;
        if (run_one("array", spares, limit) != 0) rc = 1;
        if (run_one("heap", spares, limit) != 0) rc = 1;
    } else if (run_one(method, spares, limit) != 0) {
        rc = 1;
    }
    return rc;
}
