#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "memory.h"
#include "../h/instruction.h"

int len(char* str) {
    int count = 0;
    while (str[count] != '\0') {
        count++;
    }
    return count;
}

// Helper function to convert register name to enum value
int get_register_enum(const char* reg) {
    if (strcmp(reg, "RAX") == 0) return RAX;
    if (strcmp(reg, "RBX") == 0) return RBX;
    if (strcmp(reg, "RCX") == 0) return RCX;
    if (strcmp(reg, "RDX") == 0) return RDX;
    if (strcmp(reg, "RDI") == 0) return RDI;
    if (strcmp(reg, "RSI") == 0) return RSI;
    if (strcmp(reg, "RBP") == 0) return RBP;
    if (strcmp(reg, "RSP") == 0) return RSP;
    if (strcmp(reg, "R0") == 0) return R0;
    if (strcmp(reg, "R1") == 0) return R1;
    if (strcmp(reg, "R2") == 0) return R2;
    if (strcmp(reg, "R3") == 0) return R3;
    if (strcmp(reg, "R4") == 0) return R4;
    if (strcmp(reg, "R5") == 0) return R5;
    if (strcmp(reg, "R6") == 0) return R6;
    if (strcmp(reg, "R7") == 0) return R7;
    if (strcmp(reg, "R8") == 0) return R8;
    if (strcmp(reg, "R9") == 0) return R9;
    if (strcmp(reg, "R10") == 0) return R10;
    if (strcmp(reg, "R11") == 0) return R11;
    if (strcmp(reg, "R12") == 0) return R12;
    if (strcmp(reg, "R13") == 0) return R13;
    if (strcmp(reg, "R14") == 0) return R14;
    if (strcmp(reg, "R15") == 0) return R15;
    // Add other registers as needed
    return -1; // Invalid register
}

//TODO: calculate how much this function will explode in the future.
Instruction create_instruction(int opcode, int operands[4], int operand_types[4]) {
    Instruction instruction;
    instruction.opcode = opcode;
    for (int i = 0; i < 4; i++) {
        instruction.operands[i] = operands[i];
        instruction.operand_types[i] = operand_types[i];
    }
    instruction.operand_count = 4; // Fixed to 4 for now
    return instruction;
}