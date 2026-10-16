#ifndef DEFOLD_PIXELS_TESTS_CHECK_H
#define DEFOLD_PIXELS_TESTS_CHECK_H

// Minimal check macro shared by the native tests. A failed check prints its
// location and message and is counted; the test keeps running.

#include <stdio.h>

static int failures = 0;

#define CHECK(cond, ...)                                    \
    do                                                      \
    {                                                       \
        if (!(cond))                                        \
        {                                                   \
            failures++;                                     \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);     \
            printf(__VA_ARGS__);                            \
            printf("\n");                                   \
        }                                                   \
    } while (0)

// Prints the summary line and returns the process exit code.
static int report(const char *name)
{
    if (failures == 0)
    {
        printf("all %s tests passed\n", name);
        return 0;
    }
    printf("%d %s check(s) failed\n", failures, name);
    return 1;
}

#endif
