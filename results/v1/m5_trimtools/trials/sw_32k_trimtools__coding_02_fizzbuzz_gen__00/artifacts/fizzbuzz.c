#include <stdio.h>

int main(void)
{
    for (int i = 1; i <= 100; i++)
    {
        int fizz = (i % 3 == 0);
        int buzz = (i % 5 == 0);

        if (fizz && buzz)
            printf("%d fizz buzz\n", i);
        else if (fizz)
            printf("%d fizz\n", i);
        else if (buzz)
            printf("%d buzz\n", i);
        else
            printf("%d\n", i);
    }

    return 0;
}
