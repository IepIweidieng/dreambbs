#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "dao.h"

#define MMM     (0x40000)

static int
int_cmp(
    const void *a,
    const void *b
)
{
    return *(const int *)a - *(const int *)b;
}

int main(void)
{
    int *x, *y, *z, n;

    x = (int *) malloc(MMM * sizeof(int));
    if (!x)
        return 0;

    y = x;
    z = x + MMM;

    n = time(0) & (0x40000 -1) /* 16387 */;

    do
    {
        *x = n = (n * 10001) & (0x100000 - 1);
    } while (++x < z);

    xsort(y, MMM, sizeof(int), int_cmp);
    return 0;
}
