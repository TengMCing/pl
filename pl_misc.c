//
// Created by Patrick Li on 29/6/2023.
//

#include "pl_misc.h"

#ifdef PL_TEST
    #include "pl_unittest.h"
#endif//PL_TEST

/*-----------------------------------------------------------------------------
 |  Compare address (mainly used by qsort)
 ----------------------------------------------------------------------------*/

/// Compare two chars.
/// @return 1 if a > b, -1 if a \< b, 0 otherwise.
static int compare_char(const void *const a, const void *const b)
{
    const char arg1 = ((const char *) a)[0];
    const char arg2 = ((const char *) b)[0];
    if (arg1 > arg2)
        return 1;
    if (arg1 < arg2)
        return -1;
    return 0;
}

/// Compare two ints.
/// @return 1 if a > b, -1 if a \< b, 0 otherwise.
static int compare_int(const void *const a, const void *const b)
{
    const int arg1 = ((const int *) a)[0];
    const int arg2 = ((const int *) b)[0];
    if (arg1 > arg2)
        return 1;
    if (arg1 < arg2)
        return -1;
    return 0;
}

/// Compare two doubles.
/// @return 1 if a > b, -1 if a \< b, 0 otherwise.
static int compare_double(const void *const a, const void *const b)
{
    const double arg1 = ((const double *) a)[0];
    const double arg2 = ((const double *) b)[0];
    if (arg1 > arg2)
        return 1;
    if (arg1 < arg2)
        return -1;
    return 0;
}

/// Compare two addresses.
/// @return 1 if a > b, -1 if a \< b, 0 otherwise.
static int compare_address(const void *const a, const void *const b)
{
    if (a > b)
        return 1;
    if (a < b)
        return -1;
    return 0;
}

static pl_unittest_summary test_compare(void)
{
    pl_unittest_summary summary = pl_unittest_new_summary();

    char char_x[1] = {'c'};
    char char_y[1] = {'b'};
    pl_unittest_expect_true(summary, compare_char(char_x, char_y) == 1);
    pl_unittest_expect_true(summary, compare_char(char_y, char_y) == 0);
    pl_unittest_expect_true(summary, compare_char(char_y, char_x) == -1);

    return summary;
}

/*-----------------------------------------------------------------------------
 |  Get misc namespace
 ----------------------------------------------------------------------------*/

static void test(void)
{
    printf("In file: %s\n", __FILE__);
    pl_unittest_print_summary(test_compare());
}

pl_misc_ns pl_misc_get_ns(void)
{
#ifdef PL_TEST
    static const pl_misc_ns misc_ns = {.compare_char    = compare_char,
                                       .compare_int     = compare_int,
                                       .compare_double  = compare_double,
                                       .compare_address = compare_address,
                                       .test            = test};
#else
    static const pl_misc_ns misc_ns = {.compare_char    = compare_char,
                                       .compare_int     = compare_int,
                                       .compare_double  = compare_double,
                                       .compare_address = compare_address};
#endif//PL_TEST

    return misc_ns;
}
