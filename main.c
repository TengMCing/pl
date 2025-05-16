#include "pl.h"
#include "stdio.h"

int test(pl_ns pl)
{
    pl.class.test();
    pl.error.test();
    pl.gc.test();
}

int main()
{
    // Get the namespace.
    pl_ns pl = pl_get_ns();

    // Variables declared at the beginning of the frame
    // will be initialized and protected until the end of the frame.
    $begin_frame(x, y, z)
    {
        // R: x <- c(1, 2, 3, 1)
        $set(x, $c_int(1, 2, 3, 1));
        // R: y <- c(1)
        $set(y, $c_int(1));
        // R: print(x == y)
        pl.object.print(pl.object.equal(x, y));
        // R: print(intToUtf8(x))
        pl.object.print(pl.object.as_char(x));

        // Set 10 decimals for printing double vector.
        pl.object.print_set_decimals($local_c_int(10));
        // R: print(as.double(x))
        pl.object.print(pl.object.as_double(x));

        // R: do.call(print, list(x))
        pl.object.print($list(x));

        // R: z <- list(x, y)
        $set(z, $list(x, y));
        // R: print(z)
        pl.object.print(z);

        // Report the memory usage.
        pl.gc.report();
    }
    $end_frame_and_gc;

    // Report the memory usage.
    pl.gc.report();

    // Run tests
    test(pl);

    return 0;
}
