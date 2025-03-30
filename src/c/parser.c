#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <stdbool.h>

#include "../h/instruction.h"
int get_register_enum(const char* reg);
InstructionArray *create_instruction_array(int size) {
    InstructionArray* instruction_array = (InstructionArray*)malloc(sizeof(InstructionArray));
    if (instruction_array == NULL) {
        fprintf(stderr, "Memory allocation failed for InstructionArray\n");
        return NULL;
    }
    instruction_array->instructions = (Instruction*)malloc(sizeof(Instruction) * size);
    if (instruction_array->instructions == NULL) {
        fprintf(stderr, "Memory allocation failed for instructions\n");
        free(instruction_array);
        return NULL;
    }
    instruction_array->size = size;
    instruction_array->count = 0; // Initialize count to 0
    return instruction_array;
}

void free_instruction_array(InstructionArray* instruction_array) {
    if (instruction_array != NULL) {
        for (int i = 0; i < instruction_array->count; i++) {
            //free(instruction_array->instructions[i].operand_types);
        }
        free(instruction_array->instructions);
        free(instruction_array);
    }
}

// Helper function to map a string opcode to the Instructions enum
int get_instruction_opcode(const char* opcode) {
    if (strcasecmp(opcode, "MOV") == 0) return MOV;
    if (strcasecmp(opcode, "ADD") == 0) return ADD;
    if (strcasecmp(opcode, "SUB") == 0) return SUB;
    if (strcasecmp(opcode, "MUL") == 0) return MUL;
    if (strcasecmp(opcode, "DIV") == 0) return DIV;
    if (strcasecmp(opcode, "JMP") == 0) return JMP;
    if (strcasecmp(opcode, "HLT") == 0) return HLT;
    if (strcasecmp(opcode, "LBL") == 0) return NONE; // Labels are not instructions
    if (strcasecmp(opcode, "NOP") == 0) return NOP;
    return -1; // Invalid opcode
}

// Tokenize a single line of assembly code into an Instruction
bool tokenize_instruction(const char* line, Instruction* instruction, int* operands_array, int* operand_types_array) {
    char* line_copy = strdup(line);
    if (!line_copy) {
        fprintf(stderr, "Memory allocation failed for line copy\n");
        return false;
    }

    // Find the first space to separate opcode from operands
    char* space = strchr(line_copy, ' ');
    if (!space) {
        // No space found, just an opcode with no operands
        int opcode = get_instruction_opcode(line_copy);
        if (opcode == -1) {
            fprintf(stderr, "Unknown opcode: %s\n", line_copy);
            free(line_copy);
            return false;
        }
        instruction->opcode = opcode;
        free(line_copy);
        return true;
    }

    // Temporarily terminate the string at the space to extract opcode
    *space = '\0';
    int opcode = get_instruction_opcode(line_copy);
    if (opcode == -1) {
        fprintf(stderr, "Unknown opcode: %s\n", line_copy);
        free(line_copy);
        return false;
    }
    instruction->opcode = opcode;

    // Now get the operands (skip any extra spaces)
    char* operands_start = space + 1;
    while (*operands_start == ' ') operands_start++;

    int i = 0;
    char* operand_ptr = operands_start;
    while (*operand_ptr != '\0' && i < 4) {
        // Skip leading whitespace
        while (isspace(*operand_ptr)) operand_ptr++;

        // Find the end of the operand
        char* operand_end = operand_ptr;
        while (*operand_end != '\0' && !isspace(*operand_end) && *operand_end != ',') operand_end++;

        // Calculate the length of the operand
        int operand_len = operand_end - operand_ptr;

        // Copy the operand into a temporary buffer
        char operand[32]; // Assuming max operand length of 31 characters
        strncpy(operand, operand_ptr, operand_len);
        operand[operand_len] = '\0';

        // Check if the operand is a register
        int reg_enum = get_register_enum(operand);
        if (reg_enum != -1) {
            operands_array[i] = reg_enum;
            operand_types_array[i] = REGISTER;
        } else {
            // Otherwise, treat it as an immediate value or label
            if (operand[0] == '#') {
                // Label
                operands_array[i] = atoi(operand + 1); // Convert label to integer
                operand_types_array[i] = LABEL;
            } else {
                operands_array[i] = atoi(operand); // Convert to integer
                operand_types_array[i] = IMMEDIATE;
            }
        }

        // Move to the next operand
        operand_ptr = operand_end;
        if (*operand_ptr == ',') operand_ptr++; // Skip comma
        i++;
    }
    instruction->operand_count = i;

    free(line_copy);
    return true;
}

// Update parse_instructions to use tokenize_instruction
InstructionArray* parse_instructions(const char* source_code) {
    InstructionArray* instruction_array = create_instruction_array(100); // Initial size
    if (!instruction_array) {
        return NULL;
    }

    char* code_copy = strdup(source_code);
    if (!code_copy) {
        fprintf(stderr, "Memory allocation failed for source code copy\n");
        free_instruction_array(instruction_array);
        return NULL;
    }

    char* line = strtok(code_copy, "\n");
    while (line) {
        // Skip empty lines and comments
        char* trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        if (*trimmed == '\0' || *trimmed == ';') {
            line = strtok(NULL, "\n");
            continue;
        }

        // Check if the line is a label
        if (strncasecmp(trimmed, "LBL", 3) == 0) {
            // Labels are not instructions, so skip them
            line = strtok(NULL, "\n");
            continue;
        }

        // Parse instruction
        Instruction instruction = {0};
        int operands_array[4] = {0}; // Initialize operands array
        int operand_types_array[4] = {0}; // Initialize operand types array
        if (!tokenize_instruction(trimmed, &instruction, operands_array, operand_types_array)) {
            fprintf(stderr, "Error parsing instruction: %s\n", trimmed);
            free_instruction_array(instruction_array);
            free(code_copy);
            return NULL;
        }

        // Add instruction to array
        if (instruction_array->count >= instruction_array->size) {
            instruction_array->size *= 2;
            instruction_array->instructions = (Instruction*)realloc(instruction_array->instructions, sizeof(Instruction) * instruction_array->size);
            if (!instruction_array->instructions) {
                fprintf(stderr, "Memory reallocation failed for instructions\n");
                free_instruction_array(instruction_array);
                free(code_copy);
                return NULL;
            }
        }
        // Copy operands to instruction
        for (int i = 0; i < 4; i++) {
            instruction.operands[i] = operands_array[i];
            instruction.operand_types[i] = operand_types_array[i];
        }
        instruction_array->instructions[instruction_array->count++] = instruction;

        line = strtok(NULL, "\n");
    }

    free(code_copy);
    return instruction_array;
}

