//
// Created by Patrick Li on 1/7/2023.
//
// Checked by Patrick Li on 10/19/2023
//

#include "pl_class.h"
#include "pl_error.h"

#ifdef PL_TEST

    #include "pl_unittest.h"

#endif//PL_TEST

/*-----------------------------------------------------------------------------
 |  Inherit
 ----------------------------------------------------------------------------*/

static int inherit(const int derived, const int base)
{
    pl_error_expect(derived >= 0 && derived < PL_NUM_CLASS,
                    PL_ERROR_UNDEFINED_CLASS,
                    "Undefined class [%d]!", derived);

    pl_error_expect(base >= 0 && base < PL_NUM_CLASS,
                    PL_ERROR_UNDEFINED_CLASS,
                    "Undefined class [%d]!", base);

    // Check all parents of the derived classes.
    int current = derived;
    while (current != base && current != -1)
    {
        current = PL_CLASS_INHERIT[current];
    }
    return current == -1 ? 0 : 1;
}

static pl_unittest_summary test_inherit(void)
{
    pl_unittest_summary summary = pl_unittest_new_summary();

    pl_unittest_expect_error_is(summary, PL_ERROR_UNDEFINED_CLASS, inherit(PL_NUM_CLASS, PL_CLASS_CHAR));
    pl_unittest_expect_error_is(summary, PL_ERROR_UNDEFINED_CLASS, inherit(-1, PL_CLASS_CHAR));
    pl_unittest_expect_error_is(summary, PL_ERROR_UNDEFINED_CLASS, inherit(PL_CLASS_CHAR, PL_NUM_CLASS));
    pl_unittest_expect_error_is(summary, PL_ERROR_UNDEFINED_CLASS, inherit(PL_CLASS_CHAR, -1));
    pl_unittest_expect_error_is(summary, PL_ERROR_UNDEFINED_CLASS, inherit(-1, -1));
    pl_unittest_expect_error_is(summary, PL_ERROR_UNDEFINED_CLASS, inherit(PL_NUM_CLASS, -1));
    pl_unittest_expect_error_is(summary, PL_ERROR_UNDEFINED_CLASS, inherit(-1, PL_NUM_CLASS));

    pl_unittest_expect_true(summary, inherit(PL_CLASS_CHAR, PL_CLASS_INT) == 0);
    pl_unittest_expect_true(summary, inherit(PL_CLASS_INT, PL_CLASS_INT) == 1);

    return summary;
}

/*-----------------------------------------------------------------------------
 |  Get base type
 ----------------------------------------------------------------------------*/

static int type(const int derived)
{
    pl_error_expect(derived >= 0 && derived < PL_NUM_CLASS,
                    PL_ERROR_UNDEFINED_CLASS,
                    "Undefined class [%d]!", derived);

    // Recursively find the base class.
    int current = derived;
    while (PL_CLASS_INHERIT[current] != -1)
    {
        current = PL_CLASS_INHERIT[current];
    }

    return current;
}

static pl_unittest_summary test_type(void)
{
    pl_unittest_summary summary = pl_unittest_new_summary();

    pl_unittest_expect_error_is(summary, PL_ERROR_UNDEFINED_CLASS, type(PL_NUM_CLASS));
    pl_unittest_expect_error_is(summary, PL_ERROR_UNDEFINED_CLASS, type(-1));

    pl_unittest_expect_true(summary, type(PL_CLASS_CHAR) == PL_CLASS_CHAR);

    return summary;
}

/*-----------------------------------------------------------------------------
 |  Get namespace
 ----------------------------------------------------------------------------*/

static void test(void)
{
#ifdef PL_TEST
    printf("In file: %s\n", __FILE__);
    pl_unittest_print_summary(test_inherit());
    pl_unittest_print_summary(test_type());
#else
    puts("Test mode is disabled!");
#endif//PL_TEST
}


pl_class_ns pl_class_get_ns(void)
{
    static const pl_class_ns class_ns = {.inherit = inherit,
                                         .type    = type,
                                         .test    = test};

    return class_ns;
}
