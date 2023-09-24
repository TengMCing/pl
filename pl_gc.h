//
// Created by Patrick Li on 1/7/2023.
//

#ifndef PL_PL_GC_H
#define PL_PL_GC_H

#include "pl_object.h"

/*-----------------------------------------------------------------------------
 |  Garbage collector namespace
 ----------------------------------------------------------------------------*/

/// Garbage collector namespace.
typedef struct pl_gc_ns
{
    /// New an object.
    /// @param class (int). Class of the object.
    /// @param capacity (int). Capacity of the object.
    /// @return A new object.
    /// @when_fails No side effects.
    pl_object (*const new_object)(int class, int capacity);

    /// Resize the object.
    /// @details Data may be lost if the original length of the object is
    /// greater than the original capacity.
    /// @param x (pl_object). The object.
    /// @param capacity (int). New capacity.
    /// @when_fails No side effects.
    void (*const resize_object)(pl_object x, int capacity);

    /// Reserve memory for an object.
    /// @details The function may reserve more memory than the requested
    /// amount for efficiency.
    /// @param x (pl_object). The object.
    /// @param capacity (int). New capacity.
    /// @when_fails No side effects.
    void (*const reserve_object)(pl_object x, int capacity);

    /// Declare a directly reachable object.
    /// @param x (pl_object). The object.
    /// @when_fails No external side effects.
    void (*const directly_reachable)(pl_object x);

    /// Declare a directly unreachable object.
    /// @param x (pl_object). The object.
    /// @when_fails No external side effects.
    void (*const directly_unreachable)(pl_object x);

    /// Run the garbage collector.
    /// @when_fails Usually no external side effects.\n\n
    /// Garbage collector may fail to shrink the container but successfully delete
    /// unreachable objects with error code PL_ERROR_ALLOC_FAIL.\n\n
    /// This happens when `realloc()` fails to allocate space for a new container.\n\n
    /// In this case, the garbage collector still keeps all the reachable objects correctly.\n\n
    void (*const garbage_collect)(void);

    /// Report the global table.
    /// @when_fails No external side effects.
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
