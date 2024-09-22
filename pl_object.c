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

static int pl_object_print_num_decimals = 2;

/*-----------------------------------------------------------------------------
 |  Checks
 ----------------------------------------------------------------------------*/

#define check_null_pointer(x) pl_error_expect((x) != NULL,                      \
                                              PL_ERROR_UNEXPECTED_NULL_POINTER, \
                                              "Unexpected NULL pointer `" #x "` provided!")

#define check_object_type(object, this_type)                              \
    do {                                                                  \
        const int object_type = pl_class_get_ns().type((object)->class);  \
        pl_error_expect(object_type == (this_type),                       \
                        PL_ERROR_INVALID_CLASS,                           \
                        "Object `" #object "` [%s] is not of type [%s]!", \
                        PL_CLASS_NAME[object_type],                       \
                        PL_CLASS_NAME[this_type]);                        \
    } while (0)

#define check_same_type(x, y)                                                                    \
    do {                                                                                         \
        const pl_class_ns class_ns = pl_class_get_ns();                                          \
        const int x_type           = class_ns.type((x)->class);                                  \
        const int y_type           = class_ns.type((y)->class);                                  \
        pl_error_expect(x_type == y_type,                                                        \
                        PL_ERROR_INVALID_CLASS,                                                  \
                        "Object `" #x "` [%s] is not of the same type as object `" #y "` [%s]!", \
                        PL_CLASS_NAME[x_type],                                                   \
                        PL_CLASS_NAME[y_type]);                                                  \
    } while (0)

#define check_object_length(object, len) pl_error_expect((object)->length == (len),                               \
                                                         PL_ERROR_INVALID_LENGTH,                                 \
                                                         "Object `" #object "` has length [%d] instead of [%d]!", \
                                                         (object)->length,                                        \
                                                         len)

#define check_index_out_of_bound(object, index) pl_error_expect(((index) >= 0) && ((index) <= (object)->length - 1),                       \
                                                                PL_ERROR_INDEX_OUT_OF_BOUND,                                               \
                                                                "Index [%d] out of bound [0, %d) while accessing elements of!" #object "", \
                                                                index,                                                                     \
                                                                (object)->length)

#define check_missing_value(x) pl_error_expect(!pl_is_na(x),        \
                                               PL_ERROR_INVALID_NA, \
                                               "Unexpected missing value `" #x "` !")

#define check_incompatible_length(target, provided) pl_error_expect(((target) == (provided)) || ((provided) == 1),                             \
                                                                    PL_ERROR_INCOMPATIBLE_LENGTH,                                              \
                                                                    "Incompatible length [%d] provided! Expecting length [1] or length [%d]!", \
                                                                    target,                                                                    \
                                                                    provided)

/*-----------------------------------------------------------------------------
 |  Primitive new
 ----------------------------------------------------------------------------*/

static pl_object primitive_new(const int class, const int capacity) {
    return pl_gc_get_ns().new_object(class, capacity);
}

/*-----------------------------------------------------------------------------
 |  Primitive new from array
 ----------------------------------------------------------------------------*/

// If `length` is zero, an empty object will be returned. The `array` will not be used.
static pl_object primitive_new_from_array(const int class, const int length, const void *const array) {
    if (length == 0)
        return primitive_new(class, 1);
    pl_error_expect(length > 0,
                    PL_ERROR_INVALID_LENGTH,
                    "Invalid length [%d]!",
                    length);
    check_null_pointer(array);

    pl_object object = primitive_new(class, length);
    object->length = length;

    memmove(object->data,
            array,
            pl_object_data_size(class, length));
    return object;
}

/*-----------------------------------------------------------------------------
 |  Primitive new from variadic
 ----------------------------------------------------------------------------*/

// If `length` is zero, an empty object will be returned.
static pl_object primitive_new_from_variadic(const int class, const int length, ...) {
    if (length == 0)
        return primitive_new(class, 1);
    pl_error_expect(length > 0,
                    PL_ERROR_INVALID_LENGTH,
                    "Invalid length [%d]!",
                    length);

    pl_object object = primitive_new(class, length);
    object->length = length;

    // Get the underlying type.
    const int type = pl_class_get_ns().type(class);

    va_list ap;
    va_start(ap, length);
    switch (type) {
        case PL_CLASS_CHAR: {
            char *const data_array = object->data;
            pl_misc_for_i(length) data_array[i] = (char) va_arg(ap, int);
            break;
        }
        case PL_CLASS_INT: {
            int *const data_array = object->data;
            pl_misc_for_i(length) data_array[i] = va_arg(ap, int);
            break;
        }
        case PL_CLASS_LONG: {
            long *const data_array = object->data;
            pl_misc_for_i(length) data_array[i] = va_arg(ap, long);
            break;
        }
        case PL_CLASS_DOUBLE: {
            double *const data_array = object->data;
            pl_misc_for_i(length) data_array[i] = va_arg(ap, double);
            break;
        }
        case PL_CLASS_LIST: {
            pl_object *const data_array = object->data;
            pl_misc_for_i(length) data_array[i] = va_arg(ap, pl_object);
            break;
        }
        case PL_CLASS_EXTERNAL: {
            void **const data_array = object->data;
            pl_misc_for_i(length) data_array[i] = va_arg(ap, void *);
            break;
        }
        default:
            break;
    }
    va_end(ap);

    return object;
}


/*-----------------------------------------------------------------------------
 |  Primitive to array
 ----------------------------------------------------------------------------*/

static void primitive_to_array(pl_object x, void *const array) {
    check_null_pointer(x);
    check_null_pointer(array);
    memmove(array,
            x->data,
            pl_object_data_size(x->class, x->length));
}

/*-----------------------------------------------------------------------------
 |  Primitive reserve
 ----------------------------------------------------------------------------*/

static void primitive_reserve(pl_object x, const int capacity) {
    pl_gc_get_ns().reserve_object(x, capacity);
}

/*-----------------------------------------------------------------------------
 |  Primitive shrink
 ----------------------------------------------------------------------------*/

// Data may be lost due to the shrink.
static void primitive_shrink(pl_object x, const int capacity) {
    check_null_pointer(x);
    check_missing_value(capacity);

    if (capacity >= x->capacity)
        return;

    pl_gc_get_ns().resize_object(x, capacity);
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


primitive_set_type_template(char, PL_CLASS_CHAR);

primitive_set_type_template(int, PL_CLASS_INT);

primitive_set_type_template(long, PL_CLASS_LONG);

primitive_set_type_template(double, PL_CLASS_DOUBLE);

static void primitive_set_object(pl_object x, const int index, pl_object item) {
    check_null_pointer(x);
    if (pl_is_na(index))
        return;
    check_index_out_of_bound(x, index);
    check_object_type(x, PL_CLASS_LIST);

    ((pl_object *) x->data)[index] = item;
}

static void primitive_set_external(pl_object x, const int index, void *item) {
    check_null_pointer(x);
    if (pl_is_na(index))
        return;
    check_index_out_of_bound(x, index);
    check_object_type(x, PL_CLASS_EXTERNAL);

    ((void **) x->data)[index] = item;
}

/*-----------------------------------------------------------------------------
 |  Primitive set by indices
 ----------------------------------------------------------------------------*/

// If an index of `indices` is NA, the corresponding value will not be set.
// If `length` is zero, no items will be set and `array` will not be used.
static void primitive_set_by_indices(pl_object x, const int length, const int *const indices, const void *const array) {
    check_null_pointer(x);

    check_missing_value(length);
    if (length == 0)
        return;
    pl_error_expect(length > 0,
                    PL_ERROR_INVALID_LENGTH,
                    "Invalid length [%d]!",
                    length);

    check_null_pointer(indices);
    pl_misc_for_i(length) {
        if (!pl_is_na(indices[i]))
            check_index_out_of_bound(x, indices[i]);
    }

    check_null_pointer(array);

    const int type = pl_class_get_ns().type(x->class);
    switch (type) {
        case PL_CLASS_CHAR: {
            char *const data_array = x->data;
            const char *const src_array = array;
            pl_misc_for_i(length) if (!pl_is_na(indices[i])) data_array[indices[i]] = src_array[i];
            break;
        }
        case PL_CLASS_INT: {
            int *const data_array = x->data;
            const int *const src_array = array;
            pl_misc_for_i(length) if (!pl_is_na(indices[i])) data_array[indices[i]] = src_array[i];
            break;
        }
        case PL_CLASS_LONG: {
            long *const data_array = x->data;
            const long *const src_array = array;
            pl_misc_for_i(length) if (!pl_is_na(indices[i])) data_array[indices[i]] = src_array[i];
            break;
        }
        case PL_CLASS_DOUBLE: {
            double *const data_array = x->data;
            const double *const src_array = array;
            pl_misc_for_i(length) if (!pl_is_na(indices[i])) data_array[indices[i]] = src_array[i];
            break;
        }
        case PL_CLASS_LIST: {
            pl_object *const data_array = x->data;
            const pl_object *const src_array = array;
            pl_misc_for_i(length) if (!pl_is_na(indices[i])) data_array[indices[i]] = src_array[i];
            break;
        }
        case PL_CLASS_EXTERNAL: {
            void **const data_array = x->data;
            void *const *const src_array = array;
            pl_misc_for_i(length) if (!pl_is_na(indices[i])) data_array[indices[i]] = src_array[i];
            break;
        }
        default:
            break;
    }
}

/*-----------------------------------------------------------------------------
 |  Primitive set range
 ----------------------------------------------------------------------------*/

static void primitive_set_range(pl_object x, const int start, const int end, const void *const array) {
    check_null_pointer(x);
    check_index_out_of_bound(x, start);
    check_index_out_of_bound(x, end);
    check_null_pointer(array);

    if (start > end)
        return;

    memmove((char *) x->data + pl_object_data_size(x->class, start),
            array,
            pl_object_data_size(x->class, end - start + 1));
}

/*-----------------------------------------------------------------------------
 |  Primitive set by Booleans
 ----------------------------------------------------------------------------*/

static void primitive_set_by_bool(pl_object x, const int *const bool_array, const void *const array) {
    check_null_pointer(x);
    check_null_pointer(bool_array);
    check_null_pointer(array);

    pl_misc_for_i(x->length) check_missing_value(bool_array[i]);

    const int type = pl_class_get_ns().type(x->class);

    int count = 0;
    switch (type) {
        case PL_CLASS_CHAR: {
            char *const data_array = x->data;
            const char *const src_array = array;
            pl_misc_for_i(x->length) {
                if (bool_array[i] == 1) {
                    data_array[i] = src_array[count];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_INT: {
            int *const data_array = x->data;
            const int *const src_array = array;
            pl_misc_for_i(x->length) {
                if (bool_array[i] == 1) {
                    data_array[i] = src_array[count];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_LONG: {
            long *const data_array = x->data;
            const long *const src_array = array;
            pl_misc_for_i(x->length) {
                if (bool_array[i] == 1) {
                    data_array[i] = src_array[count];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_DOUBLE: {
            double *const data_array = x->data;
            const double *const src_array = array;
            pl_misc_for_i(x->length) {
                if (bool_array[i] == 1) {
                    data_array[i] = src_array[count];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_LIST: {
            pl_object *const data_array = x->data;
            const pl_object *const src_array = array;
            pl_misc_for_i(x->length) {
                if (bool_array[i] == 1) {
                    data_array[i] = src_array[count];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_EXTERNAL: {
            void **const data_array = x->data;
            void *const *const src_array = array;
            pl_misc_for_i(x->length) {
                if (bool_array[i] == 1) {
                    data_array[i] = src_array[count];
                    count++;
                }
            }
            break;
        }
        default:
            break;
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


primitive_extract_type_template(char, PL_CLASS_CHAR, PL_CHAR_NA);

primitive_extract_type_template(int, PL_CLASS_INT, PL_INT_NA);

primitive_extract_type_template(long, PL_CLASS_LONG, PL_LONG_NA);

primitive_extract_type_template(double, PL_CLASS_DOUBLE, PL_DOUBLE_NA);

static pl_object primitive_extract_object(pl_object x, const int index) {
    check_null_pointer(x);
    check_object_type(x, PL_CLASS_LIST);

    if (pl_is_na(index))
        return NULL;
    check_index_out_of_bound(x, index);

    return ((pl_object *) x->data)[index];
}

static void *primitive_extract_external(pl_object x, const int index) {
    check_null_pointer(x);
    check_object_type(x, PL_CLASS_EXTERNAL);

    if (pl_is_na(index))
        return NULL;
    check_index_out_of_bound(x, index);

    return ((void **) x->data)[index];
}

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

primitive_extend_type_template(char, PL_CLASS_CHAR);

primitive_extend_type_template(int, PL_CLASS_INT);

primitive_extend_type_template(double, PL_CLASS_DOUBLE);

primitive_extend_type_template(long, PL_CLASS_LONG);

static void primitive_extend_object(pl_object x, pl_object item) {
    check_null_pointer(x);
    check_object_type(x, PL_CLASS_LIST);

    primitive_reserve(x, x->length + 1);
    ((pl_object *) x->data)[x->length] = item;
    x->length += 1;
}

static void primitive_extend_external(pl_object x, void *item) {
    check_null_pointer(x);
    check_object_type(x, PL_CLASS_EXTERNAL);

    primitive_reserve(x, x->length + 1);
    ((void **) x->data)[x->length] = item;
    x->length += 1;
}

/*-----------------------------------------------------------------------------
 |  Primitive subset
 ----------------------------------------------------------------------------*/

// The attribute will be dropped. If an index of `indices` is NA,
// the corresponding item will be NA. If `length` is zero, an empty object will be returned.
static pl_object primitive_subset(pl_object x, const int length, const int *const indices) {
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
    pl_misc_for_i(length) {
        if (!pl_is_na(indices[i]))
            check_index_out_of_bound(x, indices[i]);
    }

    // Request a new object.
    pl_object object = primitive_new(x->class, length);
    object->length = length;

    // Get the underlying type.
    const int type = pl_class_get_ns().type(x->class);

    switch (type) {
        case PL_CLASS_CHAR: {
            char *const src_array = x->data;
            char *const dst_array = object->data;
            pl_misc_for_i(length) dst_array[i] = pl_is_na(indices[i]) ? PL_CHAR_NA : src_array[indices[i]];
            break;
        }
        case PL_CLASS_INT: {
            int *const src_array = x->data;
            int *const dst_array = object->data;
            pl_misc_for_i(length) dst_array[i] = pl_is_na(indices[i]) ? PL_INT_NA : src_array[indices[i]];
            break;
        }
        case PL_CLASS_LONG: {
            long *const src_array = x->data;
            long *const dst_array = object->data;
            pl_misc_for_i(length) dst_array[i] = pl_is_na(indices[i]) ? PL_LONG_NA : src_array[indices[i]];
            break;
        }
        case PL_CLASS_DOUBLE: {
            double *const src_array = x->data;
            double *const dst_array = object->data;
            pl_misc_for_i(length) dst_array[i] = pl_is_na(indices[i]) ? PL_DOUBLE_NA : src_array[indices[i]];
            break;
        }
        case PL_CLASS_LIST: {
            pl_object *const src_array = x->data;
            pl_object *const dst_array = object->data;
            pl_misc_for_i(length) dst_array[i] = pl_is_na(indices[i]) ? PL_LIST_NA : src_array[indices[i]];
            break;
        }
        case PL_CLASS_EXTERNAL: {
            void **const src_array = x->data;
            void **const dst_array = object->data;
            pl_misc_for_i(length) dst_array[i] = pl_is_na(indices[i]) ? PL_EXTERNAL_NA : src_array[indices[i]];
            break;
        }
        default:
            break;
    }

    return object;
}

/*-----------------------------------------------------------------------------
 |  Primitive subset exclude
 ----------------------------------------------------------------------------*/

// The attribute will be dropped. NAs in `indices` will be ignored.
// Duplicate indices will be ignored. If `length` is zero, a shallow
// copy of the object will be returned.
static pl_object primitive_subset_exclude(pl_object x, const int length, const int *const indices) {
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
    pl_misc_for_i(length) {
        if (!pl_is_na(indices[i]))
            check_index_out_of_bound(x, indices[i]);
    }

    pl_object index_flags = primitive_new(PL_CLASS_INT, x->length);
    int *const index_array = index_flags->data;
    pl_misc_for_i(x->length) index_array[i] = 1;
    index_flags->length = x->length;

    pl_misc_for_i(length) if (!pl_is_na(indices[i])) index_array[indices[i]] = 0;

    int count = 0;
    pl_misc_for_i(x->length) {
        if (index_array[i] == 1) {
            index_array[count] = i;
            count++;
        }
    }

    return primitive_subset(x, count, index_array);
}

/*-----------------------------------------------------------------------------
 |  Primitive subset by Booleans
 ----------------------------------------------------------------------------*/

static pl_object primitive_subset_by_bool(pl_object x, const int *const bool_array) {

    check_null_pointer(x);
    check_null_pointer(bool_array);

    // Check Nas.
    pl_misc_for_i(x->length) check_missing_value(bool_array[i]);

    // Get the underlying type.
    const int type = pl_class_get_ns().type(x->class);

    int count = 0;
    pl_misc_for_i(x->length) {
        if (bool_array[i] == 1)
            count += 1;
    }
    pl_object object = primitive_new(x->class, count);
    object->length = count;

    count = 0;
    switch (type) {
        case PL_CLASS_CHAR: {
            char *const src_array = x->data;
            char *const dst_array = object->data;
            pl_misc_for_i(x->length) {
                if (bool_array[i] == 1) {
                    dst_array[count] = src_array[i];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_INT: {
            int *const src_array = x->data;
            int *const dst_array = object->data;
            pl_misc_for_i(x->length) {
                if (bool_array[i] == 1) {
                    dst_array[count] = src_array[i];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_LONG: {
            long *const src_array = x->data;
            long *const dst_array = object->data;
            pl_misc_for_i(x->length) {
                if (bool_array[i] == 1) {
                    dst_array[count] = src_array[i];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_DOUBLE: {
            double *const src_array = x->data;
            double *const dst_array = object->data;
            pl_misc_for_i(x->length) {
                if (bool_array[i] == 1) {
                    dst_array[count] = src_array[i];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_LIST: {
            pl_object *const src_array = x->data;
            pl_object *const dst_array = object->data;
            pl_misc_for_i(x->length) {
                if (bool_array[i] == 1) {
                    dst_array[count] = src_array[i];
                    count++;
                }
            }
            break;
        }
        case PL_CLASS_EXTERNAL: {
            void **const src_array = x->data;
            void **const dst_array = object->data;
            pl_misc_for_i(x->length) {
                if (bool_array[i] == 1) {
                    dst_array[count] = src_array[i];
                    count++;
                }
            }
            break;
        }
        default:
            break;
    }

    return object;
}

/*-----------------------------------------------------------------------------
 |  Primitive remove
 ----------------------------------------------------------------------------*/

static void primitive_remove(pl_object x, const int start, const int end) {
    check_null_pointer(x);
    check_missing_value(start);
    check_missing_value(end);
    check_index_out_of_bound(x, start);
    check_index_out_of_bound(x, end);

    if (end + 1 == x->length) {
        x->length = start;
        return;
    }

    memmove((char *) x->data + pl_object_data_size(x->class, start),
            (char *) x->data + pl_object_data_size(x->class, end + 1),
            pl_object_data_size(x->class, x->length - end - 1));
    x->length = x->length - (end - start + 1);
}

/*-----------------------------------------------------------------------------
 |  Primitive remove by indices
 ----------------------------------------------------------------------------*/

// NAs in `indices` will be ignored. Duplicate indices will be ignored.
static void primitive_remove_by_indices(pl_object x, const int length, const int *const indices) {
    pl_object new_x = primitive_subset_exclude(x, length, indices);
    memmove(x->data,
            new_x->data,
            pl_object_data_size(x->class, new_x->length));
    x->length = new_x->length;
}

/*-----------------------------------------------------------------------------
 |  New
 ----------------------------------------------------------------------------*/

static pl_object new(pl_object class, pl_object capacity) {
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

static void reserve(pl_object x, pl_object capacity) {
    check_null_pointer(x);
    check_null_pointer(capacity);
    check_object_type(capacity, PL_CLASS_INT);
    check_object_length(capacity, 1);

    primitive_reserve(x, ((int *) capacity->data)[0]);
}


/*-----------------------------------------------------------------------------
 |  Set
 ----------------------------------------------------------------------------*/

static void set(pl_object x, pl_object indices, pl_object items) {
    check_null_pointer(x);
    check_null_pointer(indices);
    check_null_pointer(items);
    check_object_type(indices, PL_CLASS_INT);
    check_same_type(x, items);

    primitive_set_by_indices(x, indices->length, indices->data, items->data);
}

/*-----------------------------------------------------------------------------
 |  Set range
 ----------------------------------------------------------------------------*/

static void set_range(pl_object x, pl_object start, pl_object end, pl_object items) {
    check_null_pointer(x);
    check_null_pointer(items);
    check_null_pointer(start);
    check_null_pointer(end);
    check_object_type(start, PL_CLASS_INT);
    check_object_length(start, 1);
    check_object_type(end, PL_CLASS_INT);
    check_object_length(end, 1);
    check_same_type(x, items);

    const int start_int = ((int *) start->data)[0];
    const int end_int = ((int *) end->data)[0];
    check_missing_value(start_int);
    check_missing_value(end_int);
    check_index_out_of_bound(x, start_int);
    check_index_out_of_bound(x, end_int);

    const int required_length = end_int - start_int + 1;
    if (required_length <= 0)
        return;

    check_incompatible_length(required_length, items->length);
    if (items->length == required_length) {
        primitive_set_range(x, start_int, end_int, items->data);
    } else if (items->length == 1) {
        pl_object new_y = primitive_new(x->class, required_length);
        new_y->length = required_length;
        switch (x->class) {
            case PL_CLASS_CHAR: {
                char *const new_y_array = new_y->data;
                const char item = ((char *) items->data)[0];
                pl_misc_for_i(required_length) {
                    new_y_array[i] = item;
                }
                break;
            }
            case PL_CLASS_INT: {
                int *const new_y_array = new_y->data;
                const int item = ((int *) items->data)[0];
                pl_misc_for_i(required_length) {
                    new_y_array[i] = item;
                }
                break;
            }
            case PL_CLASS_LONG: {
                long *const new_y_array = new_y->data;
                const long item = ((long *) items->data)[0];
                pl_misc_for_i(required_length) {
                    new_y_array[i] = item;
                }
                break;
            }
            case PL_CLASS_DOUBLE: {
                double *const new_y_array = new_y->data;
                const double item = ((double *) items->data)[0];
                pl_misc_for_i(required_length) {
                    new_y_array[i] = item;
                }
                break;
            }
            case PL_CLASS_LIST: {
                pl_object *const new_y_array = new_y->data;
                pl_object item = ((pl_object *) items->data)[0];
                pl_misc_for_i(required_length) {
                    new_y_array[i] = item;
                }
                break;
            }
            case PL_CLASS_EXTERNAL: {
                void **const new_y_array = new_y->data;
                void *item = ((void **) items->data)[0];
                pl_misc_for_i(required_length) {
                    new_y_array[i] = item;
                }
                break;
            }
            default:
                break;
        }

        primitive_set_range(x, start_int, end_int, new_y->data);
    }
}

/*-----------------------------------------------------------------------------
 |  Append
 ----------------------------------------------------------------------------*/

static void append(pl_object x, pl_object item) {
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

// If x is of type PL_CLASS_LIST, this function will return the value directly.
// Otherwise, the function will act the same as `subset` and drop the attribute.
static pl_object extract(pl_object x, pl_object index) {
    check_null_pointer(x);
    check_null_pointer(index);
    check_object_type(index, PL_CLASS_INT);
    check_object_length(index, 1);

    const int int_index = ((int *) index->data)[0];
    check_missing_value(int_index);
    check_index_out_of_bound(x, int_index);

    if (pl_class_get_ns().type(x->class) == PL_CLASS_LIST)
        return ((pl_object *) x->data)[int_index];
    else
        return primitive_subset(x, 1, (int[1]) {int_index});
}

/*-----------------------------------------------------------------------------
 |  Extend
 ----------------------------------------------------------------------------*/

static void extend(pl_object x, pl_object y) {
    check_null_pointer(x);
    check_null_pointer(y);
    check_same_type(x, y);

    if (y->length == 0)
        return;

    long result_length = x->length + y->length;
    pl_error_expect(result_length > 0 && result_length < PL_OBJECT_MAX_CAPACITY,
                    PL_ERROR_INVALID_CAPACITY,
                    "Invalid capacity [%ld]!",
                    result_length);

    primitive_reserve(x, (int) result_length);
    memmove((char *) x->data + pl_object_data_size(x->class, x->length),
            y->data,
            pl_object_data_size(y->class, y->length));
    x->length = x->length + y->length;
}

/*-----------------------------------------------------------------------------
 |  Subset
 ----------------------------------------------------------------------------*/

static pl_object subset(pl_object x, pl_object indices) {
    check_null_pointer(x);
    check_null_pointer(indices);
    check_object_type(indices, PL_CLASS_INT);

    return primitive_subset(x, indices->length, indices->data);
}

/*-----------------------------------------------------------------------------
 |  Subset exclude
 ----------------------------------------------------------------------------*/

static pl_object subset_exclude(pl_object x, pl_object indices) {
    check_null_pointer(x);
    check_null_pointer(indices);
    check_object_type(indices, PL_CLASS_INT);

    return primitive_subset_exclude(x, indices->length, indices->data);
}

/*-----------------------------------------------------------------------------
 |  Copy
 ----------------------------------------------------------------------------*/

static pl_object copy(pl_object x) {
    check_null_pointer(x);

    pl_object object = primitive_new(x->class, x->length);
    object->length = x->length;
    memcpy(object->data,
           x->data,
           pl_object_data_size(x->class, x->length));
    return object;
}

/*-----------------------------------------------------------------------------
 |  Equal
 ----------------------------------------------------------------------------*/

static pl_object equal(pl_object x, pl_object y) {
    check_null_pointer(x);
    check_null_pointer(y);
    check_same_type(x, y);

    // Make sure x is longer
    if (x->length < y->length) {
        pl_object tmp = x;
        x = y;
        y = x;
    }

    check_incompatible_length(x->length, y->length);

    // New an object for storing the result.
    pl_object object = primitive_new(PL_CLASS_INT, x->length == 0 ? 1 : x->length);
    object->length = x->length;
    int *const object_data_array = object->data;

    // Get the underlying type.
    const int type = pl_class_get_ns().type(y->class);

    switch (type) {
        case PL_CLASS_CHAR: {
            char *const y_data_array = y->data;
            char *const x_data_array = x->data;
            pl_misc_for_i(x->length) {
                if (y->length > 1) {
                    object_data_array[i] =
                            pl_is_na(x_data_array[i]) || pl_is_na(y_data_array[i]) ? PL_INT_NA : x_data_array[i] ==
                                                                                                 y_data_array[i];
                } else {
                    object_data_array[i] =
                            pl_is_na(x_data_array[i]) || pl_is_na(y_data_array[i]) ? PL_INT_NA : x_data_array[i] ==
                                                                                                 y_data_array[0];
                }
            }
            break;
        }
        case PL_CLASS_INT: {
            int *const y_data_array = y->data;
            int *const x_data_array = x->data;
            pl_misc_for_i(x->length) {
                if (y->length > 1) {
                    object_data_array[i] =
                            pl_is_na(x_data_array[i]) || pl_is_na(y_data_array[i]) ? PL_INT_NA : x_data_array[i] ==
                                                                                                 y_data_array[i];
                } else {
                    object_data_array[i] =
                            pl_is_na(x_data_array[i]) || pl_is_na(y_data_array[i]) ? PL_INT_NA : x_data_array[i] ==
                                                                                                 y_data_array[0];
                }
            }
            break;
        }
        case PL_CLASS_LONG: {
            long *const y_data_array = y->data;
            long *const x_data_array = x->data;
            pl_misc_for_i(x->length) {
                if (y->length > 1) {
                    object_data_array[i] =
                            pl_is_na(x_data_array[i]) || pl_is_na(y_data_array[i]) ? PL_INT_NA : x_data_array[i] ==
                                                                                                 y_data_array[i];
                } else {
                    object_data_array[i] =
                            pl_is_na(x_data_array[i]) || pl_is_na(y_data_array[i]) ? PL_INT_NA : x_data_array[i] ==
                                                                                                 y_data_array[0];
                }
            }
            break;
        }
        case PL_CLASS_DOUBLE: {
            double *const y_data_array = y->data;
            double *const x_data_array = x->data;
            pl_misc_for_i(x->length) {
                if (y->length > 1) {
                    object_data_array[i] =
                            pl_is_na(x_data_array[i]) || pl_is_na(y_data_array[i]) ? PL_INT_NA : x_data_array[i] ==
                                                                                                 y_data_array[i];
                } else {
                    object_data_array[i] =
                            pl_is_na(x_data_array[i]) || pl_is_na(y_data_array[i]) ? PL_INT_NA : x_data_array[i] ==
                                                                                                 y_data_array[0];
                }
            }
            break;
        }
        case PL_CLASS_LIST: {
            pl_object *const y_data_array = y->data;
            pl_object *const x_data_array = x->data;
            pl_misc_for_i(x->length) {
                if (y->length > 1) {
                    object_data_array[i] =
                            pl_is_na(x_data_array[i]) || pl_is_na(y_data_array[i]) ? PL_INT_NA : x_data_array[i] ==
                                                                                                 y_data_array[i];
                } else {
                    object_data_array[i] =
                            pl_is_na(x_data_array[i]) || pl_is_na(y_data_array[i]) ? PL_INT_NA : x_data_array[i] ==
                                                                                                 y_data_array[0];
                }
            }
            break;
        }
        case PL_CLASS_EXTERNAL: {
            void **const y_data_array = y->data;
            void **const x_data_array = x->data;
            pl_misc_for_i(x->length) {
                if (y->length > 1) {
                    object_data_array[i] =
                            pl_is_na(x_data_array[i]) || pl_is_na(y_data_array[i]) ? PL_INT_NA : x_data_array[i] ==
                                                                                                 y_data_array[i];
                } else {
                    object_data_array[i] =
                            pl_is_na(x_data_array[i]) || pl_is_na(y_data_array[i]) ? PL_INT_NA : x_data_array[i] ==
                                                                                                 y_data_array[0];
                }
            }
            break;
        }
    }

    return object;
}

/*-----------------------------------------------------------------------------
 |  In
 ----------------------------------------------------------------------------*/

static pl_object in(pl_object x, pl_object y) {
    check_null_pointer(x);
    check_null_pointer(y);
    check_same_type(x, y);

    // New an object for storing the result.
    pl_object object = primitive_new(PL_CLASS_INT, x->length == 0 ? 1 : x->length);
    object->length = x->length;
    int *const object_data_array = object->data;

    // Get the underlying type.
    const int type = pl_class_get_ns().type(y->class);

    switch (type) {
        case PL_CLASS_CHAR: {
            char *const set_data_array = y->data;
            char *const data_array = x->data;
            pl_misc_for_i(x->length) {
                int item_result = 0;
                pl_misc_for_j(y->length) item_result = item_result ||
                                                       (data_array[i] == set_data_array[j]);
                object_data_array[i] = item_result;
            }
            break;
        }
        case PL_CLASS_INT: {
            int *const set_data_array = y->data;
            int *const data_array = x->data;
            pl_misc_for_i(x->length) {
                int item_result = 0;
                pl_misc_for_j(y->length) item_result = item_result ||
                                                       (data_array[i] == set_data_array[j]);
                object_data_array[i] = item_result;
            }
            break;
        }
        case PL_CLASS_LONG: {
            long *const set_data_array = y->data;
            long *const data_array = x->data;
            pl_misc_for_i(x->length) {
                int item_result = 0;
                pl_misc_for_j(y->length) item_result = item_result ||
                                                       (data_array[i] == set_data_array[j]);
                object_data_array[i] = item_result;
            }
            break;
        }
        case PL_CLASS_DOUBLE: {
            double *const set_data_array = y->data;
            double *const data_array = x->data;
            pl_misc_for_i(x->length) {
                int item_result = 0;
                pl_misc_for_j(y->length) item_result = item_result ||
                                                       (data_array[i] == set_data_array[j]) ||
                                                       (pl_is_na(data_array[i]) && pl_is_na(set_data_array[i]));
                object_data_array[i] = item_result;
            }
            break;
        }
        case PL_CLASS_LIST: {
            pl_object *const set_data_array = y->data;
            pl_object *const data_array = x->data;
            pl_misc_for_i(x->length) {
                int item_result = 0;
                pl_misc_for_j(y->length) item_result = item_result ||
                                                       (data_array[i] == set_data_array[j]);
                object_data_array[i] = item_result;
            }
            break;
        }
        case PL_CLASS_EXTERNAL: {
            void **const set_data_array = y->data;
            void **const data_array = x->data;
            pl_misc_for_i(x->length) {
                int item_result = 0;
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
 |  Set decimals
 ----------------------------------------------------------------------------*/

static void print_set_decimals(pl_object x) {
    check_null_pointer(x);
    check_object_type(x, PL_CLASS_INT);
    check_object_length(x, 1);

    const int num_decimals = ((int *) x->data)[0];
    if (num_decimals >= 0) {
        pl_object_print_num_decimals = num_decimals;
    }
}

/*-----------------------------------------------------------------------------
 |  Print
 ----------------------------------------------------------------------------*/

static void print(pl_object x) {
    check_null_pointer(x);

    if (x->length == 0) {
        puts("[]");
        return;
    }

    const pl_class_ns class_ns = pl_class_get_ns();
    const int type = class_ns.type(x->class);

    putchar('[');

    switch (type) {
        case PL_CLASS_CHAR: {
            const char *data_array = x->data;
            pl_misc_for_i(x->length - 1) {
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
        case PL_CLASS_INT: {
            const int *data_array = x->data;
            pl_misc_for_i(x->length - 1) {
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
        case PL_CLASS_LONG: {
            const long *data_array = x->data;
            pl_misc_for_i(x->length - 1) {
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
        case PL_CLASS_DOUBLE: {
            const double *data_array = x->data;
            char format[100] = "";
            char end_format[100] = "";
            sprintf(format, "%%.%df, ", pl_object_print_num_decimals);
            sprintf(end_format, "%%.%df", pl_object_print_num_decimals);

            pl_misc_for_i(x->length - 1) {
                if (pl_is_na(data_array[i]))
                    printf("NA, ");
                else
                    printf(format, data_array[i]);
            }
            if (pl_is_na(data_array[x->length - 1]))
                printf("NA");
            else
                printf(end_format, data_array[x->length - 1]);
            break;
        }
        case PL_CLASS_LIST: {
            const pl_object *data_array = x->data;
            pl_misc_for_i(x->length - 1) {
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
        case PL_CLASS_EXTERNAL: {
            const pl_object *data_array = x->data;
            pl_misc_for_i(x->length - 1) {
                if (pl_is_na(data_array[i]))
                    printf("NA, ");
                else
                    printf("<EXTERNAL>, ");
            }
            if (pl_is_na(data_array[x->length - 1]))
                printf("NA");
            else
                printf("<EXTERNAL>");
            break;
        }
    }

    puts("]");
}

/*-----------------------------------------------------------------------------
 |  As char
 ----------------------------------------------------------------------------*/

static pl_object as_char(pl_object x) {
    check_null_pointer(x);

    const int type = pl_class_get_ns().type(x->class);

    pl_object object = primitive_new(PL_CLASS_CHAR, x->length);
    object->length = x->length;

    switch (type) {
        case PL_CLASS_CHAR: {
            char *const data_array = x->data;
            char *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = data_array[i];
            break;
        }
        case PL_CLASS_INT: {
            int *const data_array = x->data;
            char *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ||
                                                    data_array[i] < CHAR_MIN ||
                                                    data_array[i] > CHAR_MAX ?
                                                    PL_CHAR_NA :
                                                    (char) data_array[i];
            break;
        }
        case PL_CLASS_LONG: {
            long *const data_array = x->data;
            char *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ||
                                                    data_array[i] < CHAR_MIN ||
                                                    data_array[i] > CHAR_MAX ?
                                                    PL_CHAR_NA :
                                                    (char) data_array[i];
            break;
        }
        case PL_CLASS_DOUBLE: {
            double *const data_array = x->data;
            char *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ||
                                                    data_array[i] < CHAR_MIN ||
                                                    data_array[i] > CHAR_MAX ?
                                                    PL_CHAR_NA :
                                                    (char) data_array[i];
            break;
        }
        default: {
            pl_error_throw(PL_ERROR_INVALID_CLASS,
                           "Can not convert a [%s] object to a [%s] object!",
                           PL_CLASS_NAME[x->class],
                           PL_CLASS_NAME[PL_CLASS_CHAR]);
            break;
        }
    }

    return object;
}

/*-----------------------------------------------------------------------------
 |  As int
 ----------------------------------------------------------------------------*/

static pl_object as_int(pl_object x) {
    check_null_pointer(x);

    const pl_class_ns class_ns = pl_class_get_ns();
    const int type = class_ns.type(x->class);

    pl_object object = primitive_new(PL_CLASS_INT, x->length);
    object->length = x->length;

    switch (type) {
        case PL_CLASS_CHAR: {
            char *const data_array = x->data;
            int *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ? PL_INT_NA : (int) data_array[i];
            break;
        }
        case PL_CLASS_INT: {
            int *const data_array = x->data;
            int *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = data_array[i];
            break;
        }
        case PL_CLASS_LONG: {
            long *const data_array = x->data;
            int *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ||
                                                    data_array[i] < INT_MIN ||
                                                    data_array[i] > INT_MAX ?
                                                    PL_INT_NA :
                                                    (int) data_array[i];
            break;
        }
        case PL_CLASS_DOUBLE: {
            double *const data_array = x->data;
            int *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ||
                                                    data_array[i] < INT_MIN ||
                                                    data_array[i] > INT_MAX ?
                                                    PL_INT_NA :
                                                    (int) data_array[i];
            break;
        }
        default: {
            pl_error_throw(PL_ERROR_INVALID_CLASS,
                           "Can not convert a [%s] object to a [%s] object!",
                           PL_CLASS_NAME[x->class],
                           PL_CLASS_NAME[PL_CLASS_INT]);
            break;
        }
    }

    return object;
}

/*-----------------------------------------------------------------------------
 |  As long
 ----------------------------------------------------------------------------*/

static pl_object as_long(pl_object x) {
    check_null_pointer(x);

    const pl_class_ns class_ns = pl_class_get_ns();
    const int type = class_ns.type(x->class);

    pl_object object = primitive_new(PL_CLASS_LONG, x->length);
    object->length = x->length;

    switch (type) {
        case PL_CLASS_CHAR: {
            char *const data_array = x->data;
            long *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ? PL_LONG_NA : (long) data_array[i];
            break;
        }
        case PL_CLASS_INT: {
            int *const data_array = x->data;
            long *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ? PL_LONG_NA : (long) data_array[i];
            break;
        }
        case PL_CLASS_LONG: {
            long *const data_array = x->data;
            long *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = data_array[i];
            break;
        }
        case PL_CLASS_DOUBLE: {
            double *const data_array = x->data;
            long *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ||
                                                    data_array[i] < LONG_MIN ||
                                                    data_array[i] > LONG_MAX ?
                                                    PL_LONG_NA :
                                                    (long) data_array[i];
            break;
        }
        default: {
            pl_error_throw(PL_ERROR_INVALID_CLASS,
                           "Can not convert a [%s] object to a [%s] object!",
                           PL_CLASS_NAME[x->class],
                           PL_CLASS_NAME[PL_CLASS_LONG]);
            break;
        }
    }

    return object;
}


/*-----------------------------------------------------------------------------
 |  As double
 ----------------------------------------------------------------------------*/

static pl_object as_double(pl_object x) {
    check_null_pointer(x);

    const pl_class_ns class_ns = pl_class_get_ns();
    const int type = class_ns.type(x->class);

    pl_object object = primitive_new(PL_CLASS_DOUBLE, x->length);
    object->length = x->length;

    switch (type) {
        case PL_CLASS_CHAR: {
            char *const data_array = x->data;
            double *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ? PL_DOUBLE_NA : (double) data_array[i];
            break;
        }
        case PL_CLASS_INT: {
            int *const data_array = x->data;
            double *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ? PL_DOUBLE_NA : (double) data_array[i];
            break;
        }
        case PL_CLASS_LONG: {
            long *const data_array = x->data;
            double *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = pl_is_na(data_array[i]) ? PL_DOUBLE_NA : (double) data_array[i];
            break;
        }
        case PL_CLASS_DOUBLE: {
            double *const data_array = x->data;
            double *const dst_array = object->data;
            pl_misc_for_i(x->length) dst_array[i] = data_array[i];
            break;
        }
        default: {
            pl_error_throw(PL_ERROR_INVALID_CLASS,
                           "Can not convert a [%s] object to a [%s] object!",
                           PL_CLASS_NAME[x->class],
                           PL_CLASS_NAME[PL_CLASS_DOUBLE]);
            break;
        }
    }

    return object;
}

/*-----------------------------------------------------------------------------
 |  Attribute
 ----------------------------------------------------------------------------*/

// Returns the index of the matched attribute name. -1 for not found.
static int primitive_index_attribute(pl_object x, pl_object name) {
    check_null_pointer(x);
    check_null_pointer(name);
    check_object_type(name, PL_CLASS_CHAR);

    if (x->attribute == NULL)
        return -1;

    char *const input_array = name->data;

    // There are two objects stored in the attribute
    // The first one is a list of attribute names
    // The second one is a list of attribute values
    pl_object name_array = ((pl_object *) x->attribute->data)[0]->data;
    const int num_name = ((pl_object *) x->attribute->data)[0]->length;

    pl_misc_for_i(num_name) {
        // Check if the length of the attribute name
        // matches with the length of the given name
        if (name_array[i].length == name->length) {
            char *const this_name = name_array[i].data;
            int flag = 0;

            // Compare the matched length name character by character
            pl_misc_for_j(name->length) {
                flag = flag && this_name[j] == input_array[j];
            }
            if (flag == 1)
                return i;
        }
    }

    return -1;
}

static pl_object has_attribute(pl_object x, pl_object name) {
    int result = primitive_index_attribute(x, name) != -1;
    return primitive_new_from_array(PL_CLASS_INT, 1, &result);
}

static pl_object get_attribute(pl_object x, pl_object name) {
    const int index = primitive_index_attribute(x, name);
    if (index == -1) {
        char name_null[128] = "";
        memcpy(name_null, name->data, 128);
        pl_error_throw(PL_ERROR_ATTRIBUTE_NOT_FOUND,
                       "Can not find attribute [%s]!",
                       name_null);
    }
    pl_object attributes = ((pl_object *) x->attribute->data)[1];
    return extract(attributes, primitive_new_from_array(PL_CLASS_INT, 1, &index));
}

static void set_attribute(pl_object x, pl_object name, pl_object item) {
    const int index = primitive_index_attribute(x, name);

    // If the attribute name is not found.
    if (index == -1) {
        // If the attribute field is empty, init it.
        if (x->attribute == NULL) {
            x->attribute = primitive_new(PL_CLASS_LIST, 2);
            pl_object attribute_names = primitive_new(PL_CLASS_LIST, 1);
            pl_object attributes = primitive_new(PL_CLASS_LIST, 1);
            ((pl_object *) x->attribute->data)[0] = attribute_names;
            ((pl_object *) x->attribute->data)[1] = attributes;
            x->attribute->length = 2;
        }

        pl_object attribute_names = ((pl_object *) x->attribute->data)[0];
        pl_object attributes = ((pl_object *) x->attribute->data)[1];
        pl_error_try {
                    // Append the name to the attribute names
                    append(attribute_names, name);

                    // Append the item to the attributes
                    append(attributes, item);
                } pl_error_catch {
            // If failed, removed the appended item.
            if (attribute_names->length != attributes->length) {
                const int smaller_length = attribute_names->length > attributes->length ?
                                           attributes->length :
                                           attribute_names->length;
                attribute_names->length = smaller_length;
                attributes->length = smaller_length;
            }

            pl_error_rethrow();
        }
    } else {
        // Replace the item with the provided item
        pl_object attributes = ((pl_object *) x->attribute->data)[1];
        ((pl_object *) attributes)[index] = item;
    }
}

static void remove_attribute(pl_object x, pl_object name) {
    const int index = primitive_index_attribute(x, name);
    if (index != -1) {
        pl_object attribute_names = ((pl_object *) x->attribute->data)[0];
        pl_object attributes = ((pl_object *) x->attribute->data)[1];
        primitive_remove(attribute_names, index, index);
        primitive_remove(attributes, index, index);
    }
}


/*-----------------------------------------------------------------------------
 |  Get object namespace
 ----------------------------------------------------------------------------*/

pl_object_ns pl_object_get_ns(void) {
    static const pl_object_ns object_ns = {
            .primitive = {
                    .new               = primitive_new,
                    .new_from_array    = primitive_new_from_array,
                    .new_from_variadic = primitive_new_from_variadic,
                    .to_array          = primitive_to_array,
                    .reserve           = primitive_reserve,
                    .shrink            = primitive_shrink,
                    .set_char          = primitive_set_char,
                    .set_int           = primitive_set_int,
                    .set_long          = primitive_set_long,
                    .set_double        = primitive_set_double,
                    .set_object        = primitive_set_object,
                    .set_external      = primitive_set_external,
                    .set_by_indices    = primitive_set_by_indices,
                    .set_range         = primitive_set_range,
                    .set_by_bool       = primitive_set_by_bool,
                    .extract_char      = primitive_extract_char,
                    .extract_int       = primitive_extract_int,
                    .extract_long      = primitive_extract_long,
                    .extract_double    = primitive_extract_double,
                    .extract_object    = primitive_extract_object,
                    .extract_external  = primitive_extract_external,
                    .extend_char       = primitive_extend_char,
                    .extend_int        = primitive_extend_int,
                    .extend_long       = primitive_extend_long,
                    .extend_double     = primitive_extend_double,
                    .extend_object     = primitive_extend_object,
                    .extend_external   = primitive_extend_external,
                    .subset            = primitive_subset,
                    .subset_exclude    = primitive_subset_exclude,
                    .subset_by_bool    = primitive_subset_by_bool,
                    .remove            = primitive_remove,
                    .remove_by_indices = primitive_remove_by_indices},
            .new            = new,
            .reserve        = reserve,
            .set            = set,
            .set_range      = set_range,
            .append         = append,
            .extract        = extract,
            .extend         = extend,
            .subset         = subset,
            .subset_exclude = subset_exclude,
            .copy           = copy,
            .equal          = equal,
            .in             = in,
            .print_set_decimals = print_set_decimals,
            .print          = print,
            .as_char        = as_char,
            .as_int         = as_int,
            .as_long        = as_long,
            .as_double      = as_double,
            .attribute = {
                    .has    = has_attribute,
                    .get    = get_attribute,
                    .set    = set_attribute,
                    .remove = remove_attribute}};

    return object_ns;
}
