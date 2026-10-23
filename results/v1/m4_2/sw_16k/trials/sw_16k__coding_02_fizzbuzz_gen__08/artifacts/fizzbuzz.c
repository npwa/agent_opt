#include <stdio.h>

int main() {
    for (int i = 1; i <= 100; i++) {
        int div3 = (i % 3 == 0);
        int div5 = (i % 5 == 0);
        if (div3 || div5) {
            printf("%d", i);
            if (div3)
                printf(" fizz");
            if (div5)
                printf(" buzz");
        } else {
            printf("%d", i);
        }
        printf("\n");
    }
    return 0;
}
