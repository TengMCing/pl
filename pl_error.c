//
// Created by Patrick Li on 29/6/2023.
//

#include "pl_error.h"
#include "pl_bt.h"
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"

/// A global placeholder for storing error message.
static char global_error_message[PL_ERROR_MAX_MESSAGE_LEN] = {0};

/*-----------------------------------------------------------------------------
 |  Create error report
 ----------------------------------------------------------------------------*/

static pl_error create(const int error_code,
                       const char *const function_name,
                       const char *const file_name,
                       const int line,
                       const char *const format,
                       ...)
{
#ifndef PL_DISABLE_BT
    const pl_bt_ns bt_ns = pl_bt_get_ns();
    bt_ns.backup();
#endif

    // Create the customized error message.
    char message[PL_ERROR_MAX_MESSAGE_LEN] = {0};
    va_list ap;
    va_start(ap, format);
    const int message_len = vsnprintf(message,
                                      PL_ERROR_MAX_MESSAGE_LEN,
                                      format,
                                      ap);
    va_end(ap);

    // Check encoding error
    if (message_len < 0)
    {
        puts("PL Internal Error: Encounter an encoding error while processing error message! Set customized error message to be an empty string.");
        message[0] = '\0';
    }

    // Rewrite the buffer
    const int len = snprintf(global_error_message,
                             PL_ERROR_MAX_MESSAGE_LEN,
                             "[E%03d] Error raised by <%s> at %s:%d: %s\n",
                             error_code,
                             function_name,
                             file_name,
                             line,
                             message);

    // Check encoding error
    if (len < 0)
    {
        puts("PL Internal Error: Encounter an encoding error while processing error message! Set error message to be an empty string.");
        global_error_message[0] = '\0';
    }

    return (pl_error){.error_code = error_code, .error_message = global_error_message};
}

/*-----------------------------------------------------------------------------
 |  Optionally print
 ----------------------------------------------------------------------------*/

static void optionally_print(const pl_error error)
{
    if (!error.error_code)
        return;

#ifndef PL_DISABLE_BT
    pl_bt_ns bt_ns = pl_bt_get_ns();
    bt_ns.print_backup();
#endif

    puts(error.error_message);
}

/*-----------------------------------------------------------------------------
 |  Optionally print error and exit
 ----------------------------------------------------------------------------*/

static void optionally_print_and_exit(const pl_error error)
{
    if (!error.error_code)
        return;

#ifndef PL_DISABLE_BT
    pl_bt_ns bt_ns = pl_bt_get_ns();
    bt_ns.print_backup();
#endif

    puts(error.error_message);
    exit(EXIT_FAILURE);
}

/*-----------------------------------------------------------------------------
 |  Get error namespace
 ----------------------------------------------------------------------------*/

pl_error_ns pl_error_get_ns(void)
{
    static const pl_error_ns error_ns = {.create                    = create,
                                         .optionally_print          = optionally_print,
                                         .optionally_print_and_exit = optionally_print_and_exit};
    return error_ns;
}
