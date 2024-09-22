//
// Created by Patrick Li on 1/7/2023.
//

#include "pl_gc.h"
#include "pl_class.h"
#include "pl_error.h"
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

// A global pointer to the vector table.
// It needs to be initialized by `gc_init` before use.
static pl_object global_directly_reachable  = NULL;
static pl_object global_reachable           = NULL;
static pl_object global_reachable_unordered = NULL;
static pl_object global_table               = NULL;

static void delete_object(pl_object x);
static void init_table(pl_object *table_p);
static void table_record_object(pl_object table, pl_object x);

#define check_null_pointer(x) pl_error_expect((x) != NULL,                      \
                                              PL_ERROR_UNEXPECTED_NULL_POINTER, \
                                              "Unexpected NULL pointer `" #x "` provided!")

#define check_missing_value(x) pl_error_expect(!pl_is_na(x),        \
                                               PL_ERROR_INVALID_NA, \
                                               "Unexpected missing value `" #x "` !")

/*-----------------------------------------------------------------------------
 |  Object memory management
 ----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 |  New object
 ----------------------------------------------------------------------------*/

static pl_object new_object_without_recording(const int class, const int capacity)
{
    check_missing_value(class);
    check_missing_value(capacity);
    pl_error_expect(class >= 0 && class < PL_NUM_CLASS,
                    PL_ERROR_UNDEFINED_CLASS,
                    "Undefined class [%d]!", class);

    pl_error_expect(capacity > 0 && capacity <= PL_OBJECT_MAX_CAPACITY,
                    PL_ERROR_INVALID_CAPACITY,
                    "Invalid capacity [%d]!", capacity);

    // Allocate memory for the object and the data.
    void *object_mem      = NULL;
    void *object_data_mem = NULL;
    object_mem            = malloc(sizeof(pl_object_struct));
    object_data_mem       = malloc(pl_object_data_size(class, capacity));

    pl_error_try
    {
        pl_error_expect(object_mem != NULL, PL_ERROR_ALLOC_FAILED, "`malloc()` fails!");
        pl_error_expect(object_data_mem != NULL, PL_ERROR_ALLOC_FAILED, "`malloc()` fails!");
    }
    pl_error_catch
    {
        free(object_mem);
        free(object_data_mem);
        pl_error_rethrow();
    }

    pl_object object = object_mem;

    // Prepare the metadata struct.
    pl_object_struct object_struct_copy = {.class     = class,
                                           .capacity  = capacity,
                                           .length    = 0,
                                           .attribute = NULL,
                                           .data      = object_data_mem};

    // Copy it to the allocated metadata memory.
    memcpy(object, &object_struct_copy, sizeof(pl_object_struct));

    return object;
}


static pl_object new_object(const int class, const int capacity)
{
    // Init the global table.
    init_table(&global_table);

    // Create a new object.
    pl_object object = new_object_without_recording(class, capacity);

    //  Try to record the object.
    pl_error_try
    {
        table_record_object(global_table, object);
    }
    pl_error_catch
    {
        // If the object fails to be recorded, delete the object.
        delete_object(object);
        pl_error_rethrow();
    }

    return object;
}

/*-----------------------------------------------------------------------------
 |  Resize object
 ----------------------------------------------------------------------------*/

static void resize_object(pl_object x, const int capacity)
{
    check_null_pointer(x);
    check_missing_value(capacity);
    pl_error_expect(capacity > 0 && capacity <= PL_OBJECT_MAX_CAPACITY,
                    PL_ERROR_INVALID_CAPACITY,
                    "Invalid capacity [%d]!",
                    capacity);

    // Realloc memory block for the data.
    void *data_mem = realloc(x->data, pl_object_data_size(x->class, capacity));
    pl_error_expect(data_mem != NULL, PL_ERROR_ALLOC_FAILED, "`realloc()` fails!");
    x->data = data_mem;
    x->capacity = capacity;

    // Some data will be lost if the requested capacity is smaller than the current length.
    if (x->capacity < x->length)
        x->length = x->capacity;
}

/*-----------------------------------------------------------------------------
 |  Reserve memory for object
 ----------------------------------------------------------------------------*/

static void reserve_object(pl_object x, const int capacity)
{
    check_null_pointer(x);
    check_missing_value(capacity);
    pl_error_expect(capacity > 0 && capacity < PL_OBJECT_MAX_CAPACITY,
                    PL_ERROR_INVALID_CAPACITY,
                    "Invalid capacity [%d]!",
                    capacity);

    // If there is enough memory.
    if (x->capacity >= capacity)
        return;

    // Switch to linear growth at some point.
    int final_capacity  = 1;
    const int threshold = 2 << (sizeof(int) * 4);
    while (final_capacity <= capacity)
    {
        if (final_capacity < threshold)
            final_capacity *= 2;
        else
            final_capacity += threshold;
    }
    if (final_capacity > PL_OBJECT_MAX_CAPACITY)
        final_capacity = PL_OBJECT_MAX_CAPACITY - 1;

    // Resize the object to the final capacity.
    resize_object(x, final_capacity);
}


/*-----------------------------------------------------------------------------
 |  Delete object
 ----------------------------------------------------------------------------*/

static void delete_object(pl_object x)
{
    check_null_pointer(x);
    free(x->data);
    free(x);
}

/*-----------------------------------------------------------------------------
 |  Table operation
 ----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 |  Init table
 ----------------------------------------------------------------------------*/

static void init_table(pl_object *const table_p)
{
    // Allocate memory for the table if it is uninitialized.
    if (*table_p == NULL)
        *table_p = new_object_without_recording(PL_CLASS_LIST, 8);
}

/*-----------------------------------------------------------------------------
 |  Find object
 ----------------------------------------------------------------------------*/

// Find an object from a table, -1 for not found.
static int table_find_object(pl_object table, pl_object x)
{
    if (x == NULL)
        return -1;

    int head    = 0;
    int tail    = table->length - 1;
    int current = 0;

    pl_object *const data_array = table->data;

    // Use binary search to find the object.
    while (head <= tail)
    {
        current = head / 2 + tail / 2 + (tail % 2 && head % 2);

        if (data_array[current] == x)
            return current;

        if (data_array[current] < x)
            head = current + 1;
        else
            tail = current - 1;
    }

    return -1;
}

/*-----------------------------------------------------------------------------
 |  Record object
 ----------------------------------------------------------------------------*/

static void table_record_object(pl_object table, pl_object x)
{
    check_null_pointer(x);

    // Do nothing if the object is already recorded.
    if (table_find_object(table, x) != -1)
        return;

    // Reserve enough space for the table.
    reserve_object(table, table->length + 1);

    // Define a data array for convenience.
    pl_object *const data_array = table->data;

    // If the table is empty, there is no need to memmove.
    // Otherwise, find the correct location to insert the object.
    if (table->length == 0)
    {
        table->length = 1;
        data_array[0] = x;
    }
    else
    {
        int head    = 0;
        int tail    = table->length - 1;
        int current = 0;

        while (head <= tail)
        {
            current = head / 2 + tail / 2 + (tail % 2 && head % 2);
            if (data_array[current] < x)
                head = current + 1;
            else
                tail = current - 1;
        }

        if (head == table->length)
        {
            data_array[table->length] = x;
        }
        else
        {
            memmove(data_array + head + 1,
                    data_array + head,
                    pl_object_data_size(PL_CLASS_LIST, table->length - head));
            data_array[head] = x;
        }

        table->length++;
    }
}

/*-----------------------------------------------------------------------------
 |  Untrack object
 ----------------------------------------------------------------------------*/

static void table_untrack_object(pl_object table, pl_object x)
{
    check_null_pointer(x);

    // Do nothing if the object is not in the table.
    int index = table_find_object(table, x);
    if (index == -1)
        return;

    // Define a data array for convenience.
    pl_object *const data_array = table->data;

    // If it is the last item.
    if (index == table->length - 1)
    {
        table->length--;
        return;
    }

    // Otherwise, move the data.
    memmove(data_array + index,
            data_array + index + 1,
            pl_object_data_size(PL_CLASS_LIST, table->length - index - 1));
    table->length--;
}

/*-----------------------------------------------------------------------------
 |  Directly reachable
 ----------------------------------------------------------------------------*/

static void directly_reachable(pl_object x)
{
    // Init the table.
    init_table(&global_directly_reachable);

    if (x != NULL)
        table_record_object(global_directly_reachable, x);
}

/*-----------------------------------------------------------------------------
 |  Multiple directly reachable
 ----------------------------------------------------------------------------*/

static void multiple_directly_reachable(const int length, ...)
{
    // Init the table.
    init_table(&global_directly_reachable);

    if (length <= 0)
        return;

    va_list ap;
    va_start(ap, length);
    pl_misc_for_i(length)
    {
        pl_object x = va_arg(ap, pl_object);
        if (x != NULL)
            table_record_object(global_directly_reachable, x);
    }
    va_end(ap);
}

/*-----------------------------------------------------------------------------
 |  Directly unreachable
 ----------------------------------------------------------------------------*/

static void directly_unreachable(pl_object x)
{
    // Init the table.
    init_table(&global_directly_reachable);

    if (x != NULL)
        table_untrack_object(global_directly_reachable, x);
}

/*-----------------------------------------------------------------------------
 |  Multiple directly unreachable
 ----------------------------------------------------------------------------*/

static void multiple_directly_unreachable(const int length, ...)
{
    // Init the table.
    init_table(&global_directly_reachable);

    if (length <= 0)
        return;

    va_list ap;
    va_start(ap, length);
    pl_misc_for_i(length)
    {
        pl_object x = va_arg(ap, pl_object);
        if (x != NULL)
            table_untrack_object(global_directly_reachable, x);
    }
    va_end(ap);
}

/*-----------------------------------------------------------------------------
 |  Garbage collect
 ----------------------------------------------------------------------------*/

static void update_reachable(void)
{
    // Init the reachable and directly reachable.
    init_table(&global_reachable);
    init_table(&global_reachable_unordered);
    init_table(&global_directly_reachable);

    // Reset global reachable
    global_reachable->length = 0;

    // Do nothing if the global directly reachable has no objects.
    if (global_directly_reachable->length == 0)
        return;

    // Resize global reachable and global reachable unordered.
    resize_object(global_reachable, global_directly_reachable->length);
    resize_object(global_reachable_unordered, global_directly_reachable->length);

    // Copy all the directly reachable to unordered reachable as a starting point.
    memcpy(global_reachable_unordered->data,
           global_directly_reachable->data,
           pl_object_data_size(PL_CLASS_LIST, global_directly_reachable->length));
    global_reachable_unordered->length = global_directly_reachable->length;

    // Also make a copy to reachable.
    memcpy(global_reachable->data,
           global_directly_reachable->data,
           pl_object_data_size(PL_CLASS_LIST, global_directly_reachable->length));
    global_reachable->length = global_directly_reachable->length;

    // Use BFS to find all reachable objects.

    // Define the head and the tail.
    int head_index = 0;
    int tail_index = global_directly_reachable->length - 1;

    // Define a data array for convenience. This pointer needs to be updated if the object is reallocated.
    pl_object *reachable_unordered_array = global_reachable_unordered->data;

    const pl_class_ns class_ns = pl_class_get_ns();
    while (head_index <= tail_index)
    {
        // Get the head item.
        pl_object head_item = reachable_unordered_array[head_index];

        // No need to explore types other than list.
        if (class_ns.type(head_item->class) == PL_CLASS_LIST)
        {
            // Define an array for convenience.
            pl_object *const vectors = head_item->data;

            // Check each item in the array.
            pl_misc_for_i(head_item->length)
            {
                // Null item should be ignored.
                if (vectors[i] != NULL)
                {
                    // Try to record it by the reachable table
                    int before_length = global_reachable->length;
                    table_record_object(global_reachable, vectors[i]);

                    // If success (no repeated item), append it to the end of the reachable unordered.
                    if (global_reachable->length > before_length)
                    {
                        // Reserve enough space for the reachable unordered.
                        reserve_object(global_reachable_unordered, global_reachable_unordered->length + 1);

                        // Update the data array since the object is reallocated.
                        reachable_unordered_array = global_reachable_unordered->data;

                        // Store the object at the end of the array.
                        reachable_unordered_array[global_reachable_unordered->length] = vectors[i];
                        global_reachable_unordered->length++;
                    }
                }
            }
        }

        // If the attribute presents, record it.
        if (head_item->attribute != NULL)
        {
            // Try to record it by the reachable table.
            int before_length = global_reachable->length;
            table_record_object(global_reachable, head_item->attribute);

            // If success (no repeated item), append it to the end of the reachable unordered.
            if (global_reachable->length > before_length)
            {
                // Reserve enough space for the reachable unordered.
                reserve_object(global_reachable_unordered, global_reachable_unordered->length + 1);

                // Update the data array since the object is reallocated.
                reachable_unordered_array = global_reachable_unordered->data;

                // Store the object at the end of the array.
                reachable_unordered_array[global_reachable_unordered->length] = head_item->attribute;
                global_reachable_unordered->length++;
            }
        }

        // Update the tail index and the head index
        tail_index = global_reachable_unordered->length - 1;
        head_index++;
    }
}


#include <malloc/malloc.h>

static void garbage_collect(void)
{
    // Init all the tables.
    init_table(&global_table);
    init_table(&global_directly_reachable);
    init_table(&global_reachable);

    // Update the reachable table.
    update_reachable();

    // Delete all the objects not presented in the reachable.
    pl_object *const vectors = global_table->data;
    pl_misc_for_i(global_table->length)
    {
        if (table_find_object(global_reachable, vectors[i]) == -1)
            delete_object(vectors[i]);
    }

    // Copy all the objects from the reachable to the global table.
    memcpy(global_table->data,
           global_reachable->data,
           (size_t) global_reachable->length * sizeof(pl_object));
    global_table->length = global_reachable->length;

    // Resize the global table to save some space.
    resize_object(global_table, global_table->length == 0 ? 1 : global_table->length);
}

/*-----------------------------------------------------------------------------
 |  Report memory usage
 ----------------------------------------------------------------------------*/

static void report(void)
{
    // Init the global table
    init_table(&global_table);

    // Define a data array for convenience.
    pl_object *const data_array = global_table->data;

    // Calculate the size of all the objects.
    size_t total_size = 0;
    pl_misc_for_i(global_table->length)
    {
        total_size += pl_object_size(data_array[i]);
    }

    // Print the report.
    printf("Object table summary:\n[Capacity = %d, Length = %d, Heap Memory usage = %zu bytes]\n",
           global_table->capacity,
           global_table->length,
           total_size);

    printf("\t│ Object     │ %-16s │ %-8s │ %-10s │ %-12s │ %s │ %s │\n",
           "Address",
           "Class",
           "Length",
           "Element size",
           "Base size",
           "Total size");
    pl_misc_for_i(global_table->length)
    {
        printf("\t│ %-10d │ <%p> │ %-8s │ %-10d │ %-1lu Bytes      │ %lu Bytes  │ %lu Bytes   │\n",
               i,
               (const void *) data_array[i],
               PL_CLASS_NAME[data_array[i]->class],
               data_array[i]->length,
               PL_CLASS_ELEMENT_SIZE[data_array[i]->class],
               sizeof(pl_object_struct),
               pl_object_size(data_array[i]));
    }
    puts("");
}

/*-----------------------------------------------------------------------------
 |  Kill the garbage collector
 ----------------------------------------------------------------------------*/

static void kill(void)
{
    pl_object *const vectors = global_table->data;
    pl_misc_for_i(global_table->length)
    {
        delete_object(vectors[i]);
    }

    delete_object(global_directly_reachable);
    delete_object(global_reachable);
    delete_object(global_reachable_unordered);
    delete_object(global_table);

    global_directly_reachable  = NULL;
    global_reachable           = NULL;
    global_reachable_unordered = NULL;
    global_table               = NULL;
}

/*-----------------------------------------------------------------------------
 |  Check garbage collector status
 ----------------------------------------------------------------------------*/

static int check_status(void)
{
    return global_table != NULL;
}

/*-----------------------------------------------------------------------------
 |  Get garbage collector namespace
 ----------------------------------------------------------------------------*/

pl_gc_ns pl_gc_get_ns(void)
{
    static const pl_gc_ns gc_ns = {.new_object                    = new_object,
                                   .resize_object                 = resize_object,
                                   .reserve_object                = reserve_object,
                                   .directly_reachable            = directly_reachable,
                                   .multiple_directly_reachable   = multiple_directly_reachable,
                                   .directly_unreachable          = directly_unreachable,
                                   .multiple_directly_unreachable = multiple_directly_unreachable,
                                   .garbage_collect               = garbage_collect,
                                   .report                        = report,
                                   .kill                          = kill,
                                   .check_status                  = check_status};
    return gc_ns;
}
