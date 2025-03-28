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

//TODO: calculate how much this function will explode in the future.
Instruction create_instruction(char* opcode, char* operands) {
    Instruction instruction;
    instruction.opcode = opcode[0];
    instruction.operands = operands;
    instruction.operand_count = len(operands);
    instruction.operand_sizes = (int*)malloc(sizeof(int) * instruction.operand_count);
    if (instruction.operand_sizes == NULL) {
        fprintf(stderr, "Memory allocation failed for operand sizes\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < instruction.operand_count; i++) {
        instruction.operand_sizes[i] = sizeof(operands[i]);
    }
    instruction.operand_types = (int*)malloc(sizeof(int) * instruction.operand_count);
    if (instruction.operand_types == NULL) {
        fprintf(stderr, "Memory allocation failed for operand types\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < instruction.operand_count; i++) {
        if (operands[i] >= '0' && operands[i] <= '9') {
            instruction.operand_types[i] = 0; // Integer
        } else if (operands[i] >= 'a' && operands[i] <= 'z') {
            instruction.operand_types[i] = 1; // Register
        } else {
            instruction.operand_types[i] = 2; // Label
        }
    }
    return instruction;
}