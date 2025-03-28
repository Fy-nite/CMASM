#ifndef _HELPER_CPP
#define _HELPER_CPP
#include "../h/cpu.h"
#include "../h/instruction.h"
#include "../h/errors.h"
#include "../h/utils.h"
#include <regex>
#include <cerrno>
#include <climits>
#include <cstring>
#include <cstdio>

bool is_valid_label(const char* label) {
    std::regex regex(MASM_LABEL_REGEX);
    if (!std::regex_match(label, regex)) {
        return false; // Label does not match the regex
    }
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

bool is_valid_register(const char* reg) {
    if (reg == nullptr) {
        return false;
    }
    const char* valid_registers[] = {"RAX", "RBX", "RCX", "RDX", "RSI", "RDI", "RBP", "RSP"};
    for (const char* valid_reg : valid_registers) {
        if (strcmp(reg, valid_reg) == 0) {
            return true; // Valid register
        }
    }
    return false; // Invalid register
}

#endif // _HELPER_CPP