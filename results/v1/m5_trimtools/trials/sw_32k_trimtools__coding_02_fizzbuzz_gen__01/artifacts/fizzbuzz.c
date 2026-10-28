#include <stdio.h>
#include <string.h>

int main() {
    for (int i = 1; i <= 100; i++) {
        int fizz = (i % 3 == 0);
        int buzz = (i % 5 == 0);
        
        printf("%d", i);
        
        if (fizz && buzz) {
            printf(" fizz buzz\n");
        } else if (fizz) {
            printf(" fizz\n");
        } else if (buzz) {
            printf(" buzz\n");
        } else {
            printf("\n");
        }
    }
    return 0;
}
