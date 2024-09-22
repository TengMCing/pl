//
// Created by Patrick Li on 29/6/2023.
//

#include "pl_misc.h"
#include "stdarg.h"

#ifdef PL_TEST

#include "pl_unittest.h"

#endif//PL_TEST

/*-----------------------------------------------------------------------------
 |  Compare char (mainly used by qsort)
 ----------------------------------------------------------------------------*/

static int compare_char(const void *const a, const void *const b) {
    const char arg1 = ((const char *) a)[0];
    const char arg2 = ((const char *) b)[0];
    if (arg1 > arg2)
        return 1;
    if (arg1 < arg2)
        return -1;
    return 0;
}

static pl_unittest_summary test_compare_char(void) {
    pl_unittest_summary summary = pl_unittest_new_summary();

    char x[1] = {'c'};
    char y[1] = {'b'};
    pl_unittest_expect_true(summary, compare_char(x, y) == 1);
    pl_unittest_expect_true(summary, compare_char(y, y) == 0);
    pl_unittest_expect_true(summary, compare_char(y, x) == -1);

    return summary;
}

/*-----------------------------------------------------------------------------
 |  Compare int (mainly used by qsort)
 ----------------------------------------------------------------------------*/

static int compare_int(const void *const a, const void *const b) {
    const int arg1 = ((const int *) a)[0];
    const int arg2 = ((const int *) b)[0];
    if (arg1 > arg2)
        return 1;
    if (arg1 < arg2)
        return -1;
    return 0;
}

static pl_unittest_summary test_compare_int(void) {
    pl_unittest_summary summary = pl_unittest_new_summary();

    int x[1] = {'c'};
    int y[1] = {'b'};
    pl_unittest_expect_true(summary, compare_int(x, y) == 1);
    pl_unittest_expect_true(summary, compare_int(y, y) == 0);
    pl_unittest_expect_true(summary, compare_int(y, x) == -1);

    return summary;
}

/*-----------------------------------------------------------------------------
 |  Compare long (mainly used by qsort)
 ----------------------------------------------------------------------------*/

static int compare_long(const void *const a, const void *const b) {
    const long arg1 = ((const long *) a)[0];
    const long arg2 = ((const long *) b)[0];
    if (arg1 > arg2)
        return 1;
    if (arg1 < arg2)
        return -1;
    return 0;
}

static pl_unittest_summary test_compare_long(void) {
    pl_unittest_summary summary = pl_unittest_new_summary();

    long x[1] = {'c'};
    long y[1] = {'b'};
    pl_unittest_expect_true(summary, compare_int(x, y) == 1);
    pl_unittest_expect_true(summary, compare_int(y, y) == 0);
    pl_unittest_expect_true(summary, compare_int(y, x) == -1);

    return summary;
}

/*-----------------------------------------------------------------------------
 |  Compare double (mainly used by qsort)
 ----------------------------------------------------------------------------*/

static int compare_double(const void *const a, const void *const b) {
    const double arg1 = ((const double *) a)[0];
    const double arg2 = ((const double *) b)[0];
    if (arg1 > arg2)
        return 1;
    if (arg1 < arg2)
        return -1;
    return 0;
}

static pl_unittest_summary test_compare_double(void) {
    pl_unittest_summary summary = pl_unittest_new_summary();

    double x[1] = {2.0};
    double y[1] = {1.0};
    pl_unittest_expect_true(summary, compare_double(x, y) == 1);
    pl_unittest_expect_true(summary, compare_double(y, y) == 0);
    pl_unittest_expect_true(summary, compare_double(y, x) == -1);

    return summary;
}

/*-----------------------------------------------------------------------------
 |  Compare address (mainly used by qsort)
 ----------------------------------------------------------------------------*/

static int compare_pointer(const void *const a, const void *const b) {
    if (a > b)
        return 1;
    if (a < b)
        return -1;
    return 0;
}

static pl_unittest_summary test_compare_pointer(void) {
    pl_unittest_summary summary = pl_unittest_new_summary();

    int x_int = 0;
    int y_int = 1;
    void *x[1] = {&x_int};
    void *y[1] = {&y_int};
    pl_unittest_expect_true(summary, compare_pointer(x, y) == 1);
    pl_unittest_expect_true(summary, compare_pointer(y, y) == 0);
    pl_unittest_expect_true(summary, compare_pointer(y, x) == -1);

    return summary;
}


/*-----------------------------------------------------------------------------
 |  Get misc namespace
 ----------------------------------------------------------------------------*/

static void test(void) {
    printf("In file: %s\n", __FILE__);
    pl_unittest_print_summary(test_compare_char());
    pl_unittest_print_summary(test_compare_int());
    pl_unittest_print_summary(test_compare_long());
    pl_unittest_print_summary(test_compare_double());
    pl_unittest_print_summary(test_compare_pointer());
}

pl_misc_ns pl_misc_get_ns(void) {
#ifdef PL_TEST
    static const pl_misc_ns misc_ns = {.compare_char    = compare_char,
            .compare_int     = compare_int,
            .compare_long    = compare_long,
            .compare_double  = compare_double,
            .compare_pointer = compare_pointer,
            .test            = test};
#else
    static const pl_misc_ns misc_ns = {.compare_char    = compare_char,
                                       .compare_int     = compare_int,
                                       .compare_double  = compare_double,
                                       .compare_address = compare_address};
#endif//PL_TEST

    return misc_ns;
}
