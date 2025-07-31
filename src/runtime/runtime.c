#include <gc/gc.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Internal functions, only callable from the compiler
void __kepler_runtime_init() {
    GC_INIT();
}

int64_t __kepler_strlen(const char* a) {
    int64_t result = 0;
    while (a[result] != '\0') {
        result++;
    }
    return result;
}

char* __kepler_string_concat(const char* a, const char* b) {
    size_t length_a = __kepler_strlen(a);
    size_t length_b = __kepler_strlen(b);
    char* result = (char*)GC_MALLOC_ATOMIC(length_a + length_b + 1);
    memcpy(result, a, length_a);
    memcpy(result + length_a, b, length_b);
    result[length_a + length_b] = '\0';
    return result;
}

// Library functions for the user
void print(const char* message) {
    printf("%s\n", message);
}

void pause() {
    printf("\nPress enter to continue...\n");
    getchar();
}
