//
// Created by Patrick Li on 30/6/2023.
//

#include "pl_bt.h"
#include "pl_error.h"
#include "pl_misc.h"
#include "stdio.h"
#include "string.h"

/*-----------------------------------------------------------------------------
 |  Global backtrace stack
 ----------------------------------------------------------------------------*/

#define PL_BT_MAX_ALLOWED_FRAME_NUM 256
#define PL_BT_EXTRA_FRAME_NUM 8

typedef struct bt
{
    int frame_count;
    const char *file_name[PL_BT_MAX_ALLOWED_FRAME_NUM + PL_BT_EXTRA_FRAME_NUM];
    const char *function_name[PL_BT_MAX_ALLOWED_FRAME_NUM + PL_BT_EXTRA_FRAME_NUM];
    int line[PL_BT_MAX_ALLOWED_FRAME_NUM + PL_BT_EXTRA_FRAME_NUM];
} bt;

static bt global_bt = {0};

static bt global_bt_backup = {0};

/*-----------------------------------------------------------------------------
 |  Pop
 ----------------------------------------------------------------------------*/

static void pop(void)
{
    if (global_bt.frame_count > 0)
        global_bt.frame_count--;
}

/*-----------------------------------------------------------------------------
 |  Push
 ----------------------------------------------------------------------------*/

static pl_optional_void push(const pl_bt_frame frame)
{
    global_bt.file_name[global_bt.frame_count]     = frame.file_name;
    global_bt.function_name[global_bt.frame_count] = frame.function_name;
    global_bt.line[global_bt.frame_count]          = frame.line;
    global_bt.frame_count++;

    pl_error_expect(global_bt.frame_count < PL_BT_MAX_ALLOWED_FRAME_NUM,
                    pl_optional_void,
                    PL_ERROR_STACKOVERFLOW,
                    "Stackoverflow! Depth > [%d].",
                    PL_BT_MAX_ALLOWED_FRAME_NUM);
    return (pl_optional_void){0};
}

/*-----------------------------------------------------------------------------
 |  Get depth
 ----------------------------------------------------------------------------*/

static int get_depth(void)
{
    return global_bt.frame_count;
}

/*-----------------------------------------------------------------------------
 |  Get frame
 ----------------------------------------------------------------------------*/

static pl_bt_optional_frame get_frame(const int depth)
{
    pl_error_expect(depth >= 0 && depth < global_bt.frame_count,
                    pl_bt_optional_frame,
                    PL_ERROR_INDEX_OUT_OF_BOUND,
                    "Index [%d] out of bound [0, %d)!",
                    depth,
                    global_bt.frame_count);

    pl_bt_frame tmp_frame = {.file_name     = global_bt.file_name[depth],
                             .function_name = global_bt.function_name[depth],
                             .line          = global_bt.line[depth]};
    return (pl_bt_optional_frame){.value = tmp_frame};
}

/*-----------------------------------------------------------------------------
 |  Print backtrace
 ----------------------------------------------------------------------------*/

static void print_bt(const bt this_bt)
{
    if (this_bt.frame_count < 1)
        return;
    printf("Backtrace - %d frames in stack:\n", this_bt.frame_count);
    char spaces[PL_BT_MAX_ALLOWED_FRAME_NUM]     = {'\0'};
    int func_length[PL_BT_MAX_ALLOWED_FRAME_NUM] = {0};
    int max_func_length                          = -1;
    pl_misc_for_i(this_bt.frame_count)
    {
        func_length[i] = (int) strlen(this_bt.function_name[i]);
        if (func_length[i] > max_func_length)
            max_func_length = func_length[i];
    }

    for (int i = this_bt.frame_count - 1; i >= 0; i--)
    {
        if (i != 0)
            fputs("  ║═", stdout);
        else
            fputs("  ╚═", stdout);

        int space_length = max_func_length - (int) strlen(this_bt.function_name[i]);

        pl_misc_for_j(space_length) spaces[j] = ' ';
        spaces[space_length]                  = '\0';

        printf("[%d] Calling <%.*s>%s from %s:%d\n",
               i,
               (int) strlen(this_bt.function_name[i]),
               this_bt.function_name[i],
               spaces,
               this_bt.file_name[i],
               this_bt.line[i]);
    }
}

/*-----------------------------------------------------------------------------
 |  Print the global backtrace
 ----------------------------------------------------------------------------*/

static void print(void)
{
    print_bt(global_bt);
}

/*-----------------------------------------------------------------------------
 |  Backup the backtrace
 ----------------------------------------------------------------------------*/

static void backup(void)
{
    global_bt_backup.frame_count = global_bt.frame_count;
    pl_misc_for_i(global_bt.frame_count)
    {
        global_bt_backup.function_name[i] = global_bt.function_name[i];
        global_bt_backup.file_name[i]     = global_bt.file_name[i];
        global_bt_backup.line[i]          = global_bt.line[i];
    }
}

/*-----------------------------------------------------------------------------
 |  Print the backup backtrace
 ----------------------------------------------------------------------------*/

static void print_backup(void)
{
    print_bt(global_bt_backup);
}

/*-----------------------------------------------------------------------------
 |  Get backtrace namespace
 ----------------------------------------------------------------------------*/

pl_bt_ns pl_bt_get_ns(void)
{
    static const pl_bt_ns bt_ns = {.pop          = pop,
                                   .push         = push,
                                   .get_depth    = get_depth,
                                   .get_frame    = get_frame,
                                   .print        = print,
                                   .backup       = backup,
                                   .print_backup = print_backup};
    return bt_ns;
}
