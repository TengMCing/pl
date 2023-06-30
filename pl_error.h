//
// Created by Patrick Li on 29/6/2023.
//

#ifndef PL_PL_ERROR_H
#define PL_PL_ERROR_H

/*-----------------------------------------------------------------------------
 |  Error
 ----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 |  Error code
 ----------------------------------------------------------------------------*/

#define PL_ERROR_STACKOVERFLOW 1
#define PL_ERROR_INDEX_OUT_OF_BOUND 2

/*-----------------------------------------------------------------------------
 |  Error report
 ----------------------------------------------------------------------------*/

/// Maximum length of the error message.
#define PL_ERROR_MAX_MESSAGE_LEN 256

/// Error report.
/// @param error_code (const int). Error code. 0 means no error.
/// @param error_message (const char *const). Error message.
typedef struct pl_error
{
    const int error_code;
    const char *const error_message;
} pl_error;


/*-----------------------------------------------------------------------------
 |  Expect
 ----------------------------------------------------------------------------*/

/// Expect a condition to be true.
/// @param condition A condition.
/// @param return_type The return type of the current function.
/// This return type needs to be a struct containing a pl_error field
/// named `error`. The return values except for the error report will be default initialized.
/// @param error_code (const int). The error ID.
/// @param format (const char *). The format of the error message.
/// @param ... Additional arguments used for snprintf.
#define pl_error_expect(condition, return_type, error_code, format, ...)   \
    do {                                                                   \
        if (!(condition))                                                  \
        {                                                                  \
            const pl_error_ns error_ns = pl_error_get_ns();                \
            return (return_type){.error = error_ns.create(error_code,      \
                                                          __func__,        \
                                                          __FILE__,        \
                                                          __LINE__,        \
                                                          format,          \
                                                          ##__VA_ARGS__)}; \
        }                                                                  \
    } while (0)


/*-----------------------------------------------------------------------------
 |  Expect and exit
 ----------------------------------------------------------------------------*/

/// Expect a condition to be true.
/// @param condition A condition.
/// @param error_code (const int). The error ID.
/// @param format (const char *). The format of the error message.
/// @param ... Additional arguments used for snprintf.
#define pl_error_expect_and_exit(condition, error_code, format, ...)     \
    do {                                                                 \
        if (!(condition))                                                \
        {                                                                \
            const pl_error_ns error_ns = pl_error_get_ns();              \
            pl_error last_error        = error_ns.create(error_code,     \
                                                         __func__,       \
                                                         __FILE__,       \
                                                         __LINE__,       \
                                                         format,         \
                                                         ##__VA_ARGS__); \
            error_ns.print_and_exit(last_error);                         \
        }                                                                \
    } while (0)

/*-----------------------------------------------------------------------------
 |  Pass error to caller
 ----------------------------------------------------------------------------*/

/// Return the error to the caller.
/// @param this_error (pl_error). The error.
/// @param return_type Return type.
#define pl_error_pass_to_caller(this_error, return_type) \
    do {                                                 \
        if ((this_error).error_code)                     \
            return (return_type){.error = (this_error)}; \
    } while (0)

/*-----------------------------------------------------------------------------
 |  Error namespace
 ----------------------------------------------------------------------------*/

/// Namespace of error.
typedef struct pl_error_ns
{

    /// Create an error report.
    /// @param error_code (int). The error ID.
    /// @param function_name (const char *). Function name.
    /// @param file_name (const char *). File name.
    /// @param line (int). The line number.
    /// @param format (const char *). Format for the additional error message.
    /// @param ... Additional arguments passed to snprintf.
    pl_error (*const create)(int error_code,
                             const char *function_name,
                             const char *file_name,
                             int line,
                             const char *format,
                             ...);

    /// Print the error message if the error code is non-zero.
    /// @param error (pl_error). The error.
    void (*const optionally_print)(pl_error error);

    /// Print the error message and then exit if the error code is non-zero.
    /// @param error (pl_error). The error.
    void (*const optionally_print_and_exit)(pl_error error);
} pl_error_ns;

/// Get error namespace.
/// @return Namespace of error.
pl_error_ns pl_error_get_ns(void);

#endif//PL_PL_ERROR_H
