#include <stdio.h>

int main(void) {
    for (int i = 1; i <= 100; i++) {
        if (i % 3 == 0) {
            printf("%d", i);
            printf(" fizz");
        } else {
            printf("%d", i);
        }
        if (i % 5 == 0) {
            printf(" buzz");
        }
        printf("\n");
    }
    return 0;
}
