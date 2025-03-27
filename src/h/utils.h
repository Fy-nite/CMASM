#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdarg.h>
#include <regex.h>
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

bool is_valid_label(const char* label) {
    regex_t regex;
    int reti;

    // Compile the regular expression
    reti = regcomp(&regex, MASM_LABEL_REGEX, REG_EXTENDED);
    if (reti) {
        fprintf(stderr, "Could not compile regex for is_valid_label\n");
        return false;
    }

    // Execute the regular expression
    reti = regexec(&regex, label, 0, NULL, 0);
    if (reti == REG_NOMATCH) {
        regfree(&regex);
        return false; // Label does not match the regex
    } else if (reti) {
        fprintf(stderr, "Regex match failed for is_valid_label\n");
        regfree(&regex);
        return false;
    }

    regfree(&regex);
    return true; // Label is valid
}

bool is_valid_number(const char* number) {
    char* endptr;
    errno = 0; // Clear errno before strtol
    long val = strtol(number, &endptr, 10);
    if (errno != 0 || *endptr != '\0' || val < INT32_MIN || val > INT32_MAX) {
        return false; // Not a valid number
    }
    return true; // Valid number
}
bool is_valid_hex(const char* hex) {
    char* endptr;
    errno = 0; // Clear errno before strtol
    long val = strtol(hex, &endptr, 16);
    if (errno != 0 || *endptr != '\0' || val < INT32_MIN || val > INT32_MAX) {
        return false; // Not a valid hex number
    }
    return true; // Valid hex number
}
// TODO: make the dammed thing actualy tokenize the string before it goes through
bool is_valid_register(const char* reg) {
    if (reg == NULL) {
        return false;
    }
    if (strcmp(reg, "RAX") == 0 || strcmp(reg, "RBX") == 0 || strcmp(reg, "RCX") == 0 ||
        strcmp(reg, "RDX") == 0 || strcmp(reg, "RSI") == 0 || strcmp(reg, "RDI") == 0 ||
        strcmp(reg, "RBP") == 0 || strcmp(reg, "RSP") == 0) {
        return true; // Valid register
    }
    return false; // Invalid register
}