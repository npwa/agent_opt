#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;
    
    int len = strlen(src);
    if (len == 0) {
        dest[0] = '\0';
        return;
    }
    
    int src_idx = 0;
    int dest_idx = 0;
    
    while (src_idx < len) {
        char current_char = src[src_idx];
        int count = 0;
        
        while (src_idx < len && src[src_idx] == current_char) {
            count++;
            src_idx++;
        }
        
        dest[dest_idx++] = current_char;
        char count_str[12];
        sprintf(count_str, "%d", count);
        int count_len = strlen(count_str);
        memcpy(dest + dest_idx, count_str, count_len);
        dest_idx += count_len;
    }
    
    dest[dest_idx] = '\0';
}
