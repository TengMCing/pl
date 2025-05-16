//
// Created by Patrick Li on 24/9/2024.
//

#ifndef PL_PL_GENERIC_H
#define PL_PL_GENERIC_H

#include "pl_object.h"

enum
{
    PL_GENERIC_PRINT = 0
};


/*-----------------------------------------------------------------------------
 |  Generic namespace
 ----------------------------------------------------------------------------*/

/// Namespace of the library.
typedef struct pl_generic_ns
{
    pl_object (*const register_method)(int generic, int class, void *method, pl_object num_args);
    pl_object (*const call_method)(int generic, int class, pl_object args);
    pl_object (*const print)(pl_object args);
    pl_object (*const print_by_class)(pl_object args, int class);
} pl_generic_ns;

/// Get the namespace of the library.
/// @return Namespace of the library.
pl_generic_ns pl_generic_get_ns(void);

#endif//PL_PL_GENERIC_H
