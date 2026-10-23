#include <stdio.h>

int main(void) {
    for (int i = 1; i <= 100; i++) {
        int is_div3 = (i % 3 == 0);
        int is_div5 = (i % 5 == 0);

        if (is_div3 && is_div5)
            printf("%d fizz buzz\n", i);
        else if (is_div3)
            printf("%d fizz\n", i);
        else if (is_div5)
            printf("%d buzz\n", i);
        else
            printf("%d\n", i);
    }

    return 0;
}
