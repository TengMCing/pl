//
// Created by Patrick Li on 21/9/2023.
//

#ifndef PL_PL_UNITTEST_H
#define PL_PL_UNITTEST_H

#include "pl_error.h"
#include "pl_misc.h"
#include "stdio.h"

/// Maximum number of unit tests in a summary.
#define PL_UNITTEST_MAX_NUM 64

/// Summary of unit tests in a file.
/// @param name (const char *). Name of the summary.
/// @param total (int). Total number of unit tests.
/// @param success (int). Total number of passed unit tests.
/// @param success_bool (int[PL_UNITTEST_MAX_NUM]). A Boolean array for
/// storing the unit test result.
typedef struct pl_unittest_summary
{
    const char *name;
    int total;
    int success;
    int success_bool[PL_UNITTEST_MAX_NUM];
} pl_unittest_summary;

/// Init a unit test summary.
#define pl_unittest_new_summary() \
    (pl_unittest_summary)         \
    {                             \
        .name         = __func__, \
        .total        = 0,        \
        .success      = 0,        \
        .success_bool = { 0 }     \
    }

/// Expect the expression can be run without any error.
/// @param summary (pl_unittest_summary). A unit test summary.
/// @param expr An expression.
#define pl_unittest_expect_no_error(summary, expr)           \
    do {                                                     \
        pl_error_try                                         \
        {                                                    \
            (summary).total++;                               \
            expr;                                            \
            (summary).success_bool[(summary).total - 1] = 1; \
            (summary).success++;                             \
        }                                                    \
        pl_error_catch                                       \
        {                                                    \
            (summary).success_bool[(summary).total - 1] = 0; \
        }                                                    \
    } while (0)

/// Expect the expression can be run without any error and
/// the return equals to a result.
/// @param summary (pl_unittest_summary). A unit test summary.
/// @param cond An expression.
#define pl_unittest_expect_true(summary, cond)                   \
    do {                                                         \
        pl_error_try                                             \
        {                                                        \
            (summary).total++;                                   \
            if (cond)                                            \
            {                                                    \
                (summary).success_bool[(summary).total - 1] = 1; \
                (summary).success++;                             \
            }                                                    \
            else                                                 \
            {                                                    \
                (summary).success_bool[(summary).total - 1] = 0; \
            }                                                    \
        }                                                        \
        pl_error_catch                                           \
        {                                                        \
            (summary).success_bool[(summary).total - 1] = 0;     \
        }                                                        \
    } while (0)

/// Expect the expression will raise an error.
/// @param summary (pl_unittest_summary). A unit test summary.
/// @param error (int). Exception ID.
/// @param expr An expression.
#define pl_unittest_expect_error_is(summary, error, expr)        \
    do {                                                         \
        pl_error_try                                             \
        {                                                        \
            (summary).total++;                                   \
            expr;                                                \
            (summary).success_bool[(summary).total - 1] = 0;     \
        }                                                        \
        pl_error_catch                                           \
        {                                                        \
            if (pl_error_get_current() == (error))               \
            {                                                    \
                (summary).success_bool[(summary).total - 1] = 1; \
                (summary).success++;                             \
            }                                                    \
            else                                                 \
            {                                                    \
                (summary).success_bool[(summary).total - 1] = 0; \
            }                                                    \
        }                                                        \
    } while (0)


/// Print the unit test summary.
/// @param summary (pl_unittest_summary). A unit test summary.
static inline void pl_unittest_print_summary(pl_unittest_summary summary)
{
    printf("\t<%s>: %d/%d tests passed.\n", summary.name, summary.success, summary.total);
    if (summary.success != summary.total)
    {
        printf("\t\tTest ");
        pl_misc_for_i(summary.total - 1)
        {
            if (summary.success_bool[i] != 1)
                printf("%d, ", i);
        }
        if (summary.total > 0)
            if (summary.success_bool[summary.total - 1] != 1)
                printf("%d ", summary.total - 1);
        printf("failed!\n");
    }
}

#endif//PL_PL_UNITTEST_H
