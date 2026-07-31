#include <stdio.h>

float sqrt_iterative(float number) {
    if(number == 0 || number == 1)
        return number;
    
    float result = number; // Initial guess
    
    // Iteratively improving our estimate until it converges to a stable value.
    while (result * result - number > 0.00001 || result * result - number < -0.00001) {
        result = (number / result + result) / 2;
    }
    
    return result;
}

int main() {
    float num;
    printf("Enter a number to find its square root: ");
    scanf("%f", &num);
    
    if(num < 0)
    {
        printf("Square root not defined for negative numbers.\n");
    } else
    {
        float sqrt_value = sqrt_iterative(num);
        printf("The approximate square root of %.2f is %.6f\n", num, sqrt_value);
    }
    return 0;
}
