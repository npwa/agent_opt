#include <stdio.h>

int main(void) {
    for (int i = 1; i <= 100; i++) {
        int fizz = (i % 3 == 0);
        int buzz = (i % 5 == 0);
        printf("%d", i);
        if (fizz) printf(" fizz");
        if (buzz) printf(" buzz");
        printf("\n");
    }
    return 0;
}
