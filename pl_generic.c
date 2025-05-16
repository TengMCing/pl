//
// Created by Patrick Li on 24/9/2024.
//

#include "pl_generic.h"
#include "pl_error.h"
#include "pl_gc.h"

/*-----------------------------------------------------------------------------
 |  Checks
 ----------------------------------------------------------------------------*/

#define check_null_pointer(x) pl_error_expect((x) != NULL,                      \
                                              PL_ERROR_UNEXPECTED_NULL_POINTER, \
                                              "Unexpected NULL pointer `" #x "` provided!")

#define check_args_length(x) pl_error_expect((x)->length != 0,                      \
                                             PL_ERROR_INVALID_LENGTH,               \
                                             "Arguments `" #x "` has zero length!", \
                                             (x)->length)

#define check_args_class(x) pl_error_expect((x)->class == PL_CLASS_LIST,                                         \
                                            PL_ERROR_INVALID_CLASS,                                              \
                                            "Arguments `" #x "` [%s] is not of the same type as PL_CLASS_LIST!", \
                                            PL_CLASS_NAME[(x)->class])

static pl_object print_by_class(pl_object args, const int class)
{
    check_null_pointer(args);
    check_args_length(args);
    check_args_class(args);

    pl_object *arg_array = args->data;
    switch (class)
    {
        default:
        {
            pl_error_expect(args->length == 1,
                            PL_ERROR_INVALID_NUMBER_OF_ARGUMENTS,
                            "Calling method `pl.object.print` with [%d] arguments instead of [%d]!",
                            args->length,
                            1);
            pl_object_ns object_ns = pl_object_get_ns();
            object_ns.print(arg_array[0]);
        }
    }
    return NULL;
}

static pl_object print(pl_object args)
{
    check_null_pointer(args);
    check_args_length(args);
    check_args_class(args);
    const int class = ((pl_object *) args->data)[0]->class;

    return print_by_class(args, class);
}

pl_generic_ns pl_generic_get_ns(void)
{
    static const pl_generic_ns generic_ns = {.print          = print,
                                             .print_by_class = print_by_class};
    return generic_ns;
}
