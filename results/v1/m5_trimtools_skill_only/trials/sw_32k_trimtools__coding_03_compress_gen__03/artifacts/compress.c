#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compress.h"

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;
    
    int i = 0;
    while (src[i]) {
        char current = src[i];
        int count = 1;
        
        while (src[i + count] == current) {
            count++;
        }
        
        *dest++ = current;
        dest += sprintf(dest, "%d", count);
        
        i += count;
    }
    
    *dest = '\0';
}
