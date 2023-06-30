//
// Created by Patrick Li on 29/6/2023.
//

#include "pl_misc.h"

/*-----------------------------------------------------------------------------
 |  Compare address (mainly used by qsort)
 ----------------------------------------------------------------------------*/

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

static int compare_address(const void *const a, const void *const b)
{
    if (a > b)
        return 1;
    if (a < b)
        return -1;
    return 0;
}

/*-----------------------------------------------------------------------------
 |  Get misc namespace
 ----------------------------------------------------------------------------*/

pl_misc_ns pl_misc_get_ns(void)
{
    static const pl_misc_ns misc_ns = {.compare_char    = compare_char,
                                       .compare_int     = compare_int,
                                       .compare_double  = compare_double,
                                       .compare_address = compare_address};
    return misc_ns;
}
