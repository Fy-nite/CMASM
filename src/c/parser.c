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

// Helper function to find a label by name
int find_label(MASMLabels* labels, const char* name) {
    for (int i = 0; i < labels->label_count; i++) {
        if (strcmp(labels->labels[i].name, name) == 0) {
            return labels->labels[i].instruction_index;
        }
    }
    return -1; // Label not found
}

// Tokenize a single line of assembly code into an Instruction
bool tokenize_instruction(const char* line, Instruction* instruction, int* operands_array, int* operand_types_array, MASMLabels* labels) {
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
                const char* label_name = operand + 1;
                int label_index = find_label(labels, label_name);
                if (label_index != -1) {
                    operands_array[i] = label_index; // Store instruction index
                    operand_types_array[i] = LABEL;
                } else {
                    fprintf(stderr, "Error: Undefined label '%s'\n", label_name);
                    free(line_copy);
                    return false;
                }
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

    // Initialize labels array
    MASMLabels labels = {0};
    labels.labels = (MASMLabel*)malloc(sizeof(MASMLabel) * 10); // Initial capacity
    if (!labels.labels) {
        fprintf(stderr, "Memory allocation failed for labels\n");
        free_instruction_array(instruction_array);
        return NULL;
    }
    labels.label_count = 0;
    labels.label_capacity = 10;

    char* code_copy = strdup(source_code);
    if (!code_copy) {
        fprintf(stderr, "Memory allocation failed for source code copy\n");
        free_instruction_array(instruction_array);
        free(labels.labels);
        return NULL;
    }

    // First Pass: Collect Labels
    char* line = strtok(code_copy, "\n");
    int instruction_index = 0; // Keep track of instruction index
    while (line) {
        // Skip empty lines and comments
        char* trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        if (*trimmed == '\0' || *trimmed == ';') {
            line = strtok(NULL, "\n");
            continue;
        }

        // Check if the line is a label definition
        if (strncasecmp(trimmed, "LBL", 3) == 0) {
            // Extract label name
            char* label_name = trimmed + 3;
            while (isspace(*label_name)) label_name++;
            char* label_end = label_name;
            while (*label_end != '\0' && !isspace(*label_end)) label_end++;
            int label_len = label_end - label_name;
            char name[32]; // Assuming max label length of 31 characters
            strncpy(name, label_name, label_len);
            name[label_len] = '\0';

            // Check if label already exists
            if (find_label(&labels, name) != -1) {
                fprintf(stderr, "Error: Duplicate label '%s'\n", name);
                free_instruction_array(instruction_array);
                free(code_copy);
                free(labels.labels);
                return NULL;
            }

            // Add label to array
            if (labels.label_count >= labels.label_capacity) {
                labels.label_capacity *= 2;
                labels.labels = (MASMLabel*)realloc(labels.labels, sizeof(MASMLabel) * labels.label_capacity);
                if (!labels.labels) {
                    fprintf(stderr, "Memory reallocation failed for labels\n");
                    free_instruction_array(instruction_array);
                    free(code_copy);
                    free(labels.labels);
                    return NULL;
                }
            }
            labels.labels[labels.label_count].name = strdup(name);
            labels.labels[labels.label_count].instruction_index = instruction_index;
            labels.label_count++;
        } else {
            instruction_index++; // Increment instruction index for non-label lines
        }

        line = strtok(NULL, "\n");
    }

    // Second Pass: Parse Instructions
    free(code_copy);
    code_copy = strdup(source_code); // Restore source code copy
    if (!code_copy) {
        fprintf(stderr, "Memory allocation failed for source code copy\n");
        free_instruction_array(instruction_array);
        // Free label names
        for (int i = 0; i < labels.label_count; i++) {
            free(labels.labels[i].name);
        }
        free(labels.labels);
        return NULL;
    }

    line = strtok(code_copy, "\n");
    instruction_index = 0; // Reset instruction index
    while (line) {
        // Skip empty lines and comments
        char* trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        if (*trimmed == '\0' || *trimmed == ';') {
            line = strtok(NULL, "\n");
            continue;
        }

        // Skip label definitions in the second pass
        if (strncasecmp(trimmed, "LBL", 3) == 0) {
            line = strtok(NULL, "\n");
            continue;
        }

        // Parse instruction
        Instruction instruction = {0};
        int operands_array[4] = {0}; // Initialize operands array
        int operand_types_array[4] = {0}; // Initialize operand types array
        if (!tokenize_instruction(trimmed, &instruction, operands_array, operand_types_array, &labels)) {
            fprintf(stderr, "Error parsing instruction: %s\n", trimmed);
            free_instruction_array(instruction_array);
            free(code_copy);
            // Free label names
            for (int i = 0; i < labels.label_count; i++) {
                free(labels.labels[i].name);
            }
            free(labels.labels);
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
                // Free label names
                for (int i = 0; i < labels.label_count; i++) {
                    free(labels.labels[i].name);
                }
                free(labels.labels);
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
        instruction_index++;
    }

    free(code_copy);
    // Free label names
    for (int i = 0; i < labels.label_count; i++) {
        free(labels.labels[i].name);
    }
    free(labels.labels);
    return instruction_array;
}

