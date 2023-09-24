//
// Created by Patrick Li on 29/6/2023.
//

#include "pl_error.h"
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"

#ifdef PL_TEST
    #include "pl_unittest.h"
    #include "string.h"
#endif//PL_TEST

/// A global buffer for storing error message.
static char global_error_message[PL_ERROR_MAX_MESSAGE_LEN] = {0};

/// A volatile global variable for storing exception frames.
extern volatile pl_error_exception pl_error_exception_frames = {0};

/*-----------------------------------------------------------------------------
 |  Save error message
 ----------------------------------------------------------------------------*/

/// Save error message to the global buffer.
/// @details The buffer will be reset if an encoding error occurred.
/// @param error (int). The error ID.
/// @param function_name (const char *). Function name.
/// @param file_name (const char *). File name.
/// @param line (int). The line number.
/// @param format (const char *). Format for the additional error message.
/// @param ... Additional arguments passed to snprintf.
static void save_error_message(const int error,
                               const char *const function_name,
                               const char *const file_name,
                               const int line,
                               const char *const format,
                               ...)
{
    // If the format of the error message is empty, do not rewrite the buffer
    if (format[0] == '\0')
        return;

    // Create the customized error message.
    char message[PL_ERROR_MAX_MESSAGE_LEN] = {0};
    va_list ap;
    va_start(ap, format);
    int message_len = vsnprintf(message, PL_ERROR_MAX_MESSAGE_LEN, format, ap);
    va_end(ap);

    // Check encoding error
    if (message_len < 0)
    {
        puts("PL Internal Error: Encounter an encoding error! The error message buffer will be reset!");
        message[0] = '\0';
    }

    // Rewrite the buffer
    int len = snprintf(global_error_message,
                       PL_ERROR_MAX_MESSAGE_LEN,
                       "[E%03d] Error raised by <%s> at %s:%d: %s\n",
                       error,
                       function_name,
                       file_name,
                       line,
                       message);

    // Check encoding error
    if (len < 0)
    {
        puts("PL Internal Error: Encounter an encoding error! The error message buffer will be reset!");
        global_error_message[0] = '\0';
    }
}

static pl_unittest_summary test_save_error_message(void)
{
    pl_unittest_summary summary = pl_unittest_new_summary();

    pl_unittest_expect_true(summary,
                            (save_error_message(1, "a", "b", 123, "test!"),
                             strcmp("[E001] Error raised by <a> at b:123: test!\n", global_error_message) == 0));
    pl_unittest_expect_true(summary,
                            (save_error_message(1, "aa", "bb", 1234, ""),
                             strcmp("[E001] Error raised by <a> at b:123: test!\n", global_error_message) == 0));
    pl_unittest_expect_true(summary,
                            (save_error_message(1, "aa", "bb", 1234, "test!"),
                             strcmp("[E001] Error raised by <aa> at bb:1234: test!\n", global_error_message) == 0));
    global_error_message[0] = '\0';

    return summary;
}

/*-----------------------------------------------------------------------------
 |  Long jump if catch statement exists
 ----------------------------------------------------------------------------*/

/// Attempt to perform a long jump because of an exception.
/// @details This function will not perform a long jump if the catch statement
/// is missing
/// @param error (int). Error ID.
static void long_jump_if_catch(const int error)
{
    pl_error_exception_frames.error = error;
    if (pl_error_exception_frames.frame)
        longjmp(*pl_error_exception_frames.frame, 1);
}

/*-----------------------------------------------------------------------------
 |  Default error handler for no catch statement
 ----------------------------------------------------------------------------*/

/// The default handler for no catch statement.
/// @details The default handler prints the error message and abort the program.
_Noreturn static void default_error_handler(void)
{
    puts(global_error_message);
#ifdef VDL_EXCEPTION_DISABLE
    puts("PL Internal Message: Program abort in no exception mode!");
#else
    puts("PL Internal Message: Program abort!");
#endif//VDL_EXCEPTION_DISABLE
    exit(EXIT_FAILURE);
}

/*-----------------------------------------------------------------------------
 |  Get error namespace
 ----------------------------------------------------------------------------*/

static void test(void)
{
    printf("In file: %s\n", __FILE__);
    pl_unittest_print_summary(test_save_error_message());
}


pl_error_ns pl_error_get_ns(void)
{
#ifdef PL_TEST
    static const pl_error_ns error_ns = {.save_error_message    = save_error_message,
                                         .long_jump_if_catch    = long_jump_if_catch,
                                         .default_error_handler = default_error_handler,
                                         .test                  = test};
#else
    static const pl_error_ns error_ns = {.save_error_message    = save_error_message,
                                         .long_jump_if_catch    = long_jump_if_catch,
                                         .default_error_handler = default_error_handler};
#endif//PL_TEST

    return error_ns;
}
