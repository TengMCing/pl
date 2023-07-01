//
// Created by Patrick Li on 30/6/2023.
//

#include "pl_mem.h"
#include "pl_error.h"

/*-----------------------------------------------------------------------------
 |  New
 ----------------------------------------------------------------------------*/

static pl_optional_void_p new(const size_t size)
{
    void *tmp_object = malloc(size);
    pl_error_assert_and_return(tmp_object != NULL,
                               pl_optional_void_p,
                               PL_ERROR_MALLOC_FAIL,
                               "Malloc fails!");
    return (pl_optional_void_p){.value = tmp_object};
}

/*-----------------------------------------------------------------------------
 |  Resize
 ----------------------------------------------------------------------------*/

static pl_optional_void_p resize(void *const address, const size_t size)
{
    void *tmp_object = realloc(address, size);
    if (tmp_object == NULL)
    {
        const pl_error_ns error_ns = pl_error_get_ns();
        const pl_error this_error  = error_ns.create(PL_ERROR_MALLOC_FAIL,
                                                     __func__,
                                                     __FILE_NAME__,
                                                     __LINE__,
                                                     "Realloc fails!");
        return (pl_optional_void_p){.value = address,
                                    .error = this_error};
    }
    return (pl_optional_void_p){.value = tmp_object};
}

/*-----------------------------------------------------------------------------
 |  Delete
 ----------------------------------------------------------------------------*/

static void delete(void *const address)
{
    free(address);
}

/*-----------------------------------------------------------------------------
 |  Get memory namespace
 ----------------------------------------------------------------------------*/

pl_mem_ns pl_mem_get_ns(void)
{
    static const pl_mem_ns mem_ns = {.new    = new,
                                     .delete = delete,
                                     .resize = resize};
    return mem_ns;
}
