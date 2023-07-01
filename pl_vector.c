//
// Created by Patrick Li on 1/7/2023.
//

#include "pl_vector.h"

pl_any_vector pl_vector_char_decay(pl_char_vector v)
{
    pl_any_vector result = {.class  = v.class,
                            .length = v.length,
                            .data   = v.data};
    return result;
}

pl_any_vector pl_vector_int_decay(pl_int_vector v)
{
    pl_any_vector result = {.class  = v.class,
                            .length = v.length,
                            .data   = v.data};
    return result;
}

pl_any_vector pl_vector_double_decay(pl_double_vector v)
{
    pl_any_vector result = {.class  = v.class,
                            .length = v.length,
                            .data   = v.data};
    return result;
}

pl_char_vector pl_vector_any_to_char(pl_any_vector v)
{

}
