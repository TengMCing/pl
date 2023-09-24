//
// Created by Patrick Li on 29/6/2023.
//

#ifndef PL_PL_ERROR_H
#define PL_PL_ERROR_H

#include "setjmp.h"

/*-----------------------------------------------------------------------------
 |  Error
 ----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 |  Error code
 ----------------------------------------------------------------------------*/

#define PL_ERROR_NONE 0
#define PL_ERROR_INDEX_OUT_OF_BOUND 1
#define PL_ERROR_ALLOC_FAIL 2
#define PL_ERROR_UNDEFINED_CLASS 3
#define PL_ERROR_INVALID_CAPACITY 4
#define PL_ERROR_INVALID_NULL_POINTER 5
#define PL_ERROR_INVALID_CLASS 6
#define PL_ERROR_INVALID_LENGTH 7
#define PL_ERROR_INVALID_NA 8
#define PL_ERROR_INVALID_FRAME 9
#define PL_ERROR_VARIABLE_NOT_FOUND 10
#define PL_ERROR_INVALID_VARIABLE_NAME 11


/*-----------------------------------------------------------------------------
 |  Error report
 ----------------------------------------------------------------------------*/

/// Maximum length of the error message.
#define PL_ERROR_MAX_MESSAGE_LEN 256

/*-----------------------------------------------------------------------------
 |  Exception
 ----------------------------------------------------------------------------*/

typedef volatile struct pl_error_exception
{
    jmp_buf *frame;
    volatile int error;
} pl_error_exception;

/// A volatile global variable for storing exception frames.
/// @param frame (jmp_buf *). The current frame.
/// @param error (volatile VDL_EXCEPTION_T). The error ID.
extern volatile pl_error_exception pl_error_exception_frames;

/// Get the current error ID.
#define pl_error_get_current() ((int){pl_error_exception_frames.error})

/*-----------------------------------------------------------------------------
 |  Try
 ----------------------------------------------------------------------------*/

// If the exception is disabled, no setup needs to be done.
#ifdef PL_ERROR_DISABLE
    /// @dscription Try one or more statements.
    /// @details Since the exception is disabled, the program will be aborted
    /// if an exception occurred.
    #define pl_error_try \
        {                \
            if (1)
#else
    /// @dscription Try one or more statements.
    /// @details pl_error_try needs to be paired with pl_error_catch_switch.
    #define pl_error_try                                                       \
        {                                                                      \
            jmp_buf *previous_frame, new_frame;                                \
            previous_frame                  = pl_error_exception_frames.frame; \
            pl_error_exception_frames.frame = (jmp_buf *) (&new_frame);        \
            pl_error_exception_frames.error = PL_ERROR_NONE;                   \
            if (setjmp(new_frame) == 0)                                        \
            {                                                                  \
                if (1)
#endif//PL_ERROR_DISABLE

/*-----------------------------------------------------------------------------
 |  Catch
 ----------------------------------------------------------------------------*/

// If the exception is disabled, the catch scope should not be run.
#ifdef PL_ERROR_DISABLE
    /// Catch an exception.
    /// @details Since the exception is disabled, the program will be aborted
    /// if an exception occurred. This block of code will never be run.
    #define pl_error_catch \
        else {}             \
        }                   \
        if (0)
#else
    /// @dscription Catch an exception.
    /// @details pl_error_try needs to be paired with pl_error_catch.
    /// You should always `pl_error_rethrow()` if the exception can not be handled.
    #define pl_error_catch                               \
        else {}                                           \
        pl_error_exception_frames.error = PL_ERROR_NONE;  \
        }                                                 \
        else {}                                           \
        pl_error_exception_frames.frame = previous_frame; \
        }                                                 \
        if (pl_error_exception_frames.error != PL_ERROR_NONE)
#endif//PL_ERROR_DISABLE

/*-----------------------------------------------------------------------------
 |  Throw
 ----------------------------------------------------------------------------*/

// If the exception is disabled, no jump will be performed.
#ifdef PL_ERROR_DISABLE
    /// Save an error message and throw an exception.
    /// @details Since the exception is disabled, the program will be aborted.
    /// @param error (int). Error ID.
    /// @param format (const char *). The format of the error message.
    /// @param ... Additional arguments used for snprintf.
    #define pl_error_throw(error, format, ...)                                                       \
        do {                                                                                         \
            const pl_error_ns error_ns = pl_error_get_ns();                                          \
            error_ns.save_error_message(error, __func__, __FILE__, __LINE__, format, ##__VA_ARGS__); \
            error_ns.default_error_handler();                                                        \
        } while (0)
#else
    /// Save an error message and throw an exception.
    /// @param error (int). Error ID.
    /// @param format (const char *). The format of the error message.
    /// @param ... Additional arguments used for snprintf.
    #define pl_error_throw(error, format, ...)                                                       \
        do {                                                                                         \
            const pl_error_ns error_ns = pl_error_get_ns();                                          \
            error_ns.save_error_message(error, __func__, __FILE__, __LINE__, format, ##__VA_ARGS__); \
            error_ns.long_jump_if_catch(error);                                                      \
            error_ns.default_error_handler();                                                        \
        } while (0)
#endif//PL_ERROR_DISABLE


/*-----------------------------------------------------------------------------
 |  Rethrow
 ----------------------------------------------------------------------------*/

// If the exception is disabled, no jump will be performed.
#ifdef PL_ERROR_DISABLE
    /// Rethrow the current exception.
    /// @details Since the exception is disabled, the program will be aborted.
    #define pl_error_rethrow()                                                                     \
        do {                                                                                       \
            const pl_error_ns error_ns = pl_error_get_ns();                                        \
            error_ns.save_error_message(pl_error_get_current(), __func__, __FILE__, __LINE__, ""); \
            error_ns.default_error_handler();                                                      \
        } while (0)
#else
    /// Rethrow the current exception.
    #define pl_error_rethrow()                                                                     \
        do {                                                                                       \
            const pl_error_ns error_ns = pl_error_get_ns();                                        \
            error_ns.save_error_message(pl_error_get_current(), __func__, __FILE__, __LINE__, ""); \
            error_ns.long_jump_if_catch(pl_error_get_current());                                   \
            error_ns.default_error_handler();                                                      \
        } while (0)
#endif//PL_ERROR_DISABLE


/*-----------------------------------------------------------------------------
 |  Exit Try
 ----------------------------------------------------------------------------*/

/// Exit the current pl_error_try block and skip one pl_error_catch.
#define pl_error_exit_try() pl_error_throw(PL_ERROR_NONE, "")


/*-----------------------------------------------------------------------------
 |  Expect an expression to be true
 ----------------------------------------------------------------------------*/

/// Expect a condition to be true.
/// @param condition An expression.
/// @param error (VDL_EXCEPTION). Exception ID.
/// @param format (const char *). The format of the error message.
/// @param ... Additional arguments used for snprintf.
#define pl_error_expect(condition, error, format, ...)    \
    do {                                                  \
        if ((condition) == 0)                             \
        {                                                 \
            pl_error_throw(error, format, ##__VA_ARGS__); \
        }                                                 \
    } while (0)

/*-----------------------------------------------------------------------------
 |  Error namespace
 ----------------------------------------------------------------------------*/

/// Namespace of error.
typedef struct pl_error_ns
{

    /// Save error message to the global buffer.
    /// @details The buffer will be reset if an encoding error occurred.
    /// @param error (int). The error ID.
    /// @param function_name (const char *). Function name.
    /// @param file_name (const char *). File name.
    /// @param line (int). The line number.
    /// @param format (const char *). Format for the additional error message.
    /// @param ... Additional arguments passed to snprintf.
    void (*const save_error_message)(int error,
                                     const char *function_name,
                                     const char *file_name,
                                     int line,
                                     const char *format,
                                     ...);

    /// Attempt to perform a long jump because of an exception.
    /// @details This function will not perform a long jump if the catch statement
    /// is missing
    /// @param error (int). Error ID.
    void (*const long_jump_if_catch)(int error);

    /// The default handler for no catch statement.
    /// @details The default handler prints the error message and abort the program.
    _Noreturn void (*const default_error_handler)(void);

#ifdef PL_TEST
    /// Test the namespace.
    void (*const test)(void);
#endif//PL_TEST

} pl_error_ns;

/// Get error namespace.
/// @return Namespace of error.
pl_error_ns pl_error_get_ns(void);

#endif//PL_PL_ERROR_H
