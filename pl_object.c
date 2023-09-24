//
// Created by Patrick Li on 1/7/2023.
//

#include "pl_object.h"
#include "pl_class.h"
#include "pl_gc.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

static pl_object copy(pl_object x);

/*-----------------------------------------------------------------------------
 |  Checks
 ----------------------------------------------------------------------------*/

#define check_null_pointer(x) pl_error_expect((x) != NULL,                   \
                                              PL_ERROR_INVALID_NULL_POINTER, \
                                              "Can't access NULL pointer `" #x "` !")

#define check_object_type(object, this_type)                              \
    do {                                                                  \
        const pl_class_ns class_ns = pl_class_get_ns();                   \
        const int object_type      = class_ns.type((object)->class);      \
        pl_error_expect(object_type == this_type,                         \
                        PL_ERROR_INVALID_CLASS,                           \
                        "Object `" #object "` [%d] is not of type [%d]!", \
                        object_type,                                      \
                        this_type);                                       \
    } while (0)

#define check_same_type(x, y)                                                                    \
    do {                                                                                         \
        const pl_class_ns class_ns = pl_class_get_ns();                                          \
        const int x_type           = class_ns.type((x)->class);                                  \
        const int y_type           = class_ns.type((y)->class);                                  \
        pl_error_expect(x_type == y_type,                                                        \
                        PL_ERROR_INVALID_CLASS,                                                  \
                        "Object `" #x "` [%d] is not of the same type as object `" #y "` [%d]!", \
                        x_type,                                                                  \
                        y_type);                                                                 \
    } while (0)

#define check_object_length(object, len) pl_error_expect((object)->length == len,                                 \
                                                         PL_ERROR_INVALID_LENGTH,                                 \
                                                         "Object `" #object "` has length [%d] instead of [%d]!", \
                                                         (object)->length,                                        \
                                                         len)

#define check_index_out_of_bound(object, index) pl_error_expect((index >= 0) && (index <= x->length - 1), \
                                                                PL_ERROR_INDEX_OUT_OF_BOUND,              \
                                                                "Index [%d] out of bound [0, %d)!",       \
                                                                index,                                    \
                                                                (object)->length)

#define check_missing_value(x) pl_error_expect(!pl_is_na(x),        \
                                               PL_ERROR_INVALID_NA, \
                                               "Can't handle missing value `" #x "` !")

/*-----------------------------------------------------------------------------
 |  Primitive new
 ----------------------------------------------------------------------------*/

/// New an object.
/// @param class (int). Class of the object.
/// @param capacity (int). Capacity of the object.
/// @return A new empty object.
/// @when_fails No side effects.
static pl_object primitive_new(const int class, const int capacity)
{
    check_missing_value(class);
    check_missing_value(capacity);
    const pl_gc_ns gc_ns = pl_gc_get_ns();
    return gc_ns.new_object(class, capacity);
}

/*-----------------------------------------------------------------------------
 |  Primitive new from array
 ----------------------------------------------------------------------------*/

/// New an object using an array.
/// @details If `length` is zero, an empty object will be returned. The `array`
/// will not be used.
/// @param class (int). Class of the object.
/// @param length (int). Length of the object.
/// @param array (const void *). The source array.
/// @return A new object.
/// @when_fails No side effects.
static pl_object primitive_new_from_array(const int class, const int length, const void *const array)
{
    check_missing_value(class);
    check_missing_value(length);

    if (length == 0)
        return primitive_new(class, 1);
    pl_error_expect(length > 0,
                    PL_ERROR_INVALID_LENGTH,
                    "Invalid length [%d]!",
                    length);
    check_null_pointer(array);

    pl_object object = primitive_new(class, length);
    object->length   = length;
    memmove(object->data,
            array,
            (size_t) length * PL_CLASS_ELEMENT_SIZE[class]);
    return object;
}

/*-----------------------------------------------------------------------------
 |  Primitive new from variadic
 ----------------------------------------------------------------------------*/

/// New an object using variadic arguments.
/// @details If `length` is zero, an empty object will be returned.
/// @param class (int). Class of the object.
/// @param length (int). Number of items will be provided.
/// @param ... Items need to be stored by the object.
/// @return A new object.
/// @when_fails No side effects.
static pl_object primitive_new_from_variadic(const int class, const int length, ...)
{
    check_missing_value(class);
    check_missing_value(length);

    if (length == 0)
        return primitive_new(class, 1);
    pl_error_expect(length > 0,
                    PL_ERROR_INVALID_LENGTH,
                    "Invalid length [%d]!",
                    length);

    pl_object object = primitive_new(class, length);
    object->length   = length;

    // Get the underlying type.
    const pl_class_ns class_ns = pl_class_get_ns();
    const int type             = class_ns.type(class);

    va_list ap;
    va_start(ap, length);
    switch (type)
    {
        case PL_CLASS_CHAR:
        {
            char *const data_array              = object->data;
            pl_misc_for_i(length) data_array[i] = (char) va_arg(ap, int);
            break;
        }
        case PL_CLASS_INT:
        {
            int *const data_array               = object->data;
            pl_misc_for_i(length) data_array[i] = va_arg(ap, int);
            break;
        }
        case PL_CLASS_LONG:
        {
            long *const data_array              = object->data;
            pl_misc_for_i(length) data_array[i] = va_arg(ap, long);
            break;
        }
        case PL_CLASS_DOUBLE:
        {
            double *const data_array            = object->data;
            pl_misc_for_i(length) data_array[i] = va_arg(ap, double);
            break;
        }
        default:
        {
            pl_object *const data_array         = object->data;
            pl_misc_for_i(length) data_array[i] = va_arg(ap, pl_object);
            break;
        }
    }
    va_end(ap);

    return object;
}


/*-----------------------------------------------------------------------------
 |  Primitive to array
 ----------------------------------------------------------------------------*/

/// Copy an object to an array.
/// @details This is a shallow copy.
/// @param x (pl_object). The object.
/// @param array (void *). Destination array.
/// @when_fails No side effects.
static void primitive_to_array(pl_object x, void *const array)
{
    check_null_pointer(x);
    check_null_pointer(array);
    memmove(array, x->data, (size_t) x->length * PL_CLASS_ELEMENT_SIZE[x->class]);
}

/*-----------------------------------------------------------------------------
 |  Primitive reserve
 ----------------------------------------------------------------------------*/

/// Reserve memory for an object.
/// @details The function may reserve more memory than the requested
/// amount for efficiency.
/// @param x (pl_object). The object.
/// @param capacity (int). New capacity.
/// @when_fails No side effects.
static void primitive_reserve(pl_object x, const int capacity)
{
    check_null_pointer(x);
    check_missing_value(capacity);
    const pl_gc_ns gc_ns = pl_gc_get_ns();
    gc_ns.reserve_object(x, capacity);
}

/*-----------------------------------------------------------------------------
 |  Primitive shrink
 ----------------------------------------------------------------------------*/

/// Shrink an object to a desired capacity.
/// @details Data may lost due to the shrink.
/// @param x (pl_object). The object.
/// @param capacity (int). The desired capacity.
/// @when_fails No side effects.
static void primitive_shrink(pl_object x, const int capacity)
{
    check_null_pointer(x);
    check_missing_value(capacity);
    const pl_gc_ns gc_ns = pl_gc_get_ns();
    gc_ns.resize_object(x, capacity);
}

/*-----------------------------------------------------------------------------
 |  Primitive set
 ----------------------------------------------------------------------------*/

#define primitive_set_type_template(this_type, this_class)                                    \
    static void primitive_set_##this_type(pl_object x, const int index, const this_type item) \
    {                                                                                         \
        check_null_pointer(x);                                                                \
        if (pl_is_na(index))                                                                  \
            return;                                                                           \
        check_index_out_of_bound(x, index);                                                   \
        check_object_type(x, this_class);                                                     \
                                                                                              \
        ((this_type *) x->data)[index] = item;                                                \
    }

/// Set value for an object.
/// @details If `index` is NA, the value will not be set.
/// @param x (pl_object). An object of type PL_CLASS_CHAR.
/// @param index (int). The index.
/// @param item (char). The new value.
/// @when_fails No side effects.
primitive_set_type_template(char, PL_CLASS_CHAR);

/// Set value for an object.
/// @details If `index` is NA, the value will not be set.
/// @param x (pl_object). An object of type PL_CLASS_INT.
/// @param index (int). The index.
/// @param item (int). The new value.
/// @when_fails No side effects.
primitive_set_type_template(int, PL_CLASS_INT);

/// Set value for an object.
/// @details If `index` is NA, the value will not be set.
/// @param x (pl_object). An object of type PL_CLASS_LONG.
/// @param index (int). The index.
/// @param item (long). The new value.
/// @when_fails No side effects.
primitive_set_type_template(long, PL_CLASS_LONG);

/// Set value for an object.
/// @details If `index` is NA, the value will not be set.
/// @param x (pl_object). An object of type PL_CLASS_DOUBLE.
/// @param index (int). The index.
/// @param item (double). The new value.
/// @when_fails No side effects.
primitive_set_type_template(double, PL_CLASS_DOUBLE);

/// Set value for an object.
/// @details If `index` is NA, the value will not be set.
/// @param x (pl_object). An object of type PL_CLASS_LIST.
/// @param index (int). The index.
/// @param item (pl_object). The new value.
/// @when_fails No side effects.
primitive_set_type_template(pl_object, PL_CLASS_LIST);


/*-----------------------------------------------------------------------------
 |  Primitive set by indices
 ----------------------------------------------------------------------------*/

/// Set values for an object by indices.
/// @details If an index of `indices` is NA, the corresponding value will not be set.
/// If `length` is zero, not items will be set and `array` will not be used.
/// @param x (pl_object). The object.
/// @param length (int). Length of the array.
/// @param indices (const int *). The indices.
/// @param array (const void *). The array.
/// @when_fails No side effects.
static void primitive_set_by_indices(pl_object x, const int length, const int *const indices, const void *const array)
{
    check_null_pointer(x);

    check_missing_value(length);
    if (length == 0)
        return;
    pl_error_expect(length > 0,
                    PL_ERROR_INVALID_LENGTH,
                    "Invalid length [%d]!",
                    length);

    check_null_pointer(indices);
    pl_misc_for_i(length)
    {
        if (!pl_is_na(indices[i]))
            check_index_out_of_bound(x, indices[i]);
    }

    check_null_pointer(array);

    const pl_class_ns class_ns = pl_class_get_ns();
    const int type             = class_ns.type(x->class);
    switch (type)
    {
        case PL_CLASS_CHAR:
        {
            char *const data_array                                                  = x->data;
            const char *const src_array                                             = array;
            pl_misc_for_i(length) if (!pl_is_na(indices[i])) data_array[indices[i]] = src_array[i];
            break;
        }
        case PL_CLASS_INT:
        {
            int *const data_array                                                   = x->data;
            const int *const src_array                                              = array;
            pl_misc_for_i(length) if (!pl_is_na(indices[i])) data_array[indices[i]] = src_array[i];
            break;
        }
        case PL_CLASS_LONG:
        {
            long *const data_array                                                  = x->data;
            const long *const src_array                                             = array;
            pl_misc_for_i(length) if (!pl_is_na(indices[i])) data_array[indices[i]] = src_array[i];
            break;
        }
        case PL_CLASS_DOUBLE:
        {
            double *const data_array                                                = x->data;
            const double *const src_array                                           = array;
            pl_misc_for_i(length) if (!pl_is_na(indices[i])) data_array[indices[i]] = src_array[i];
            break;
        }
        default:
        {
            pl_object *const data_array                                             = x->data;
            const pl_object *const src_array                                        = array;
            pl_misc_for_i(length) if (!pl_is_na(indices[i])) data_array[indices[i]] = src_array[i];
            break;
        }
    }
}

/*-----------------------------------------------------------------------------
 |  Primitive set range
 ----------------------------------------------------------------------------*/

/// Set a range of items for an object.
/// @param x (pl_object). The object.
/// @param start (int). The first index.
/// @param end (int). The last index.
/// @param array (const void *). The source array.
/// @when_fails No side effects.
static void primitive_set_range(pl_object x, const int start, const int end, const void *const array)
{
    check_null_pointer(x);

    check_index_out_of_bound(x, start);
    check_index_out_of_bound(x, end);

    check_null_pointer(array);

    memmove(array,
            (char *) x->data + (size_t) start * PL_CLASS_ELEMENT_SIZE[x->class],
            end - start + 1);
}

/*-----------------------------------------------------------------------------
 |  Primitive set by Booleans
 ----------------------------------------------------------------------------*/

/// Set values for an object by a Boolean array.
/// @param x (pl_object). The object.
/// @param bool_array (const int *). The Boolean array.
/// @param array (const void *). The source array.
/// @when_fails No side effects.
static void primitive_set_by_bool(pl_object x, const int *const bool_array, const void *const array)
{
    check_null_pointer(x);
    check_null_pointer(bool_array);
    check_null_pointer(array);

    pl_misc_for_i(x->length) check_missing_value(bool_array[i]);

    const pl_class_ns class_ns = pl_class_get_ns();
    const int type             = class_ns.type(x->class);

    int count = 0;
    switch (type)
    {
        case PL_CLASS_CHAR:
        {
            char *const data_array      = x->data;
            const char *const src_array = array;
            pl_misc_for_i(x->length)
            {
                if (bool_array[i] == 1)
                {
                    data_array[i] = src_array[count];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_INT:
        {
            int *const data_array      = x->data;
            const int *const src_array = array;
            pl_misc_for_i(x->length)
            {
                if (bool_array[i] == 1)
                {
                    data_array[i] = src_array[count];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_LONG:
        {
            long *const data_array      = x->data;
            const long *const src_array = array;
            pl_misc_for_i(x->length)
            {
                if (bool_array[i] == 1)
                {
                    data_array[i] = src_array[count];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_DOUBLE:
        {
            double *const data_array      = x->data;
            const double *const src_array = array;
            pl_misc_for_i(x->length)
            {
                if (bool_array[i] == 1)
                {
                    data_array[i] = src_array[count];
                    count++;
                }
            }
            break;
        }
        default:
        {
            pl_object *const data_array      = x->data;
            const pl_object *const src_array = array;
            pl_misc_for_i(x->length)
            {
                if (bool_array[i] == 1)
                {
                    data_array[i] = src_array[count];
                    count++;
                }
            }
            break;
        }
    }
}

/*-----------------------------------------------------------------------------
 |  Primitive extract
 ----------------------------------------------------------------------------*/

#define primitive_extract_type_template(this_type, this_class, this_na)          \
    static this_type primitive_extract_##this_type(pl_object x, const int index) \
    {                                                                            \
        check_null_pointer(x);                                                   \
        check_object_type(x, this_class);                                        \
                                                                                 \
        if (pl_is_na(index))                                                     \
            return this_na;                                                      \
        check_index_out_of_bound(x, index);                                      \
                                                                                 \
        return ((this_type *) x->data)[index];                                   \
    }

/// Extract value of an object.
/// @details If `index` is NA, returns NA.
/// @param x (pl_object). An object of type PL_CLASS_CHAR.
/// @param index (int). The index.
/// @return A char.
/// @when_fails No side effects.
primitive_extract_type_template(char, PL_CLASS_CHAR, PL_CHAR_NA);

/// Extract value of an object.
/// @details If `index` is NA, returns NA.
/// @param x (pl_object). An object of type PL_CLASS_INT.
/// @param index (int). The index.
/// @return A int.
/// @when_fails No side effects.
primitive_extract_type_template(int, PL_CLASS_INT, PL_INT_NA);

/// Extract value of an object.
/// @details If `index` is NA, returns NA.
/// @param x (pl_object). An object of type PL_CLASS_LONG.
/// @param index (int). The index.
/// @return A long.
/// @when_fails No side effects.
primitive_extract_type_template(long, PL_CLASS_LONG, PL_LONG_NA);

/// Extract value of an object.
/// @details If `index` is NA, returns NA.
/// @param x (pl_object). An object of type PL_CLASS_DOUBLE.
/// @param index (int). The index.
/// @return A double.
/// @when_fails No side effects.
primitive_extract_type_template(double, PL_CLASS_DOUBLE, PL_DOUBLE_NA);

/// Extract value of an object.
/// @details If `index` is NA, returns NA.
/// @details The attribute of the outer list will be dropped.
/// @param x (pl_object). An object of type PL_CLASS_LIST.
/// @param index (int). The index.
/// @return A pl_object.
/// @when_fails No side effects.
primitive_extract_type_template(pl_object, PL_CLASS_LIST, PL_LIST_NA);

/*-----------------------------------------------------------------------------
 |  Primitive extend
 ----------------------------------------------------------------------------*/

#define primitive_extend_type_template(this_type, this_class)                   \
    static void primitive_extend_##this_type(pl_object x, const this_type item) \
    {                                                                           \
        check_null_pointer(x);                                                  \
        check_object_type(x, this_class);                                       \
                                                                                \
        primitive_reserve(x, x->length + 1);                                    \
        ((this_type *) x->data)[x->length] = item;                              \
        x->length += 1;                                                         \
    }

/// Extend an object by a value.
/// @param x (pl_object). An object of type PL_CLASS_CHAR.
/// @param item (char). The item.
/// @when_fails No side effects.
primitive_extend_type_template(char, PL_CLASS_CHAR);

/// Extend an object by a value.
/// @param x (pl_object). An object of type PL_CLASS_INT.
/// @param item (int). The item.
/// @when_fails No side effects.
primitive_extend_type_template(int, PL_CLASS_INT);

/// Extend an object by a value.
/// @param x (pl_object). An object of type PL_CLASS_LONG.
/// @param item (long). The item.
/// @when_fails No side effects.
primitive_extend_type_template(double, PL_CLASS_DOUBLE);

/// Extend an object by a value.
/// @param x (pl_object). An object of type PL_CLASS_DOUBLE.
/// @param item (double). The item.
/// @when_fails No side effects.
primitive_extend_type_template(long, PL_CLASS_LONG);

/// Extend an object by a value.
/// @param x (pl_object). An object of type PL_CLASS_DOUBLE.
/// @param item (pl_object). The item.
/// @when_fails No side effects.
primitive_extend_type_template(pl_object, PL_CLASS_LIST);

/*-----------------------------------------------------------------------------
 |  Primitive subset
 ----------------------------------------------------------------------------*/

/// Construct a new object as a subset of the original object.
/// @details The attribute will be dropped. If an index of `indices` is NA,
/// the corresponding item will be NA.
/// If `length` is zero, an empty object will be returned.
/// @param x (pl_object). The object.
/// @param length (int). The length of the new object.
/// @param indices (int *). Indices.
/// @return A new object.
/// @when_fails No side effects.
static pl_object primitive_subset(pl_object x, const int length, const int *const indices)
{
    check_null_pointer(x);

    check_missing_value(length);
    if (length == 0)
        return primitive_new(x->class, 1);
    pl_error_expect(length > 0,
                    PL_ERROR_INVALID_LENGTH,
                    "Invalid length [%d]!",
                    length);

    check_null_pointer(indices);

    // Check index out of bound.
    pl_misc_for_i(length)
    {
        if (!pl_is_na(indices[i]))
            check_index_out_of_bound(x, indices[i]);
    }

    // Request a new object.
    pl_object object = primitive_new(x->class, length);
    object->length   = length;

    // Get the underlying type.
    const pl_class_ns class_ns = pl_class_get_ns();
    const int type             = class_ns.type(x->class);

    switch (type)
    {
        case PL_CLASS_CHAR:
        {
            char *const src_array              = x->data;
            char *const dst_array              = object->data;
            pl_misc_for_i(length) dst_array[i] = pl_is_na(indices[i]) ? PL_CHAR_NA : src_array[indices[i]];
            break;
        }
        case PL_CLASS_INT:
        {
            int *const src_array               = x->data;
            int *const dst_array               = object->data;
            pl_misc_for_i(length) dst_array[i] = pl_is_na(indices[i]) ? PL_INT_NA : src_array[indices[i]];
            break;
        }
        case PL_CLASS_LONG:
        {
            long *const src_array              = x->data;
            long *const dst_array              = object->data;
            pl_misc_for_i(length) dst_array[i] = pl_is_na(indices[i]) ? PL_LONG_NA : src_array[indices[i]];
            break;
        }
        case PL_CLASS_DOUBLE:
        {
            double *const src_array            = x->data;
            double *const dst_array            = object->data;
            pl_misc_for_i(length) dst_array[i] = pl_is_na(indices[i]) ? PL_DOUBLE_NA : src_array[indices[i]];
            break;
        }
        default:
        {
            pl_object *const src_array         = x->data;
            pl_object *const dst_array         = object->data;
            pl_misc_for_i(length) dst_array[i] = pl_is_na(indices[i]) ? PL_LIST_NA : src_array[indices[i]];
            break;
        }
    }

    return object;
}

/*-----------------------------------------------------------------------------
 |  Primitive subset exclude
 ----------------------------------------------------------------------------*/

/// Construct a new object as a subset of the original object by excluding some indices.
/// @details The attribute will be dropped. NAs in `indices` will be ignored.
/// If `length` is zero, a shallow copy of the object will be returned.
/// @param x (pl_object). The object.
/// @param length (int). The length of indices.
/// @param indices (int *). Indices that needs to be excluded.
/// @return A new object.
/// @when_fails No side effects.
static pl_object primitive_subset_exclude(pl_object x, const int length, const int *const indices)
{
    check_null_pointer(x);

    check_missing_value(length);
    if (length == 0)
        return copy(x);
    pl_error_expect(length > 0,
                    PL_ERROR_INVALID_LENGTH,
                    "Invalid length [%d]!",
                    length);

    check_null_pointer(indices);
    // Check index out of bound.
    pl_misc_for_i(length)
    {
        if (!pl_is_na(indices[i]))
            check_index_out_of_bound(x, indices[i]);
    }

    pl_object index_flags                   = primitive_new(PL_CLASS_INT, x->length);
    int *const index_array                  = index_flags->data;
    pl_misc_for_i(x->length) index_array[i] = 1;
    index_flags->length                     = x->length;

    pl_misc_for_i(length) if (!pl_is_na(indices[i])) index_array[indices[i]] = 0;

    int count = 0;
    pl_misc_for_i(x->length)
    {
        if (index_array[i] == 1)
        {
            index_array[count] = i;
            count++;
        }
    }

    return primitive_subset(x, count, index_array);
}

/*-----------------------------------------------------------------------------
 |  Primitive subset by Booleans
 ----------------------------------------------------------------------------*/

/// Construct a new object as a subset of the original object by a Boolean array.
/// @param x (pl_object). The object.
/// @param bool_array (const int *). An array of Booleans.
/// @return A new object.
/// @when_fails No side effects.
static pl_object primitive_subset_by_bool(pl_object x, const int *const bool_array)
{

    check_null_pointer(x);
    check_null_pointer(bool_array);

    // Check Nas.
    pl_misc_for_i(x->length) check_missing_value(bool_array[i]);

    // Get the underlying type.
    const pl_class_ns class_ns = pl_class_get_ns();
    const int type             = class_ns.type(x->class);

    int count = 0;
    switch (type)
    {
        case PL_CLASS_CHAR:
        {
            char *const data_array = x->data;
            pl_misc_for_i(x->length)
            {
                if (bool_array[i] == 1)
                {
                    data_array[count] = data_array[i];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_INT:
        {
            int *const data_array = x->data;
            pl_misc_for_i(x->length)
            {
                if (bool_array[i] == 1)
                {
                    data_array[count] = data_array[i];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_LONG:
        {
            long *const data_array = x->data;
            pl_misc_for_i(x->length)
            {
                if (bool_array[i] == 1)
                {
                    data_array[count] = data_array[i];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_DOUBLE:
        {
            double *const data_array = x->data;
            pl_misc_for_i(x->length)
            {
                if (bool_array[i] == 1)
                {
                    data_array[count] = data_array[i];
                    count++;
                }
            }
            break;
        }
        default:
        {
            pl_object *const data_array = x->data;
            pl_misc_for_i(x->length)
            {
                if (bool_array[i] == 1)
                {
                    data_array[count] = data_array[i];
                    count++;
                }
            }
        }
    }

    x->length = count;
}

/*-----------------------------------------------------------------------------
 |  Primitive remove
 ----------------------------------------------------------------------------*/

/// Remove items from the object.
/// @param x (pl_object). The object.
/// @param start (int). The first index.
/// @param end (int). The last index.
/// @when_fails No side effects.
static void primitive_remove(pl_object x, const int start, const int end)
{
    check_null_pointer(x);
    check_missing_value(start);
    check_missing_value(end);
    check_index_out_of_bound(x, start);
    check_index_out_of_bound(x, end);

    if (end + 1 == x->length)
    {
        x->length = start;
        return;
    }

    memmove((char *) x->data + (size_t) start * PL_CLASS_ELEMENT_SIZE[x->class],
            (char *) x->data + (size_t) (end + 1) * PL_CLASS_ELEMENT_SIZE[x->class],
            x->length - end - 1);
    x->length = x->length - (end - start + 1);
}

/*-----------------------------------------------------------------------------
 |  Primitive remove by indices
 ----------------------------------------------------------------------------*/

/// Remove items from the object by indices.
/// @details NAs in `indices` will be ignored. Duplicate indices will
/// be ignored.
/// @param x (pl_object). The object.
/// @param length (int). Length of the array.
/// @param indices (const int *). The indices.
/// @param array (const void *). The array.
/// @when_fails No side effects.
static void primitive_remove_by_indices(pl_object x, const int length, const int *const indices)
{
    check_null_pointer(x);

    check_missing_value(length);
    if (length == 0)
        return;
    pl_error_expect(length > 0,
                    PL_ERROR_INVALID_LENGTH,
                    "Invalid length [%d]!",
                    length);

    check_null_pointer(indices);
    // Check index out of bound.
    pl_misc_for_i(length)
    {
        if (!pl_is_na(indices[i]))
            check_index_out_of_bound(x, indices[i]);
    }

    pl_object index_flags                   = primitive_new(PL_CLASS_INT, x->length);
    int *const index_array                  = index_flags->data;
    pl_misc_for_i(x->length) index_array[i] = 1;
    index_flags->length                     = x->length;

    pl_misc_for_i(length) if (!pl_is_na(indices[i])) index_array[indices[i]] = 0;

    primitive_subset_by_bool(x, index_array);
}

/*-----------------------------------------------------------------------------
 |  New
 ----------------------------------------------------------------------------*/

/// New an object.
/// @param class (pl_object). Class of the object.
/// @param capacity (pl_object). Capacity of the object.
/// @return A new empty object.
static pl_object new(pl_object class, pl_object capacity)
{
    check_null_pointer(class);
    check_null_pointer(capacity);
    check_object_type(class, PL_CLASS_INT);
    check_object_length(class, 1);
    check_object_type(capacity, PL_CLASS_INT);
    check_object_length(capacity, 1);

    pl_object object = primitive_new(((int *) class->data)[0],
                                     ((int *) capacity->data)[0]);

    return object;
}

/*-----------------------------------------------------------------------------
 |  Reserve
 ----------------------------------------------------------------------------*/

/// Reserve memory for an object.
/// The function may reserve more memory than requested amount for efficiency.
/// @param x (pl_object). The object.
/// @param capacity (pl_object). New capacity.
static void reserve(pl_object x, pl_object capacity)
{
    check_null_pointer(x);
    check_null_pointer(capacity);
    check_object_type(capacity, PL_CLASS_INT);
    check_object_length(capacity, 1);

    primitive_reserve(x, ((int *) capacity->data)[0]);
}


/*-----------------------------------------------------------------------------
 |  Set
 ----------------------------------------------------------------------------*/

/// Set one or more values of an object.
/// @param x (pl_object). The object.
/// @param indices (pl_object). Indices.
/// @param items (pl_object). Items.
static void set(pl_object x, pl_object indices, pl_object items)
{
    check_null_pointer(x);
    check_null_pointer(indices);
    check_null_pointer(items);
    check_object_type(indices, PL_CLASS_INT);
    check_same_type(x, items);

    primitive_set_by_indices(x, indices->length, indices->data, items->data);
}

/*-----------------------------------------------------------------------------
 |  Append
 ----------------------------------------------------------------------------*/

/// Append an item to the end of a list.
/// @param x (pl_object). The object.
/// @param item (pl_object). The item.
static void append(pl_object x, pl_object item)
{
    check_null_pointer(x);
    check_null_pointer(item);
    check_object_type(x, PL_CLASS_LIST);

    primitive_reserve(x, x->length + 1);
    ((pl_object *) x->data)[x->length] = item;
    x->length += 1;
}

/*-----------------------------------------------------------------------------
 |  Extract
 ----------------------------------------------------------------------------*/

/// Extract a value of an object.
/// @param x (pl_object). The object.
/// @param index (pl_object). The index.
/// @return If x is of type PL_CLASS_LIST,
/// this function will return the value directly.
/// Otherwise, the function will act the same as `subset`
/// and drop the attribute.
static pl_object extract(pl_object x, pl_object index)
{
    check_null_pointer(x);
    check_null_pointer(index);
    check_object_type(index, PL_CLASS_INT);
    check_object_length(index, 1);

    const int int_index = ((int *) index->data)[0];
    check_missing_value(int_index);
    check_index_out_of_bound(x, int_index);

    const pl_class_ns class_ns = pl_class_get_ns();
    if (class_ns.type(x->class) == PL_CLASS_LIST)
        return ((pl_object *) x->data)[int_index];
    else
        return primitive_subset(x, 1, (int[1]){int_index});
}

/*-----------------------------------------------------------------------------
 |  Extend
 ----------------------------------------------------------------------------*/

/// Extend an object by another object.
/// @param x (pl_object). The object.
/// @param y (pl_object). Another object of the same type.
static void extend(pl_object x, pl_object y)
{
    check_null_pointer(x);
    check_null_pointer(y);
    check_same_type(x, y);

    long result_length = x->length + y->length;
    pl_error_expect(result_length > 0 && result_length < PL_OBJECT_MAX_CAPACITY,
                    PL_ERROR_INVALID_CAPACITY,
                    "Invalid capacity [%ld]!",
                    result_length);

    primitive_reserve(x, (int) result_length);
    memmove((char *) x->data + (size_t) x->length * PL_CLASS_ELEMENT_SIZE[x->class],
            y->data,
            (size_t) y->length * PL_CLASS_ELEMENT_SIZE[y->class]);
    x->length = x->length + y->length;
}

/*-----------------------------------------------------------------------------
 |  Subset
 ----------------------------------------------------------------------------*/

/// Construct a new object as a subset of the original object.
/// @details The attribute will be dropped.
/// @param x (pl_object). The object.
/// @param indices (pl_object). Indices.
/// @return A new object.
static pl_object subset(pl_object x, pl_object indices)
{
    check_null_pointer(x);
    check_null_pointer(indices);
    check_object_type(indices, PL_CLASS_INT);

    return primitive_subset(x, indices->length, indices->data);
}

/*-----------------------------------------------------------------------------
 |  Copy
 ----------------------------------------------------------------------------*/

/// Construct a shallow copy of an object.
/// @details The attribute will be dropped.
/// @param x (pl_object). The object.
/// @return A new object.
static pl_object copy(pl_object x)
{
    check_null_pointer(x);

    pl_object object = primitive_new(x->class, x->length);
    object->length   = x->length;
    memcpy(object->data,
           x->data,
           (size_t) x->length * PL_CLASS_ELEMENT_SIZE[x->class]);
    return object;
}

/*-----------------------------------------------------------------------------
 |  In
 ----------------------------------------------------------------------------*/

/// Check if each element of x is in y.
/// @param x (pl_object). The object.
/// @param y (pl_object). Another object.
/// @return A new object of type PL_CLASS_INT of
/// the same length as x.
static pl_object in(pl_object x, pl_object y)
{
    check_null_pointer(x);
    check_null_pointer(y);
    check_same_type(x, y);

    // New an object for storing the result.
    pl_object object             = primitive_new(PL_CLASS_INT, x->length == 0 ? 1 : x->length);
    object->length               = x->length;
    int *const object_data_array = object->data;

    // Get the underlying type.
    const pl_class_ns class_ns = pl_class_get_ns();
    const int type             = class_ns.type(y->class);

    switch (type)
    {
        case PL_CLASS_CHAR:
        {
            char *const set_data_array = y->data;
            char *const data_array     = x->data;
            pl_misc_for_i(x->length)
            {
                int item_result                      = 0;
                pl_misc_for_j(y->length) item_result = item_result ||
                                                       (data_array[i] == set_data_array[j]);
                object_data_array[i] = item_result;
            }
            break;
        }
        case PL_CLASS_INT:
        {
            int *const set_data_array = y->data;
            int *const data_array     = x->data;
            pl_misc_for_i(x->length)
            {
                int item_result                      = 0;
                pl_misc_for_j(y->length) item_result = item_result ||
                                                       (data_array[i] == set_data_array[j]);
                object_data_array[i] = item_result;
            }
            break;
        }
        case PL_CLASS_LONG:
        {
            long *const set_data_array = y->data;
            long *const data_array     = x->data;
            pl_misc_for_i(x->length)
            {
                int item_result                      = 0;
                pl_misc_for_j(y->length) item_result = item_result ||
                                                       (data_array[i] == set_data_array[j]);
                object_data_array[i] = item_result;
            }
            break;
        }
        case PL_CLASS_DOUBLE:
        {
            double *const set_data_array = y->data;
            double *const data_array     = x->data;
            pl_misc_for_i(x->length)
            {
                int item_result                      = 0;
                pl_misc_for_j(y->length) item_result = item_result ||
                                                       (data_array[i] == set_data_array[j]) ||
                                                       (pl_is_na(data_array[i]) && pl_is_na(set_data_array[i]));
                object_data_array[i] = item_result;
            }
            break;
        }
        default:
        {
            pl_object *const set_data_array = y->data;
            pl_object *const data_array     = x->data;
            pl_misc_for_i(x->length)
            {
                int item_result                      = 0;
                pl_misc_for_j(y->length) item_result = item_result ||
                                                       (data_array[i] == set_data_array[j]);
                object_data_array[i] = item_result;
            }
            break;
        }
    }

    return object;
}


/*-----------------------------------------------------------------------------
 |  Print
 ----------------------------------------------------------------------------*/

static void print(pl_object x)
{
    check_null_pointer(x);

    if (x->length == 0)
    {
        puts("[]");
        return;
    }

    const pl_class_ns class_ns = pl_class_get_ns();
    const int type             = class_ns.type(x->class);

    putchar('[');

    switch (type)
    {
        case PL_CLASS_CHAR:
        {
            const char *data_array = x->data;
            pl_misc_for_i(x->length - 1)
            {
                if (pl_is_na(data_array[i]))
                    printf("NA, ");
                else
                    printf("'%c', ", data_array[i]);
            }
            if (pl_is_na(data_array[x->length - 1]))
                printf("NA");
            else
                printf("'%c'", data_array[x->length - 1]);
            break;
        }
        case PL_CLASS_INT:
        {
            const int *data_array = x->data;
            pl_misc_for_i(x->length - 1)
            {
                if (pl_is_na(data_array[i]))
                    printf("NA, ");
                else
                    printf("%d, ", data_array[i]);
            }
            if (pl_is_na(data_array[x->length - 1]))
                printf("NA");
            else
                printf("%d", data_array[x->length - 1]);
            break;
        }
        case PL_CLASS_LONG:
        {
            const long *data_array = x->data;
            pl_misc_for_i(x->length - 1)
            {
                if (pl_is_na(data_array[i]))
                    printf("NA, ");
                else
                    printf("%ldL, ", data_array[i]);
            }
            if (pl_is_na(data_array[x->length - 1]))
                printf("NA");
            else
                printf("%ldL", data_array[x->length - 1]);
            break;
        }
        case PL_CLASS_DOUBLE:
        {
            const double *data_array = x->data;
            pl_misc_for_i(x->length - 1)
            {
                if (pl_is_na(data_array[i]))
                    printf("NA, ");
                else
                    printf("%.2f, ", data_array[i]);
            }
            if (pl_is_na(data_array[x->length - 1]))
                printf("NA");
            else
                printf("%.f2", data_array[x->length - 1]);
            break;
        }
        default:
        {
            const pl_object *data_array = x->data;
            pl_misc_for_i(x->length - 1)
            {
                if (pl_is_na(data_array[i]))
                    printf("NA, ");
                else
                    printf("<%s>, ", PL_CLASS_NAME[data_array[i]->class]);
            }
            if (pl_is_na(data_array[x->length - 1]))
                printf("NA");
            else
                printf("<%s>", PL_CLASS_NAME[data_array[x->length - 1]->class]);
            break;
        }
    }

    puts("]");
}

/*-----------------------------------------------------------------------------
 |  As char
 ----------------------------------------------------------------------------*/

static pl_object as_char(pl_object x)
{
    check_null_pointer(x);

    const pl_class_ns class_ns = pl_class_get_ns();
    const int type             = class_ns.type(x->class);

    pl_object object = primitive_new(PL_CLASS_CHAR, x->length);
    object->length   = x->length;

    switch (type)
    {
        case PL_CLASS_CHAR:
        {
            char *const data_array                = x->data;
            char *const dst_array                 = object->data;
            pl_misc_for_i(x->length) dst_array[i] = data_array[i];
            break;
        }
        case PL_CLASS_INT:
        {
            int *const data_array                 = x->data;
            char *const dst_array                 = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ||
                                                                    data_array[i] < CHAR_MIN ||
                                                                    data_array[i] > CHAR_MAX ?
                                                            PL_CHAR_NA :
                                                            (char) data_array[i];
            break;
        }
        case PL_CLASS_LONG:
        {
            long *const data_array                = x->data;
            char *const dst_array                 = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ||
                                                                    data_array[i] < CHAR_MIN ||
                                                                    data_array[i] > CHAR_MAX ?
                                                            PL_CHAR_NA :
                                                            (char) data_array[i];
            break;
        }
        case PL_CLASS_DOUBLE:
        {
            double *const data_array              = x->data;
            char *const dst_array                 = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ||
                                                                    data_array[i] < CHAR_MIN ||
                                                                    data_array[i] > CHAR_MAX ?
                                                            PL_CHAR_NA :
                                                            (char) data_array[i];
            break;
        }
        default:
        {
            pl_error_throw(PL_ERROR_INVALID_CLASS,
                           "Can not convert a [%d] object to a [%d] object!",
                           PL_CLASS_LIST,
                           PL_CLASS_CHAR);
            break;
        }
    }

    return object;
}

/*-----------------------------------------------------------------------------
 |  As int
 ----------------------------------------------------------------------------*/

static pl_object as_int(pl_object x)
{
    check_null_pointer(x);

    const pl_class_ns class_ns = pl_class_get_ns();
    const int type             = class_ns.type(x->class);

    pl_object object = primitive_new(PL_CLASS_INT, x->length);
    object->length   = x->length;

    switch (type)
    {
        case PL_CLASS_CHAR:
        {
            char *const data_array                = x->data;
            int *const dst_array                  = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ? PL_INT_NA : (int) data_array[i];
            break;
        }
        case PL_CLASS_INT:
        {
            int *const data_array                 = x->data;
            int *const dst_array                  = object->data;
            pl_misc_for_i(x->length) dst_array[i] = data_array[i];
            break;
        }
        case PL_CLASS_LONG:
        {
            long *const data_array                = x->data;
            int *const dst_array                  = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ||
                                                                    data_array[i] < INT_MIN ||
                                                                    data_array[i] > INT_MAX ?
                                                            PL_INT_NA :
                                                            (int) data_array[i];
            break;
        }
        case PL_CLASS_DOUBLE:
        {
            double *const data_array              = x->data;
            int *const dst_array                  = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ||
                                                                    data_array[i] < INT_MIN ||
                                                                    data_array[i] > INT_MAX ?
                                                            PL_INT_NA :
                                                            (int) data_array[i];
            break;
        }
        default:
        {
            pl_error_throw(PL_ERROR_INVALID_CLASS,
                           "Can not convert a [%d] object to a [%d] object!",
                           PL_CLASS_LIST,
                           PL_CLASS_INT);
            break;
        }
    }

    return object;
}

/*-----------------------------------------------------------------------------
 |  As long
 ----------------------------------------------------------------------------*/

static pl_object as_long(pl_object x)
{
    check_null_pointer(x);

    const pl_class_ns class_ns = pl_class_get_ns();
    const int type             = class_ns.type(x->class);

    pl_object object = primitive_new(PL_CLASS_LONG, x->length);
    object->length   = x->length;

    switch (type)
    {
        case PL_CLASS_CHAR:
        {
            char *const data_array                = x->data;
            long *const dst_array                 = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ? PL_LONG_NA : (long) data_array[i];
            break;
        }
        case PL_CLASS_INT:
        {
            int *const data_array                 = x->data;
            long *const dst_array                 = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ? PL_LONG_NA : (long) data_array[i];
            break;
        }
        case PL_CLASS_LONG:
        {
            long *const data_array                = x->data;
            long *const dst_array                 = object->data;
            pl_misc_for_i(x->length) dst_array[i] = data_array[i];
            break;
        }
        case PL_CLASS_DOUBLE:
        {
            double *const data_array              = x->data;
            long *const dst_array                 = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ||
                                                                    data_array[i] < LONG_MIN ||
                                                                    data_array[i] > LONG_MAX ?
                                                            PL_LONG_NA :
                                                            (long) data_array[i];
            break;
        }
        default:
        {
            pl_error_throw(PL_ERROR_INVALID_CLASS,
                           "Can not convert a [%d] object to a [%d] object!",
                           PL_CLASS_LIST,
                           PL_CLASS_LONG);
            break;
        }
    }

    return object;
}


/*-----------------------------------------------------------------------------
 |  As double
 ----------------------------------------------------------------------------*/

static pl_object as_double(pl_object x)
{
    check_null_pointer(x);

    const pl_class_ns class_ns = pl_class_get_ns();
    const int type             = class_ns.type(x->class);

    pl_object object = primitive_new(PL_CLASS_DOUBLE, x->length);
    object->length   = x->length;

    switch (type)
    {
        case PL_CLASS_CHAR:
        {
            char *const data_array                = x->data;
            double *const dst_array               = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ? PL_DOUBLE_NA : (double) data_array[i];
            break;
        }
        case PL_CLASS_INT:
        {
            int *const data_array                 = x->data;
            double *const dst_array               = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ? PL_DOUBLE_NA : (double) data_array[i];
            break;
        }
        case PL_CLASS_LONG:
        {
            long *const data_array                = x->data;
            double *const dst_array               = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ? PL_DOUBLE_NA : (double) data_array[i];
            break;
        }
        case PL_CLASS_DOUBLE:
        {
            double *const data_array              = x->data;
            double *const dst_array               = object->data;
            pl_misc_for_i(x->length) dst_array[i] = data_array[i];
            break;
        }
        default:
        {
            pl_error_throw(PL_ERROR_INVALID_CLASS,
                           "Can not convert a [%d] object to a [%d] object!",
                           PL_CLASS_LIST,
                           PL_CLASS_DOUBLE);
            break;
        }
    }

    return object;
}

/*-----------------------------------------------------------------------------
 |  Attribute
 ----------------------------------------------------------------------------*/


static int primitive_index_attribute(pl_object x, pl_object name)
{
    check_null_pointer(x);
    check_null_pointer(name);
    check_object_type(name, PL_CLASS_CHAR);

    if (x->attribute == NULL)
        return -1;

    char *const input_array = name->data;
    pl_object name_array    = ((pl_object *) name->data)[0]->data;
    const int num_name      = ((pl_object *) name->data)[0]->length;

    pl_misc_for_i(num_name)
    {
        if (name_array[i].length == name->length)
        {
            char *const this_name = name_array[i].data;
            int flag              = 0;
            pl_misc_for_j(name_array[i].length)
            {
                flag = flag && this_name[j] == input_array[j];
            }
            if (flag == 1)
                return i;
        }
    }

    return -1;
}

static pl_object has_attribute(pl_object x, pl_object name)
{
    int result = primitive_index_attribute(x, name) != -1;
    return primitive_new_from_array(PL_CLASS_INT, 1, &result);
}

static pl_object get_attribute(pl_object x, pl_object name)
{
    int index = primitive_index_attribute(x, name);
}


/*-----------------------------------------------------------------------------
 |  Get object namespace
 ----------------------------------------------------------------------------*/

pl_object_ns pl_object_get_ns(void)
{
    static const pl_object_ns object_ns = {.primitive = {.new               = primitive_new,
                                                         .new_from_array    = primitive_new_from_array,
                                                         .new_from_variadic = primitive_new_from_variadic,
                                                         .to_array          = primitive_to_array,
                                                         .reserve           = primitive_reserve,
                                                         .set_char          = primitive_set_char,
                                                         .set_int           = primitive_set_int,
                                                         .set_long          = primitive_set_long,
                                                         .set_double        = primitive_set_double,
                                                         .set_object        = primitive_set_pl_object,
                                                         .set_by_indices    = primitive_set_by_indices,
                                                         .set_range         = primitive_set_range,
                                                         .extract_char      = primitive_extract_char,
                                                         .extract_int       = primitive_extract_int,
                                                         .extract_long      = primitive_extract_long,
                                                         .extract_double    = primitive_extract_double,
                                                         .extract_object    = primitive_extract_pl_object,
                                                         .extend_char       = primitive_extend_char,
                                                         .extend_int        = primitive_extend_int,
                                                         .extend_long       = primitive_extend_long,
                                                         .extend_double     = primitive_extend_double,
                                                         .extend_object     = primitive_extend_pl_object,
                                                         .subset            = primitive_subset,
                                                         .subset_exclude    = primitive_subset_exclude,
                                                         .subset_by_bool    = primitive_subset_by_bool},
                                           .new       = new,
                                           .reserve   = reserve,
                                           .set       = set,
                                           .append    = append,
                                           .extract   = extract,
                                           .extend    = extend,
                                           .subset    = subset,
                                           .copy      = copy,
                                           .in        = in,
                                           .print     = print,
                                           .as_char   = as_char,
                                           .as_int    = as_int,
                                           .as_long   = as_long,
                                           .as_double = as_double};

    return object_ns;
}
