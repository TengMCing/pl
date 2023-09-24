#include "pl.h"
#include "stdio.h"

// static pl_object range(int end)
// {
//     pl_ns pl = pl_get_ns();
//     declare_variables(x)
//     {
//         pl_misc_for_i(end)
//         {
//         }
//     }
// }


void test(void)
{
    // Get the namespace.
    pl_ns pl = pl_get_ns();

    pl.class.test();
    pl.error.test();
    pl.misc.test();
}


int main()
{

    // Get the namespace.
    pl_ns pl = pl_get_ns();

    test();

    return 0;

    // In R: v <- c(1, 2, 3)
    pl.var.set("v", c(1, 2, 3), 0);

    // In R: v <- c(4, 5, 6)
    pl.var.set("v", c(4, 5, 6), 0);

    // In R: l <- list(c('a', 'b', 'c'), c(1, 2, 3, NA))
    pl.var.set("l", list(c(*"a", 'b', 'c'), c(1, 2, 3, PL_INT_NA)), 0);

    // In R: print(c(4) %in% v)
    // stdout: [0]
    pl.object.print(pl.object.in(local_c(4), pl.var.get("v", 0)));

    // In R: print(v[c(1, 3, 3, 2)])
    // stdout: [1, 3, 3, 2]
    pl.object.print(pl.object.subset(pl.var.get("v", 0), local_c(0, 2, 2, 1)));

    // In R: print(v[[c(1)]])
    // stdout: [1]
    pl.object.print(pl.object.extract(pl.var.get("v", 0), local_c(0)));

    // In R: print(l[[c(1)]])
    // stdout: ['a', 'b', 'c']
    pl.object.print(pl.object.extract(pl.var.get("l", 0), local_c(0)));

    // In R: print(l[[c(2)]])
    // stdout: [1, 2, 3, NA]
    pl.object.print(pl.object.extract(pl.var.get("l", 0), local_c(1)));


    // In R: l <- append(l, v)
    pl.object.append(pl.var.get("l", 0), pl.var.get("v", 0));

    // In R: print(l)
    // stdout: [<CHAR>, <INT>, <INT>]
    pl.object.print(pl.var.get("l", 0));

    // In R: l2 <- l
    pl.var.set("l2", pl.object.copy(pl.var.get("l", 0)), 0);

    // Report the memory usage.
    pl.gc.report();

    pl.var.delete("l2", 0);

    // Run the garbage collector.
    pl.gc.garbage_collect();

    // Report the memory usage.
    pl.gc.report();

    pl.var.delete("l", 0);

    // Run the garbage collector.
    pl.gc.garbage_collect();

    // Report the memory usage.
    pl.gc.report();

    pl.var.delete("v", 0);

    // Run the garbage collector.
    pl.gc.garbage_collect();

    // Report the memory usage.
    pl.gc.report();

    return 0;
}
