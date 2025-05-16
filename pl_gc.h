//
// Created by Patrick Li on 1/7/2023.
//

#ifndef PL_PL_GC_H
#define PL_PL_GC_H

#include "pl_object.h"

/*-----------------------------------------------------------------------------
 |  Garbage collector strategy
 ----------------------------------------------------------------------------*/

#define PL_GC_STRATEGY_COUNTER 1
#define PL_GC_STRATEGY_TIME 2

#define PL_GC_STRATEGY_COUNTER_MAX 100
#define PL_GC_STRATEGY_TIME_MAX 10

#define PL_GC_STRATEGY PL_GC_STRATEGY_TIME

/*-----------------------------------------------------------------------------
 |  Shortcuts for garbage collection management
 ----------------------------------------------------------------------------*/

#ifdef PL_GC_SHORTCUTS

/// Begin a frame. Objects will be marked directly unreachable at the end of the frame.
/// @param ... Objects need to be used inside the frame.
#define $begin_frame(...)                                                                           \
        for (pl_object __VA_ARGS__,                                                                     \
             *pl_misc_with_i_name(__LINE__),                                                            \
             **pl_misc_with_ip_name(__LINE__) = &pl_misc_with_i_name(__LINE__);                         \
             pl_misc_with_ip_name(__LINE__);                                                            \
             pl_misc_with_ip_name(__LINE__) = 0,                                                        \
             pl_gc_get_ns().multiple_directly_unreachable(pl_misc_count_arg(__VA_ARGS__), __VA_ARGS__)) \
        {                                                                                               \
            pl_misc_init_variables(NULL, __VA_ARGS__);                                                  \
            pl_error_try

/// End a frame. Objects will be marked directly unreachable at the end of the frame.
#define $end_frame                                   \
        pl_error_catch                                   \
        {                                                \
        }                                                \
        }                                                \
        do {                                             \
            if (pl_error_get_current() != PL_ERROR_NONE) \
                pl_error_rethrow();                      \
        } while (0);

/// Set value for object. The previous content will be marked directly unreachable, and the updated content will be marked directly reachable.
/// @param x Object.
/// @param content Content.
#define $set(x, content)                            \
        do {                                            \
            if (x != NULL)                              \
                pl_gc_get_ns().directly_unreachable(x); \
            x = content;                                \
            pl_gc_get_ns().directly_reachable(x);       \
        } while (0)
#endif


/*-----------------------------------------------------------------------------
 |  Garbage collector namespace
 ----------------------------------------------------------------------------*/

/// Garbage collector namespace.
typedef struct pl_gc_ns {
    /// New an object.
    /// @param class (int). Class of the object.
    /// @param capacity (int). Capacity of the object.
    /// @return A new object.
    pl_object (*const new_object)(int class, int capacity);

    /// Resize the object.
    /// @details Data may be lost if the original length of the object is
    /// greater than the original capacity.
    /// @param x (pl_object). The object.
    /// @param capacity (int). New capacity.
    /// @return A resized object.
    void (*const resize_object)(pl_object x, int capacity);

    /// Reserve memory for an object.
    /// @details The function may reserve more memory than the requested
    /// amount for efficiency.
    /// @param x (pl_object). The object.
    /// @param capacity (int). New capacity.
    /// @return A new object.
    void (*const reserve_object)(pl_object x, int capacity);

    /// Declare a directly reachable object.
    /// @param x (pl_object). The object.
    void (*const directly_reachable)(pl_object x);

    /// Declare multiple directly reachable objects.
    /// @param length (int). Number of objects.
    /// @param ... Objects need to be recorded.
    void (*const multiple_directly_reachable)(int length, ...);

    /// Declare a directly unreachable object.
    /// @param x (pl_object). The object.
    void (*const directly_unreachable)(pl_object x);

    /// Declare multiple directly unreachable objects.
    /// @param length (int). Number of objects.
    /// @param ... Objects need to be untracked.
    void (*const multiple_directly_unreachable)(int length, ...);

    /// Run the garbage collector.
    /// @details Garbage collector may fail to shrink the container but successfully delete
    /// unreachable objects with error code PL_ERROR_ALLOC_FAIL.\n\n
    /// This happens when `realloc()` fails to allocate space for a new container.\n\n
    /// In this case, the garbage collector still keeps all the reachable objects correctly.\n\n
    void (*const garbage_collect)(void);

    /// Report the global table.
    void (*const report)(void);

    /// Kill the garbage collector and release all memory.
    void (*const kill)(void);

    /// Check the status of the garbage collector.
    /// @return 0 for stopped, 1 for working.
    int (*const check_status)(void);
} pl_gc_ns;

/// Get garbage collector namespace.
/// @return Namespace of garbage collector.
pl_gc_ns pl_gc_get_ns(void);

#endif//PL_PL_GC_H
