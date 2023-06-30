//
// Created by Patrick Li on 29/6/2023.
//

#include "pl.h"

/*-----------------------------------------------------------------------------
 |  Get library namespace
 ----------------------------------------------------------------------------*/

pl_ns pl_get_ns(void)
{
    const pl_ns ns = {.misc  = pl_misc_get_ns(),
                      .error = pl_error_get_ns(),
                      .bt    = pl_bt_get_ns()};
    return ns;
}