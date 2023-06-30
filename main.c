#include "pl.h"
#include "stdio.h"

static pl_optional_double test2(void)
{
    return (pl_optional_double){.value = 2.0};
}

static pl_optional_int test(void)
{
    pl_optional_double x = pl_bt_call(pl_optional_double, test2);
    pl_error_pass_to_caller(x.error, pl_optional_int);
    return (pl_optional_int){.value = (int) x.value};
}

int main()
{
    pl_ns pl = pl_get_ns();
    pl_optional_int x = pl_bt_call(pl_optional_int, test);
    pl.error.optionally_print_and_exit(x.error);
    return pl.misc.compare_address(NULL, NULL);
}
