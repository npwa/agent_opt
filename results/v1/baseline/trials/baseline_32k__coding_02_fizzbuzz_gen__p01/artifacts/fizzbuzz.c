#include <stdio.h>

int main(void) {
    for (int i = 1; i <= 100; i++) {
        int div3 = (i % 3 == 0);
        int div5 = (i % 5 == 0);

        if (div3 && div5)
            printf("%d fizz buzz\n", i);
        else if (div3)
            printf("%d fizz\n", i);
        else if (div5)
            printf("%d buzz\n", i);
        else
            printf("%d\n", i);
    }
    return 0;
}
