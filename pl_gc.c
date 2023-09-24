//
// Created by Patrick Li on 1/7/2023.
//

#include "pl_gc.h"
#include "pl_class.h"
#include "pl_error.h"
#include "stdio.h"
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

#define check_null_pointer(x) pl_error_expect((x) != NULL,                   \
                                              PL_ERROR_INVALID_NULL_POINTER, \
                                              "Can't access NULL pointer `" #x "` !")

/*-----------------------------------------------------------------------------
 |  Object memory management
 ----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 |  New object
 ----------------------------------------------------------------------------*/

/// New an object without recording it by the garbage collector.
/// @param class (int). Class of the object.
/// @param capacity (int). Capacity of the object.
/// @return A new object.
/// @when_fails No side effects.
static pl_object new_object_without_recording(const int class, const int capacity)
{
    pl_error_expect(class >= 0 && class < PL_NUM_CLASS,
                    PL_ERROR_UNDEFINED_CLASS,
                    "Undefined class [%d]!",
                    class);

    pl_error_expect(capacity > 0 && capacity <= PL_OBJECT_MAX_CAPACITY,
                    PL_ERROR_INVALID_CAPACITY,
                    "Invalid capacity [%d]!",
                    capacity);

    // Allocate memory for the object and its data.
    volatile pl_object object_memory = NULL;
    volatile pl_object data_memory   = NULL;
    pl_error_try
    {
        object_memory = malloc(sizeof(pl_object_struct));
        data_memory   = malloc((size_t) capacity * PL_CLASS_ELEMENT_SIZE[class]);

        pl_error_expect(object_memory != NULL, PL_ERROR_ALLOC_FAIL, "`malloc()` fails!");
        pl_error_expect(data_memory != NULL, PL_ERROR_ALLOC_FAIL, "`malloc()` fails!");
    }
    pl_error_catch
    {
        // If the allocation fails, release the memory and rethrow the exception.
        free(object_memory);
        free(data_memory);
        pl_error_rethrow();
    }

    // Prepare the metadata struct.
    pl_object_struct object_struct_copy = {.class     = class,
                                           .capacity  = capacity,
                                           .length    = 0,
                                           .attribute = NULL,
                                           .data      = data_memory};

    // Copy it to the allocated metadata memory.
    memcpy(object_memory, &object_struct_copy, sizeof(pl_object_struct));

    return object_memory;
}


/// New an object.
/// @param class (int). Class of the object.
/// @param capacity (int). Capacity of the object.
/// @return A new object.
/// @when_fails No side effects.
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

/// Resize the object.
/// @details Data may be lost if the original length of the object is
/// greater than the original capacity.
/// @param x (pl_object). The object.
/// @param capacity (int). New capacity.
/// @when_fails No side effects.
static void resize_object(pl_object x, const int capacity)
{
    check_null_pointer(x);
    pl_error_expect(capacity > 0 && capacity <= PL_OBJECT_MAX_CAPACITY,
                    PL_ERROR_INVALID_CAPACITY,
                    "Invalid capacity [%d]!",
                    capacity);

    // Realloc memory block for the data.
    void *const memory = realloc(x->data,
                                 (size_t) capacity * PL_CLASS_ELEMENT_SIZE[x->class]);
    pl_error_expect(memory != NULL, PL_ERROR_ALLOC_FAIL, "`realloc()` fails!");

    // Prepare the metadata struct.
    // Copy it to the result metadata memory.
    pl_object_struct tmp_struct = {.class    = x->class,
                                   .capacity = capacity,
                                   .length   = x->length,
                                   .data     = memory};
    memcpy(x, &tmp_struct, sizeof(pl_object_struct));

    // Some data will be lost if the requested capacity is smaller than the current length.
    if (x->capacity < x->length)
        x->length = x->capacity;
}

/*-----------------------------------------------------------------------------
 |  Reserve memory for object
 ----------------------------------------------------------------------------*/

/// Reserve memory for an object.
/// @details The function may reserve more memory than the requested
/// amount for efficiency.
/// @param x (pl_object). The object.
/// @param capacity (int). New capacity.
/// @when_fails No side effects.
static void reserve_object(pl_object x, const int capacity)
{
    check_null_pointer(x);
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

    // Resize the object to the final capacity.
    resize_object(x, final_capacity);
}


/*-----------------------------------------------------------------------------
 |  Delete object
 ----------------------------------------------------------------------------*/

/// Delete an object.
/// @param x (pl_object). The object.
/// @when_fails No side effects.
static void delete_object(pl_object x)
{
    check_null_pointer(x);

    // Free both the struct and the data container.
    free(x->data);
    free(x);
}

/*-----------------------------------------------------------------------------
 |  Table operation
 ----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 |  Init table
 ----------------------------------------------------------------------------*/

/// Init a table.
/// @param table_p (pl_object *). Pointer to a table.
/// @when_fails No side effects.
static void init_table(pl_object *const table_p)
{
    // Allocate memory for the table if it is uninitialized.
    if (*table_p == NULL)
        *table_p = new_object_without_recording(PL_CLASS_LIST, 8);
}

/*-----------------------------------------------------------------------------
 |  Find object
 ----------------------------------------------------------------------------*/

/// Find an object in a table.
/// @param table (pl_object). The table.
/// @param x (pl_object). The object.
/// @return -1 if not found, otherwise returns the index.
static int table_find_object(pl_object table, pl_object x)
{
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

/// Record an object by a table.
/// @param table (pl_object). The table.
/// @param x (pl_object). The object.
/// @when_fails No side effects.
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
                    (size_t) (table->length - head) * PL_CLASS_ELEMENT_SIZE[PL_CLASS_LIST]);
            data_array[head] = x;
        }

        table->length++;
    }
}

/*-----------------------------------------------------------------------------
 |  Untrack object
 ----------------------------------------------------------------------------*/

/// Untrack an object by a table.
/// @param table (pl_object). The table.
/// @param x (pl_object). The object.
/// @when_fails No side effects.
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
            (size_t) (table->length - index - 1) * PL_CLASS_ELEMENT_SIZE[PL_CLASS_LIST]);
    table->length--;
}

/*-----------------------------------------------------------------------------
 |  Directly reachable
 ----------------------------------------------------------------------------*/

/// Declare a directly reachable object.
/// @param x (pl_object). The object.
/// @when_fails No external side effects.
static void directly_reachable(pl_object x)
{
    // Init the table.
    init_table(&global_directly_reachable);

    table_record_object(global_directly_reachable, x);
}

/*-----------------------------------------------------------------------------
 |  Directly unreachable
 ----------------------------------------------------------------------------*/

/// Declare a directly unreachable object.
/// @param x (pl_object). The object.
/// @when_fails No external side effects.
static void directly_unreachable(pl_object x)
{
    // Init the table.
    init_table(&global_directly_reachable);

    table_untrack_object(global_directly_reachable, x);
}

/*-----------------------------------------------------------------------------
 |  Garbage collect
 ----------------------------------------------------------------------------*/

/// Update the reachable table.
/// @when_fails No external side effects.
static void update_reachable(void)
{
    // Init the reachable and directly reachable.
    init_table(&global_reachable);
    init_table(&global_reachable_unordered);
    init_table(&global_directly_reachable);

    // Do nothing if the global directly reachable has no objects.
    if (global_directly_reachable->length == 0)
        return;

    // Resize global reachable and global reachable unordered.
    resize_object(global_reachable,
                  global_directly_reachable->length == 0 ? 1 : global_directly_reachable->length);
    resize_object(global_reachable_unordered,
                  global_directly_reachable->length == 0 ? 1 : global_directly_reachable->length);

    // Copy all the directly reachable to unordered reachable as a starting point.
    memcpy(global_reachable_unordered->data,
           global_directly_reachable->data,
           (size_t) global_directly_reachable->length * PL_CLASS_ELEMENT_SIZE[PL_CLASS_LIST]);
    global_reachable_unordered->length = global_directly_reachable->length;

    // Also make a copy to reachable.
    memcpy(global_reachable->data,
           global_directly_reachable->data,
           (size_t) global_directly_reachable->length * PL_CLASS_ELEMENT_SIZE[PL_CLASS_LIST]);
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

/// Run the garbage collector.
/// @when_fails Usually no external side effects.\n\n
/// Garbage collector may fail to shrink the container but successfully delete
/// unreachable objects with error code PL_ERROR_ALLOC_FAIL.\n\n
/// This happens when `realloc()` fails to allocate space for a new container.\n\n
/// In this case, the garbage collector still keeps all the reachable objects correctly.\n\n
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

/// Report the global table.
/// @when_fails No external side effects.
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
        total_size += PL_CLASS_ELEMENT_SIZE[data_array[i]->class] * (size_t) data_array[i]->capacity + sizeof(pl_object_struct);
    }

    // Print the report.
    printf("Object table summary:\n[Capacity = %d, Length = %d, Heap Memory usage = %zu bytes]\n",
           global_table->capacity,
           global_table->length,
           total_size);

    pl_misc_for_i(global_table->length)
    {
        printf("\tObject %4d <%p>: %-4s %3d Items, %lu Bytes\n",
               i,
               (const void *) data_array[i],
               PL_CLASS_NAME[data_array[i]->class],
               data_array[i]->length,
               PL_CLASS_ELEMENT_SIZE[data_array[i]->class] * (size_t) data_array[i]->capacity + sizeof(pl_object_struct));
    }
    puts("");
}

/// Kill the garbage collector and release all memory.
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

/// Check the status of the garbage collector.
/// @return 0 for stopped, 1 for working.
static int check_status(void)
{
    return global_table != NULL;
}

/*-----------------------------------------------------------------------------
 |  Get garbage collector namespace
 ----------------------------------------------------------------------------*/

pl_gc_ns pl_gc_get_ns(void)
{
    static const pl_gc_ns gc_ns = {.new_object           = new_object,
                                   .resize_object        = resize_object,
                                   .reserve_object       = reserve_object,
                                   .directly_reachable   = directly_reachable,
                                   .directly_unreachable = directly_unreachable,
                                   .garbage_collect      = garbage_collect,
                                   .report               = report,
                                   .kill                 = kill,
                                   .check_status         = check_status};
    return gc_ns;
}
