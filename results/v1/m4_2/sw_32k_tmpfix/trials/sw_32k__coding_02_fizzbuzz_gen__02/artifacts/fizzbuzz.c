#include <stdio.h>

int main() {
    for (int n = 1; n <= 100; n++) {
        printf("%d", n);
        if (n % 3 == 0) {
            printf(" fizz");
        }
        if (n % 5 == 0) {
            printf(" buzz\n");
        } else {
            printf("\n");
        }
    }

    return 0;
}
