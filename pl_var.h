//
// Created by Patrick Li on 10/9/2023.
//

#ifndef PL_PL_VAR_H
#define PL_PL_VAR_H

#include "pl_gc.h"
#include "pl_object.h"


/*-----------------------------------------------------------------------------
 |  Variable namespace
 ----------------------------------------------------------------------------*/

/// Namespace of variable.
typedef struct pl_var_ns
{
    /// Get a variable using a string.
    /// @param name (const char *). The name of the variable.
    /// @param frame (int). To search the variable in which frame.
    /// @return The associated object.
    pl_object (*const get)(const char *name, int frame);

    /// New or set a variable
    /// @param name (const char *). The name of the variable.
    /// @param content (pl_object). The associated object.
    /// @param frame (int). To store the variable in which frame.
    /// @return The original object.
    pl_object (*const set)(const char *name, pl_object content, int frame);

    /// Delete a variable.
    /// @param name (const char *). The name of the variable.
    /// @param frame (int). To search the variable in which frame.
    void (*const delete)(const char *name, int frame);

    /// Delete all variables in a frame.
    /// @param frame (int). The frame number.
    void (*const delete_frame)(int frame);

    /// Delete all variables in frames greater than a given frame number.
    /// @param frame (int). The frame number.
    void (*const delete_frames_greater)(int frame);

    /// Get the greatest frame number.
    /// @return The frame number.
    int (*const max_frame_number)(void);
} pl_var_ns;

/// Get variable namespace.
/// @return Namespace of variable.
pl_var_ns pl_var_get_ns(void);

#endif//PL_PL_VAR_H
