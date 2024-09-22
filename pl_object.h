//
// Created by Patrick Li on 1/7/2023.
//

#ifndef PL_PL_OBJECT_H
#define PL_PL_OBJECT_H

#include "limits.h"
#include "math.h"
#include "pl_class.h"
#include "pl_error.h"
#include "pl_misc.h"

/*-----------------------------------------------------------------------------
 |  Object definition
 ----------------------------------------------------------------------------*/


typedef struct pl_object_struct pl_object_struct;

/// An object.
/// @param class (int). Class of the object.
/// @param capacity (int). Capacity of the object.
/// @param length (int). Length of the object.
/// @param placeholder (int). A reserved space for future use.
/// @param attribute (pl_object). Additional attributes.
/// @param data (void *). A data container.
typedef pl_object_struct *pl_object;
struct pl_object_struct {
    const int class;
    int capacity;
    int length;
    int placeholder;
    pl_object attribute;
    void *data;
};


/*-----------------------------------------------------------------------------
 |  Size of the object
 ----------------------------------------------------------------------------*/

/// Size of the data.
#define pl_object_data_size(class, capacity) (((size_t) (capacity)) * PL_CLASS_ELEMENT_SIZE[class])

/// Expected size of an object
#define pl_object_expected_size(class, capacity) (sizeof(pl_object_struct) + pl_object_data_size(class, capacity))

/// Size of an object.
#define pl_object_size(x) pl_object_expected_size((x)->class, (x)->capacity)

/*-----------------------------------------------------------------------------
 |  Capacity and NA definition
 ----------------------------------------------------------------------------*/

#define PL_OBJECT_MAX_CAPACITY (2 << (sizeof(int) * 7))

#define PL_CHAR_NA 0
#define PL_INT_NA INT_MAX
#define PL_LONG_NA LONG_MAX
#define PL_DOUBLE_NA ((double) NAN)
#define PL_LIST_NA NULL
#define PL_EXTERNAL_NA NULL

#define pl_is_na(x) _Generic(x, char                     \
                             : (x) == PL_CHAR_NA, int    \
                             : (x) == PL_INT_NA, long    \
                             : (x) == PL_LONG_NA, double \
                             : (x) != (x), pl_object     \
                             : (x) == 0, void *          \
                             : (x) == 0, const void *    \
                             : (x) == 0)

/*-----------------------------------------------------------------------------
 |  Shortcuts for object creation
 ----------------------------------------------------------------------------*/

#ifdef PL_OBJECT_SHORTCUTS

/// New a local char vector.
/// @details This vector and its content will be automatically destroyed at the end of the scope.
/// It should not be referenced by any other objects.
/// @param ... Items need to be stored by the object.
/// @return A new object.
#define $local_c_char(...) (pl_object) (&(pl_object_struct){.class     = PL_CLASS_CHAR,                                    \
                                                            .capacity  = pl_misc_count_arg(__VA_ARGS__),                   \
                                                            .length    = pl_misc_count_arg(__VA_ARGS__),                   \
                                                            .attribute = NULL,                                             \
                                                            .data      = (char[pl_misc_count_arg(__VA_ARGS__)]){__VA_ARGS__}})

/// New a local int vector.
/// @details This vector and its content will be automatically destroyed at the end of the scope.
/// It should not be referenced by any other objects.
/// @param ... Items need to be stored by the object.
/// @return A new object.
#define $local_c_int(...) (pl_object) (&(pl_object_struct){.class     = PL_CLASS_INT,                                     \
                                                           .capacity  = pl_misc_count_arg(__VA_ARGS__),                   \
                                                           .length    = pl_misc_count_arg(__VA_ARGS__),                   \
                                                           .attribute = NULL,                                             \
                                                           .data      = (int[pl_misc_count_arg(__VA_ARGS__)]){__VA_ARGS__}})

/// New a local long vector.
/// @details This vector and its content will be automatically destroyed at the end of the scope.
/// It should not be referenced by any other objects.
/// @param ... Items need to be stored by the object.
/// @return A new object.
#define $local_c_long(...) (pl_object) (&(pl_object_struct){.class     = PL_CLASS_LONG,                                    \
                                                            .capacity  = pl_misc_count_arg(__VA_ARGS__),                   \
                                                            .length    = pl_misc_count_arg(__VA_ARGS__),                   \
                                                            .attribute = NULL,                                             \
                                                            .data      = (long[pl_misc_count_arg(__VA_ARGS__)]){__VA_ARGS__}})

/// New a local double vector.
/// @details This vector and its content will be automatically destroyed at the end of the scope.
/// It should not be referenced by any other objects.
/// @param ... Items need to be stored by the object.
/// @return A new object.
#define $local_c_double(...) (pl_object) (&(pl_object_struct){.class     = PL_CLASS_DOUBLE,                                  \
                                                              .capacity  = pl_misc_count_arg(__VA_ARGS__),                   \
                                                              .length    = pl_misc_count_arg(__VA_ARGS__),                   \
                                                              .attribute = NULL,                                             \
                                                              .data      = (double[pl_misc_count_arg(__VA_ARGS__)]){__VA_ARGS__}}))

/// New a local external vector.
/// @details This vector and its content will be automatically destroyed at the end of the scope.
/// It should not be referenced by any other objects.
/// @param ... Items need to be stored by the object.
/// @return A new object.
#define $local_c_external(...) (pl_object) (&(pl_object_struct){.class     = PL_CLASS_EXTERNAL,                                \
                                                                .capacity  = pl_misc_count_arg(__VA_ARGS__),                   \
                                                                .length    = pl_misc_count_arg(__VA_ARGS__),                   \
                                                                .attribute = NULL,                                             \
                                                                .data      = (void *[pl_misc_count_arg(__VA_ARGS__)]){__VA_ARGS__}}))


/// New a char vector.
/// @param ... Items need to be stored by the object.
/// @return A new object.
#define $c_char(...) pl.object.primitive.new_from_array(PL_CLASS_CHAR, pl_misc_count_arg(__VA_ARGS__), (char[pl_misc_count_arg(__VA_ARGS__)]){__VA_ARGS__})

/// New a int vector.
/// @param ... Items need to be stored by the object.
/// @return A new object.
#define $c_int(...) pl.object.primitive.new_from_array(PL_CLASS_INT, pl_misc_count_arg(__VA_ARGS__), (int[pl_misc_count_arg(__VA_ARGS__)]){__VA_ARGS__})

/// New a long vector.
/// @param ... Items need to be stored by the object.
/// @return A new object.
#define $c_long(...) pl.object.primitive.new_from_array(PL_CLASS_LONG, pl_misc_count_arg(__VA_ARGS__), (long[pl_misc_count_arg(__VA_ARGS__)]){__VA_ARGS__})

/// New a double vector.
/// @param ... Items need to be stored by the object.
/// @return A new object.
#define $c_double(...) pl.object.primitive.new_from_array(PL_CLASS_DOUBLE, pl_misc_count_arg(__VA_ARGS__), (double[pl_misc_count_arg(__VA_ARGS__)]){__VA_ARGS__})

/// New a external vector.
/// @param ... Items need to be stored by the object.
/// @return A new object.
#define $c_external(...) pl.object.primitive.new_from_array(PL_CLASS_EXTERNAL, pl_misc_count_arg(__VA_ARGS__), (void *[pl_misc_count_arg(__VA_ARGS__)]){__VA_ARGS__})

/// New a list.
/// @param ... Items need to be stored by the object.
/// @return A new object.
#define $list(...) pl.object.primitive.new_from_array(PL_CLASS_LIST, pl_misc_count_arg(__VA_ARGS__), (pl_object[pl_misc_count_arg(__VA_ARGS__)]){__VA_ARGS__})

#endif//PL_OBJECT_SHORTCUTS

/*-----------------------------------------------------------------------------
 |  Object namespace
 ----------------------------------------------------------------------------*/

/// Object namespace
typedef struct pl_object_ns {
    /// Namespace for primitive functions.
    struct pl_object_primitive_ns {

        /*-----------------------------------------------------------------------------
         |  Primitive new
         ----------------------------------------------------------------------------*/

        /// New an object.
        /// @param class (int). Class of the object.
        /// @param capacity (int). Capacity of the object.
        /// @return A new empty object.
        pl_object (*const new)(int class, int capacity);

        /// New an object using an array.
        /// @details If `length` is zero, an empty object will be returned. The `array`
        /// will not be used.
        /// @param class (int). Class of the object.
        /// @param length (int). Length of the object.
        /// @param array (const void *). The source array.
        /// @return A new object.
        pl_object (*const new_from_array)(int class, int length, const void *array);

        /// New an object using variadic arguments.
        /// @details If `length` is zero, an empty object will be returned.
        /// @param class (int). Class of the object.
        /// @param length (int). Number of items will be provided.
        /// @param ... Items need to be stored by the object.
        /// @return A new object.
        pl_object (*const new_from_variadic)(int class, int length, ...);

        /*-----------------------------------------------------------------------------
         |  Primitive to array
         ----------------------------------------------------------------------------*/

        /// Copy an object to an array.
        /// @details This is a shallow copy.
        /// @param x (pl_object). The object.
        /// @param array (void *). Destination array.
        void (*const to_array)(pl_object x, void *array);

        /*-----------------------------------------------------------------------------
         |  Primitive Reserve
         ----------------------------------------------------------------------------*/

        /// Reserve memory for an object.
        /// @details The function may reserve more memory than the requested
        /// amount for efficiency.
        /// @param x (pl_object). The object.
        /// @param capacity (int). New capacity.
        void (*const reserve)(pl_object x, int capacity);

        /*-----------------------------------------------------------------------------
         |  Primitive shrink
         ----------------------------------------------------------------------------*/

        /// Shrink an object to a desired capacity.
        /// @details Data may be lost due to the shrink.
        /// @param x (pl_object). The object.
        /// @param capacity (int). The desired capacity.
        void (*const shrink)(pl_object x, int capacity);

        /*-----------------------------------------------------------------------------
         |  Primitive set
         ----------------------------------------------------------------------------*/

        /// Set value for an object.
        /// @details If `index` is NA, the value will not be set.
        /// @param x (pl_object). An object of type PL_CLASS_CHAR.
        /// @param index (int). The index.
        /// @param item (char). The new value.
        void (*const set_char)(pl_object x, int index, char item);

        /// Set value for an object.
        /// @details If `index` is NA, the value will not be set.
        /// @param x (pl_object). An object of type PL_CLASS_INT.
        /// @param index (int). The index.
        /// @param item (int). The new value.
        void (*const set_int)(pl_object x, int index, int item);

        /// Set value for an object.
        /// @details If `index` is NA, the value will not be set.
        /// @param x (pl_object). An object of type PL_CLASS_LONG.
        /// @param index (int). The index.
        /// @param item (long). The new value.
        void (*const set_long)(pl_object x, int index, long item);

        /// Set value for an object.
        /// @details If `index` is NA, the value will not be set.
        /// @param x (pl_object). An object of type PL_CLASS_DOUBLE.
        /// @param index (int). The index.
        /// @param item (double). The new value.
        void (*const set_double)(pl_object x, int index, double item);

        /// Set value for an object.
        /// @details If `index` is NA, the value will not be set.
        /// @param x (pl_object). An object of type PL_CLASS_LIST.
        /// @param index (int). The index.
        /// @param item (pl_object). The new value.
        void (*const set_object)(pl_object x, int index, pl_object item);

        /// Set value for an object.
        /// @details If `index` is NA, the value will not be set.
        /// @param x (pl_object). An object of type PL_CLASS_LIST.
        /// @param index (int). The index.
        /// @param item (void *). The new value.
        void (*const set_external)(pl_object x, int index, void *item);

        /// Set values for an object by indices.
        /// @details If an index of `indices` is NA, the corresponding value will not be set.
        /// If `length` is zero, no items will be set and `array` will not be used.
        /// @param x (pl_object). The object.
        /// @param length (int). Length of the array.
        /// @param indices (const int *). The indices.
        /// @param array (const void *). The source array.
        void (*const set_by_indices)(pl_object x, const int length, const int *indices, const void *array);

        /// Set a range of items for an object.
        /// @param x (pl_object). The object.
        /// @param start (int). The first index.
        /// @param end (int). The last index.
        /// @param array (const void *). The source array.
        void (*const set_range)(pl_object x, const int start, const int end, const void *array);

        /// Set values for an object by a Boolean array. The boolean array
        /// should not contain any NA values.
        /// @param x (pl_object). The object.
        /// @param bool_array (const int *). The Boolean array.
        /// @param array (const void *). The source array.
        void (*const set_by_bool)(pl_object x, const int *bool_array, const void *array);

        /*-----------------------------------------------------------------------------
         |  Primitive extract
         ----------------------------------------------------------------------------*/

        /// Extract value of an object.
        /// @details If `index` is NA, returns NA.
        /// @param x (pl_object). An object of type PL_CLASS_CHAR.
        /// @param index (int). The index.
        /// @return A char.
        char (*const extract_char)(pl_object x, int index);

        /// Extract value of an object.
        /// @details If `index` is NA, returns NA.
        /// @param x (pl_object). An object of type PL_CLASS_INT.
        /// @param index (int). The index.
        /// @return A int.
        int (*const extract_int)(pl_object x, int index);

        /// Extract value of an object.
        /// @details If `index` is NA, returns NA.
        /// @param x (pl_object). An object of type PL_CLASS_LONG.
        /// @param index (int). The index.
        /// @return A long.
        long (*const extract_long)(pl_object x, int index);

        /// Extract value of an object.
        /// @details If `index` is NA, returns NA.
        /// @param x (pl_object). An object of type PL_CLASS_DOUBLE.
        /// @param index (int). The index.
        /// @return A double.
        double (*const extract_double)(pl_object x, int index);

        /// Extract value of an object.
        /// @details If `index` is NA, returns NA.
        /// @details The attribute of the outer list will be dropped.
        /// @param x (pl_object). An object of type PL_CLASS_LIST.
        /// @param index (int). The index.
        /// @return A pl_object.
        pl_object (*const extract_object)(pl_object x, int index);

        /// Extract value of an object.
        /// @details If `index` is NA, returns NA.
        /// @details The attribute of the outer list will be dropped.
        /// @param x (pl_object). An object of type PL_CLASS_EXTERNAL.
        /// @param index (int). The index.
        /// @return A pointer.
        void *(*const extract_external)(pl_object x, int index);

        /*-----------------------------------------------------------------------------
         |  Primitive extend
         ----------------------------------------------------------------------------*/

        /// Extend an object by a value.
        /// @param x (pl_object). An object of type PL_CLASS_CHAR.
        /// @param item (char). The item.
        void (*const extend_char)(pl_object x, char item);

        /// Extend an object by a value.
        /// @param x (pl_object). An object of type PL_CLASS_INT.
        /// @param item (int). The item.
        void (*const extend_int)(pl_object x, int item);

        /// Extend an object by a value.
        /// @param x (pl_object). An object of type PL_CLASS_LONG.
        /// @param item (long). The item.
        void (*const extend_long)(pl_object x, long item);

        /// Extend an object by a value.
        /// @param x (pl_object). An object of type PL_CLASS_DOUBLE.
        /// @param item (double). The item.
        void (*const extend_double)(pl_object x, double item);

        /// Extend an object by a value.
        /// @param x (pl_object). An object of type PL_CLASS_LIST.
        /// @param item (pl_object). The item.
        void (*const extend_object)(pl_object x, pl_object item);

        /// Extend an object by a value.
        /// @param x (pl_object). An object of type PL_CLASS_EXTERNAL.
        /// @param item (void *). The item.
        void (*const extend_external)(pl_object x, void *item);

        /*-----------------------------------------------------------------------------
         |  Primitive subset
         ----------------------------------------------------------------------------*/

        /// Construct a new object as a subset of the original object.
        /// @details The attribute will be dropped. If an index of `indices` is NA,
        /// the corresponding item will be NA. If `length` is zero, an empty
        /// object will be returned.
        /// @param x (pl_object). The object.
        /// @param length (int). The length of the new object.
        /// @param indices (const int *). Indices.
        /// @return A new object.
        pl_object (*const subset)(pl_object x, int length, const int *indices);

        /// Construct a new object as a subset of the original object by excluding some indices.
        /// @details The attribute will be dropped. NAs in `indices` will be ignored.
        /// Duplicate indices will be ignored. If `length` is zero, a shallow
        /// copy of the object will be returned.
        /// @param x (pl_object). The object.
        /// @param length (int). The length of indices.
        /// @param indices (const int *). Indices that needs to be excluded.
        /// @return A new object.
        pl_object (*const subset_exclude)(pl_object x, int length, const int *indices);

        /// Construct a new object as a subset of the original object by a Boolean array.
        /// @param x (pl_object). The object.
        /// @param bool_array (const int *). An array of Booleans.
        /// @return A new object.
        pl_object (*const subset_by_bool)(pl_object x, const int *bool_array);

        /*-----------------------------------------------------------------------------
         |  Primitive remove
         ----------------------------------------------------------------------------*/

        /// Remove items from the object.
        /// @param x (pl_object). The object.
        /// @param start (int). The first index.
        /// @param end (int). The last index.
        void (*const remove)(pl_object x, int start, int end);

        /// Remove items from the object by indices.
        /// @details NAs in `indices` will be ignored. Duplicate indices will
        /// be ignored.
        /// @param x (pl_object). The object.
        /// @param length (int). Length of the array.
        /// @param indices (const int *). The indices.
        /// @param array (const void *). The array.
        void (*const remove_by_indices)(pl_object x, int length, const int *indices);

    } primitive;

    /// New an object.
    /// @param class (pl_object). Class of the object.
    /// @param capacity (pl_object). Capacity of the object.
    /// @return A new empty object.
    pl_object (*const new)(pl_object class, pl_object capacity);

    /// Reserve memory for an object.
    /// The function may reserve more memory than requested amount for efficiency.
    /// @param x (pl_object). The object.
    /// @param capacity (pl_object). New capacity.
    void (*const reserve)(pl_object x, pl_object capacity);

    /// Set one or more values of an object.
    /// @param x (pl_object). The object.
    /// @param indices (pl_object). Indices.
    /// @param items (pl_object). Items.
    void (*const set)(pl_object x, pl_object indices, pl_object items);

    /// Set values for a range of items of an object.
    /// @param x (pl_object). The object.
    /// @param start (pl_object). The start index.
    /// @param end (pl_object). The end index.
    /// @param items (pl_object). Items.
    void (*const set_range)(pl_object x, pl_object start, pl_object end, pl_object items);

    /// Append an item to the end of a list.
    /// @param x (pl_object). The object.
    /// @param item (pl_object). The item.
    void (*const append)(pl_object x, pl_object item);

    /// Extract a value of an object.
    /// @param x (pl_object). The object.
    /// @param index (pl_object). The index.
    /// @return If x is of type PL_CLASS_LIST,
    /// this function will return the value directly.
    /// Otherwise, the function will act the same as `subset`
    /// and drop the attribute.
    pl_object (*const extract)(pl_object x, pl_object index);

    /// Extend an object by another object.
    /// @param x (pl_object). The object.
    /// @param y (pl_object). Another object of the same type.
    void (*const extend)(pl_object x, pl_object y);

    /// Construct a new object as a subset of the original object.
    /// @details The attribute will be dropped.
    /// @param x (pl_object). The object.
    /// @param indices (pl_object). Indices.
    /// @return A new object.
    pl_object (*const subset)(pl_object x, pl_object indices);

    /// Construct a new object as a subset of the original object by excluding some indices.
    /// @details The attribute will be dropped. NAs in `indices` will be ignored.
    /// Duplicate indices will be ignored.
    /// @param x (pl_object). The object.
    /// @param indices (pl_object). Indices.
    /// @return A new object.
    pl_object (*const subset_exclude)(pl_object x, pl_object indices);

    /// Construct a shallow copy of an object.
    /// @details The attribute will be dropped.
    /// @param x (pl_object). The object.
    /// @return A new object.
    pl_object (*const copy)(pl_object x);

    /// Check if items in two objects are equal.
    /// @param x (pl_object). The object.
    /// @param y (pl_object). Another object.
    /// @return A new object.
    pl_object (*const equal)(pl_object x, pl_object y);

    /// Check if each element of x is in y.
    /// @param x (pl_object). The object.
    /// @param y (pl_object). Another object.
    /// @return A new object of type PL_CLASS_INT of
    /// the same length as x.
    pl_object (*const in)(pl_object x, pl_object y);

    /// Set number of decimals.
    /// @param x (pl_object). An int object.
    void (*const print_set_decimals)(pl_object x);

    /// Print an object.
    /// @param x (pl_object). The object.
    void (*const print)(pl_object x);

    /// Convert to a PL_CLASS_CHAR object.
    /// @details The attribute will be dropped.
    /// @param x (pl_object). The object.
    /// @return A new PL_CLASS_CHAR object.
    pl_object (*const as_char)(pl_object x);

    /// Convert to a PL_CLASS_INT object.
    /// @details The attribute will be dropped.
    /// @param x (pl_object). The object.
    /// @return A new PL_CLASS_INT object.
    pl_object (*const as_int)(pl_object x);

    /// Convert to a PL_CLASS_LONG object.
    /// @details The attribute will be dropped.
    /// @param x (pl_object). The object.
    /// @return A new PL_CLASS_LONG object.
    pl_object (*const as_long)(pl_object x);

    /// Convert to a PL_CLASS_DOUBLE object.
    /// @details The attribute will be dropped.
    /// @param x (pl_object). The object.
    /// @return A new PL_CLASS_DOUBLE object.
    pl_object (*const as_double)(pl_object x);

    struct pl_object_attribute_ns {
        /// Check if an attribute name exists.
        /// @param x (pl_object). The object.
        /// @param name (pl_object). The attribute name.
        /// @param A length one PL_CLASS_INT object.
        pl_object (*const has)(pl_object x, pl_object name);

        /// Get an attribute with its name.
        /// @param x (pl_object). The object.
        /// @param name (pl_object). The attribute name.
        /// @param The attribute.
        pl_object (*const get)(pl_object x, pl_object name);

        /// New or set an attribute.
        /// @param x (pl_object). The object.
        /// @param name (pl_object). The attribute name.
        /// @param item (pl_object). The item.
        void (*const set)(pl_object x, pl_object name, pl_object item);

        /// Remove an attribute with its name.
        /// @param x (pl_object). The object.
        /// @param name (pl_object). The attribute name.
        void (*const remove)(pl_object x, pl_object name);
    } attribute;

} pl_object_ns;

/// Get object namespace.
/// @return Namespace of object.
pl_object_ns pl_object_get_ns(void);

#endif//PL_PL_OBJECT_H
