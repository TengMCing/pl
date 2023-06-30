//
// Created by Patrick Li on 29/6/2023.
//

#ifndef PL_PL_MISC_H
#define PL_PL_MISC_H

/*-----------------------------------------------------------------------------
 |  Misc
 ----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 |  Count the number of arguments
 ----------------------------------------------------------------------------*/

/// Count the number of arguments.
#define pl_misc_count_arg(...) pl_misc_count_arg_list(0, ##__VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define pl_misc_count_arg_list(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count

/*-----------------------------------------------------------------------------
 |  Get the nth argument
 ----------------------------------------------------------------------------*/

/// Get the first argument.
#define pl_misc_arg1(arg1, ...) arg1

/// Get the second argument.
#define pl_misc_arg2(arg1, arg2, ...) arg2

/// Get the third argument.
#define pl_misc_arg3(arg1, arg2, arg3, ...) arg3

/// Get the fourth argument.
#define pl_misc_arg4(arg1, arg2, arg3, arg4, ...) arg4

/// Get the fifth argument.
#define pl_misc_arg5(arg1, arg2, arg3, arg4, arg5, ...) arg5

/*-----------------------------------------------------------------------------
 |  For loop
 ----------------------------------------------------------------------------*/

#define pl_misc_for_i_name(line) pl_misc_paste_arg(_i, line)
#define pl_misc_for_j_name(line) pl_misc_paste_arg(_j, line)

/// A variadic macro similar to `range()` in python. The counter i is an int.
/// @arg1 start (int). The start.
/// @arg2 start (int), end (int). The start and the end.
/// @arg3 start (int), end (int), step (int). The start, the end and the step size.
#define pl_misc_for_i(...) pl_misc_arg4(__VA_ARGS__, pl_misc_for_i_3, pl_misc_for_i_2, pl_misc_for_i_1)(__VA_ARGS__)
#define pl_misc_for_i_1(end)                      \
    const int pl_misc_for_i_name(__LINE__) = end; \
    for (int i = 0; i < pl_misc_for_i_name(__LINE__); i++)
#define pl_misc_for_i_2(start, end)               \
    const int pl_misc_for_i_name(__LINE__) = end; \
    for (int i = start; i < pl_misc_for_i_name(__LINE__); i++)
#define pl_misc_for_i_3(start, end, step)         \
    const int pl_misc_for_i_name(__LINE__) = end; \
    for (int i = start; i < pl_misc_for_i_name(__LINE__); i += (step))

/// A variadic macro similar to `range()` in python. The counter j is an int.
/// @arg1 start (int). The start.
/// @arg2 start (int), end (int). The start and the end.
/// @arg3 start (int), end (int), step (int). The start, the end and the step size.
#define pl_misc_for_j(...) pl_misc_arg4(__VA_ARGS__, pl_misc_for_j_3, pl_misc_for_j_2, pl_misc_for_j_1)(__VA_ARGS__)
#define pl_misc_for_j_1(end)                      \
    const int pl_misc_for_j_name(__LINE__) = end; \
    for (int j = 0; j < pl_misc_for_j_name(__LINE__); j++)
#define pl_misc_for_j_2(start, end)               \
    const int pl_misc_for_j_name(__LINE__) = end; \
    for (int j = start; j < pl_misc_for_j_name(__LINE__); j++)
#define pl_misc_for_j_3(start, end, step)         \
    const int pl_misc_for_j_name(__LINE__) = end; \
    for (int j = start; j < pl_misc_for_i_name(__LINE__); j += (step))

/*-----------------------------------------------------------------------------
 |  Paste argument
 ----------------------------------------------------------------------------*/

/// Paste two tokens together.
#define pl_misc_paste_arg(a, b) a##b

/*-----------------------------------------------------------------------------
 |  Compile time assert
 ----------------------------------------------------------------------------*/

/// Compile time condition check.
/// @param condition (int) Boolean expression.
/// @param file (string literal) The file name.
#define pl_misc_compile_time_assert(condition, file) pl_misc_compile_time_assert_line(condition, __LINE__, file)
#define pl_misc_compile_time_assert_line(condition, line, file) typedef char pl_misc_paste_arg(assertion_failed_##file##_, line)[2 * !!(condition) -1];

/*-----------------------------------------------------------------------------
 |  Misc namespace
 ----------------------------------------------------------------------------*/

/// Namespace of misc.
typedef struct pl_misc_ns
{
    /*-----------------------------------------------------------------------------
     |  Compare address (mainly used by qsort)
     ----------------------------------------------------------------------------*/

    /// Compare two chars.
    /// @param a (const void *). Pointer to a char.
    /// @param b (const void *). Pointer to another char.
    /// @return 1 if a > b, -1 if a \< b and 0 if a == b.
    int (*const compare_char)(const void *a, const void *b);

    /// Compare two ints.
    /// @param a (const void *). Pointer to an int.
    /// @param b (const void *). Pointer to another int.
    /// @return 1 if a > b, -1 if a \< b and 0 if a == b.
    int (*const compare_int)(const void *a, const void *b);

    /// Compare two doubles.
    /// @param a (const void *). Pointer to a double.
    /// @param b (const void *). Pointer to another double.
    /// @return 1 if a > b, -1 if a \< b and 0 if a == b.
    int (*const compare_double)(const void *a, const void *b);

    /// Compare two addresses.
    /// @param a (const void *). An address.
    /// @param b (const void *). Another address.
    /// @return 1 if a > b, -1 if a \< b and 0 if a == b.
    int (*const compare_address)(const void *a, const void *b);
} pl_misc_ns;

/// Get misc namespace.
/// @return Namespace of misc.
pl_misc_ns pl_misc_get_ns(void);

#endif//PL_PL_MISC_H
