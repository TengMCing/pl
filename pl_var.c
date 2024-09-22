//
// Created by Patrick Li on 10/9/2023.
//

#include "pl_var.h"
#include "pl_class.h"
#include "pl_error.h"
#include "pl_gc.h"
#include "string.h"

// Global variables for storing user variables.
static pl_object global_var_table   = NULL;
static pl_object global_var_frames  = NULL;
static pl_object global_var_strings = NULL;
static pl_object global_var_objects = NULL;

/*-----------------------------------------------------------------------------
 |  Checks
 ----------------------------------------------------------------------------*/

#define check_null_pointer(x) pl_error_expect((x) != NULL,                      \
                                              PL_ERROR_UNEXPECTED_NULL_POINTER, \
                                              "Can't access NULL pointer `" #x "` !")

/*-----------------------------------------------------------------------------
 |  Init
 ----------------------------------------------------------------------------*/

// Init the variable table.
static void init(void)
{
    const pl_gc_ns gc_ns = pl_gc_get_ns();

    // If the garbage collector is killed, it needs to be restarted.
    if (global_var_table != NULL && gc_ns.check_status())
        return;

    const pl_object_ns object_ns = pl_object_get_ns();

    // Try to init the global variable table, and declare the global
    // variable table to be directly reachable.
    pl_error_try
    {
        global_var_table   = object_ns.primitive.new(PL_CLASS_LIST, 3);
        global_var_frames  = object_ns.primitive.new(PL_CLASS_INT, 1);
        global_var_strings = object_ns.primitive.new(PL_CLASS_LIST, 1);
        global_var_objects = object_ns.primitive.new(PL_CLASS_LIST, 1);

        object_ns.append(global_var_table, global_var_frames);
        object_ns.append(global_var_table, global_var_strings);
        object_ns.append(global_var_table, global_var_objects);

        gc_ns.directly_reachable(global_var_table);
    }
    pl_error_catch
    {
        // If fails, reset all pointers.
        global_var_table   = NULL;
        global_var_frames  = NULL;
        global_var_strings = NULL;
        global_var_objects = NULL;
        pl_error_rethrow();
    }
}

/*-----------------------------------------------------------------------------
 |  Find
 ----------------------------------------------------------------------------*/

// Returns the position or -1 for not found.
static int find(const char *const name, const int frame)
{
    check_null_pointer(name);
    pl_error_expect(frame >= 0,
                    PL_ERROR_INVALID_FRAME,
                    "Frame [%d] is negative!",
                    frame);

    const int name_length = (int) strlen(name);
    pl_error_expect(name_length > 0,
                    PL_ERROR_INVALID_VARIABLE_NAME,
                    "Variable name [%s] is invalid!",
                    name);
    init();

    pl_object *const string_array = global_var_strings->data;
    int *const frame_array        = global_var_frames->data;
    pl_misc_for_i(global_var_strings->length)
    {
        pl_object this_string = string_array[i];
        int this_frame        = frame_array[i];

        if (name_length == this_string->length && this_frame == frame)
        {
            int match                                = 1;
            char *const data_array                   = this_string->data;
            pl_misc_for_j(this_string->length) match = match && (data_array[j] == name[j]);
            if (match)
                return i;
        }
    }

    return -1;
}

/*-----------------------------------------------------------------------------
 |  Get
 ----------------------------------------------------------------------------*/

static pl_object get(const char *const name, const int frame)
{
    int position = find(name, frame);
    pl_error_expect(position >= 0,
                    PL_ERROR_VARIABLE_NOT_FOUND,
                    "Variable [%s] not found in frame [%d]!",
                    name,
                    frame);

    return ((pl_object *) global_var_objects->data)[position];
}

/*-----------------------------------------------------------------------------
 |  Set
 ----------------------------------------------------------------------------*/

static pl_object set(const char *const name, pl_object content, const int frame)
{
    int position = find(name, frame);
    check_null_pointer(content);

    const pl_object_ns object_ns = pl_object_get_ns();

    if (position >= 0)
    {
        ((pl_object *) global_var_objects->data)[position] = content;
    }
    else
    {
        const int var_frames_length  = global_var_frames->length;
        const int var_strings_length = global_var_strings->length;
        const int var_objects_length = global_var_objects->length;

        pl_error_try
        {
            object_ns.primitive.extend_int(global_var_frames, frame);

            const int name_length = (int) strlen(name);
            pl_object this_name   = object_ns.primitive.new_from_array(PL_CLASS_CHAR, name_length, name);
            object_ns.append(global_var_strings, this_name);

            object_ns.append(global_var_objects, content);
        }
        pl_error_catch
        {
            // Recover the tables if fails.
            global_var_frames->length  = var_frames_length;
            global_var_strings->length = var_strings_length;
            global_var_objects->length = var_objects_length;
            pl_error_rethrow();
        }
    }

    return content;
}


/*-----------------------------------------------------------------------------
 |  Delete
 ----------------------------------------------------------------------------*/

static void delete_pos(const int position)
{
    if (position == -1)
        return;

    // If it is the last item.
    if (position == global_var_strings->length - 1)
    {
        global_var_frames->length--;
        global_var_strings->length--;
        global_var_objects->length--;
        return;
    }

    // Otherwise, move the memory.
    int *const frame_data_array = global_var_frames->data;
    memmove(frame_data_array + position,
            frame_data_array + position + 1,
            (size_t) (global_var_frames->length - position - 1) * PL_CLASS_ELEMENT_SIZE[PL_CLASS_INT]);
    global_var_frames->length--;

    pl_object *const string_data_array = global_var_strings->data;
    memmove(string_data_array + position,
            string_data_array + position + 1,
            (size_t) (global_var_strings->length - position - 1) * PL_CLASS_ELEMENT_SIZE[PL_CLASS_LIST]);
    global_var_strings->length--;

    pl_object *const object_data_array = global_var_objects->data;
    memmove(object_data_array + position,
            object_data_array + position + 1,
            (size_t) (global_var_objects->length - position - 1) * PL_CLASS_ELEMENT_SIZE[PL_CLASS_LIST]);
    global_var_objects->length--;
}

static void delete(const char *name, const int frame)
{
    int position = find(name, frame);
    delete_pos(position);
}

/*-----------------------------------------------------------------------------
 |  Delete a frame
 ----------------------------------------------------------------------------*/

static void delete_frame(const int frame)
{
    pl_error_expect(frame >= 0,
                    PL_ERROR_INVALID_FRAME,
                    "Frame [%d] is negative!",
                    frame);

    int i                       = 0;
    int *const frame_data_array = global_var_frames->data;
    while (i < global_var_frames->length)
    {
        int position = frame_data_array[i];
        if (position == frame)
            delete_pos(position);
        else
            i++;
    }
}

/*-----------------------------------------------------------------------------
 |  Delete returned frames
 ----------------------------------------------------------------------------*/

static void delete_frames_greater(const int frame)
{
    pl_error_expect(frame >= 0,
                    PL_ERROR_INVALID_FRAME,
                    "Frame [%d] is negative!",
                    frame);

    int i                       = 0;
    int *const frame_data_array = global_var_frames->data;
    while (i < global_var_frames->length)
    {
        int position = frame_data_array[i];
        if (position > frame)
            delete_pos(position);
        else
            i++;
    }
}

/*-----------------------------------------------------------------------------
 |  Get the max frame number
 ----------------------------------------------------------------------------*/

static int max_frame_number(void)
{
    init();
    int max_frame               = -1;
    int *const frame_data_array = global_var_frames->data;
    pl_misc_for_i(global_var_frames->length)
    {
        if (frame_data_array[i] > max_frame)
            max_frame = frame_data_array[i];
    }
    return max_frame;
}

/*-----------------------------------------------------------------------------
 |  Get variable namespace
 ----------------------------------------------------------------------------*/

pl_var_ns pl_var_get_ns(void)
{
    static const pl_var_ns var_ns = {.get                    = get,
                                     .set                    = set,
                                     .delete                 = delete,
                                     .delete_frame           = delete_frame,
                                     .delete_frames_greater = delete_frames_greater,
                                     .max_frame_number       = max_frame_number};
    return var_ns;
}
