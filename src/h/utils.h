#ifndef MASM_UTILS_H
#define MASM_UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include "instruction.h"
#ifdef __linux__
#include <regex.h>
#elif _WIN32
// find a better way to do this
#include <windows.h>
#endif
#define MASM_VERSION "1.0.0"
#define MASM_KEYWORD "masm"
#define MASM_KEYWORD_LEN 4





// lbl <name> or LBL <name>
#define MASM_LABEL_REGEX "^[a-zA-Z_][a-zA-Z0-9_]*$"

bool CHECK_NULL(void* ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return false;
    }
    return true;
}

bool CHECK_NULL_STR(char* str) {
    if (str == NULL) {
        fprintf(stderr, "String allocation failed\n");
        return false;
    }
    return true;
}

bool CHECK_NULL_INT(int* ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "Integer allocation failed\n");
        return false;
    }
    return true;
}

bool free_ptr(void* ptr) {
    if (ptr != NULL) {
        free(ptr);
        return true;
    }
    return false;
}

bool free_interpreter(struct MASMObject* interpreter) {
    if (interpreter != NULL) {
        free(interpreter->instructions);
        free(interpreter->labels);
        free(interpreter->functions);
        free(interpreter->variables);
        free(interpreter->memory);
        free(interpreter->code);
        free(interpreter);
        return true;
    }
    return false;
}

void box(char* str) {
    int len = strlen(str);
    printf("+");
    for (int i = 0; i < len + 2; i++) {
        printf("-");
    }
    printf("+\n| %s |\n+", str);
    for (int i = 0; i < len + 2; i++) {
        printf("-");
    }
    printf("+\n");
}
void print_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "\033[31m"); // Set text color to red
    vfprintf(stderr, format, args);
    fprintf(stderr, "\033[0m"); // Reset text color
    va_end(args);
}
void print_warning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "\033[33m"); // Set text color to yellow
    vfprintf(stderr, format, args);
    fprintf(stderr, "\033[0m"); // Reset text color
    va_end(args);
}
void print_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "\033[32m"); // Set text color to green
    vfprintf(stderr, format, args);
    fprintf(stderr, "\033[0m"); // Reset text color
    va_end(args);
}
void print_debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "\033[34m"); // Set text color to blue
    vfprintf(stderr, format, args);
    fprintf(stderr, "\033[0m"); // Reset text color
    va_end(args);
}


#endif // MASM_UTILS_H