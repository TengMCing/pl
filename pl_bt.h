//
// Created by Patrick Li on 30/6/2023.
//

#ifndef PL_PL_BT_H
#define PL_PL_BT_H

#include "pl_error.h"
#include "pl_optional.h"

/*-----------------------------------------------------------------------------
 |  Frame recording
 ----------------------------------------------------------------------------*/

/// A frame or a call.
/// @param file_name (const char *const). File name of the current call.
/// @param function_name (const char* const). Function name of the current call.
/// @param line (const int). Line number of the current call.
typedef struct pl_bt_frame
{
    const char *const file_name;
    const char *const function_name;
    const int line;
} pl_bt_frame;

/// An optional frame.
/// @param value (const pl_bt_frame). A frame.
/// @param error (const pl_error). An error report.
typedef struct pl_bt_optional_frame
{
    const pl_bt_frame value;
    const pl_error error;
} pl_bt_optional_frame;

/// Record the current call.
/// @return A frame.
#define pl_bt_make_frame(func_name)   \
    (pl_bt_frame)                     \
    {                                 \
        .file_name     = __FILE__,    \
        .function_name = (func_name), \
        .line          = __LINE__     \
    }

#define PL_BT_MAX_ALLOWED_FRAME_NUM 256

/*-----------------------------------------------------------------------------
 |  Caller
 ----------------------------------------------------------------------------*/

#ifndef PL_DISABLE_BT
    /// Call a void function.
    /// @param function The function name.
    /// @param ... Additional arguments passed to the function.
    /// @return A pl_optional_void result to indicate if the push to the backtrace stack fails.
    #define pl_bt_call_void(function, ...)                                           \
        ({                                                                           \
            const pl_bt_ns bt_ns          = pl_bt_get_ns();                          \
            pl_optional_void _push_result = bt_ns.push(pl_bt_make_frame(#function)); \
            if (!_push_result.error.error_code)                                      \
                function(__VA_ARGS__);                                               \
            bt_ns.pop();                                                             \
            _push_result;                                                            \
        })

    /// Call a function with an optional return type.
    /// @param return_type The function return type. This has to be an optional type.
    /// @param function The function name.
    /// @param ... Additional arguments passed to the function.
    /// @return The return value of the function or a failed result if any error occurred.
    #define pl_bt_call(return_type, function, ...)                                                                                               \
        ({                                                                                                                                       \
            const pl_bt_ns bt_ns          = pl_bt_get_ns();                                                                                      \
            pl_optional_void _push_result = bt_ns.push(pl_bt_make_frame(#function));                                                             \
            return_type _return_value     = !_push_result.error.error_code ? function(__VA_ARGS__) : (return_type){.error = _push_result.error}; \
            bt_ns.pop();                                                                                                                         \
            _return_value;                                                                                                                       \
        })
#endif

#ifdef PL_DISABLE_BT
    #define pl_bt_call_void(function, ...) ({function(__VA_ARGS__); (pl_optional_void){0}; })
    #define pl_bt_call(return_type, function, ...) function(__VA_ARGS__)
#endif

/*-----------------------------------------------------------------------------
 |  Backtrace namespace
 ----------------------------------------------------------------------------*/

/// Namespace of backtrace.
typedef struct pl_bt_ns
{
    /// Pop a frame from the backtrace stack.
    void (*const pop)(void);

    /// Push a frame to the backtrace stack.
    /// @param frame (pl_bt_frame). A frame to be pushed to the backtrace stack.
    /// @return An optional void.
    pl_optional_void (*const push)(pl_bt_frame frame);

    /// Get the depth of the backtrace stack.
    int (*const get_depth)(void);

    /// Get a frame from the backtrace stack.
    /// @param depth (int). Depth of the frame.
    /// @return An optional frame.
    pl_bt_optional_frame (*const get_frame)(int depth);

    /// Print the backtrace.
    void (*const print)(void);

    /// Backup the current state of the backtrace.
    void (*const backup)(void);

    /// Print the backup backtrace.
    void (*const print_backup)(void);
} pl_bt_ns;

/// Get backtrace namespace.
/// @return Namespace of backtrace.
pl_bt_ns pl_bt_get_ns(void);

#endif//PL_PL_BT_H
