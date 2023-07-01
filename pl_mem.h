//
// Created by Patrick Li on 30/6/2023.
//

#ifndef PL_PL_MEM_H
#define PL_PL_MEM_H

#include "pl_optional.h"
#include "stdlib.h"

/*-----------------------------------------------------------------------------
 |  Memory namespace
 ----------------------------------------------------------------------------*/

/// Namespace of memory.
typedef struct pl_mem_ns
{
    /// Request a memory block.
    /// @param size (size_t). The size of the memory block in bytes.
    /// @return An optional void pointer to the memory location.
    pl_optional_void_p (*const new)(size_t size);

    /// Delete a memory block.
    /// @param address (void*). Memory address.
    void (*const delete)(void *address);

    /// Resize a memory block.
    /// @param address (void*). Memory address.
    /// @param size (size_t). The size of the memory block in bytes.
    /// @return An optional void pointer to the new memory location.
    pl_optional_void_p (*const resize)(void *address, size_t size);

} pl_mem_ns;

/// Get memory namespace.
/// @return Namespace of memory.
pl_mem_ns pl_mem_get_ns(void);

#endif//PL_PL_MEM_H
