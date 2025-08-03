#include <gc/gc.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define KEPLER_TO_STRING(format, value) int length = snprintf(NULL, 0, format, value);\
    char* result = (char*)GC_MALLOC_ATOMIC(length + 1);\
    snprintf(result, length + 1, format, value);\
    return result;

// ----------------------------------------
// Library functions
// ----------------------------------------

void print(const char* message) {
    printf("%s\n", message);
}

void pause() {
    printf("\nPress enter to continue...\n");
    getchar();
}

// ----------------------------------------
// Internal functions
// ----------------------------------------

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

char* __kepler_bool_to_string(bool value) {
    const char* s = value ? "true" : "false";
    size_t length = __kepler_strlen(s);
    char* result = (char*)GC_MALLOC_ATOMIC(length + 1);
    memcpy(result, s, length);
    result[length] = '\0';
    return result;
}

char* __kepler_i8_to_string(int8_t value) {
    KEPLER_TO_STRING("%d", (int)value)
}

char* __kepler_i16_to_string(int16_t value) {
    KEPLER_TO_STRING("%" PRId16, value)
}

char* __kepler_i32_to_string(int32_t value) {
    KEPLER_TO_STRING("%" PRId32, value)
}

char* __kepler_i64_to_string(int64_t value) {
    KEPLER_TO_STRING("%" PRId64, value)
}

char* __kepler_f32_to_string(float value) {
    KEPLER_TO_STRING("%g", value)
}

char* __kepler_f64_to_string(double value) {
    KEPLER_TO_STRING("%g", value)
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
