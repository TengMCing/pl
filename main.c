#include "pl.h"
#include "stdio.h"

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
        pl.object.print_set_decimals($c_int(10));
        // R: print(as.double(x))
        pl.object.print(pl.object.as_double(x));

        // R: z <- list(x, y)
        $set(z, $list(x, y));
        // R: print(z)
        pl.object.print(z);

        // Report the memory usage.
        pl.gc.report();
    }
    $end_frame

    // The GC needs to be triggered manually at the moment. There is no scheduler for it.
    // This is a pretty simple stop-the-world GC so use it wisely.
    pl.gc.garbage_collect();

    // Report the memory usage.
    pl.gc.report();

    return 0;
}
