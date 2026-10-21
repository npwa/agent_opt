#include <stdio.h>
#include "compress.c"

int main() {
    char dest[64];
    
    compress_string("aabcccccaaa", dest);
    printf("Input: aabcccccaaa -> Output: %s\n", dest);
    
    compress_string("aabbcc", dest);
    printf("Input: aabbcc -> Output: %s\n", dest);
    
    compress_string("abc", dest);
    printf("Input: abc -> Output: %s\n", dest);
    
    compress_string("aaaabbbbcccc", dest);
    printf("Input: aaaabbbbcccc -> Output: %s\n", dest);
    
    return 0;
}
