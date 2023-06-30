//
// Created by Patrick Li on 29/6/2023.
//

#ifndef PL_PL_H
#define PL_PL_H

#include "pl_misc.h"
#include "pl_error.h"
#include "pl_optional.h"
#include "pl_bt.h"

/*-----------------------------------------------------------------------------
 |  Library namespace
 ----------------------------------------------------------------------------*/

/// Namespace of the library.
typedef struct pl_ns {

    /// Namespace of misc.
    const pl_misc_ns misc;

    /// Namespace of error.
    const pl_error_ns error;

    /// Namespace of backtrace.
    const pl_bt_ns bt;
} pl_ns;

/// Get the namespace of the library.
/// @return Namespace of the library.
pl_ns pl_get_ns(void);

#endif//PL_PL_H
