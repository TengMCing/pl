//
// Created by Patrick Li on 29/6/2023.
//

#ifndef PL_PL_H
#define PL_PL_H

#include "pl_misc.h"
#include "pl_error.h"
#include "pl_class.h"
#include "pl_gc.h"
#include "pl_object.h"
#include "pl_var.h"

/*-----------------------------------------------------------------------------
 |  Library namespace
 ----------------------------------------------------------------------------*/

/// Namespace of the library.
typedef struct pl_ns {

    /// Namespace of misc.
    const pl_misc_ns misc;

    /// Namespace of error.
    const pl_error_ns error;

    /// Namespace of class.
    const pl_class_ns class;

    /// Namespace of garbage collector.
    const pl_gc_ns gc;

    /// Namespace of object.
    const pl_object_ns object;

    /// Namespace of variable.
    const pl_var_ns var;
} pl_ns;

/// Get the namespace of the library.
/// @return Namespace of the library.
pl_ns pl_get_ns(void);

#endif//PL_PL_H
