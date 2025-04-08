#ifndef __CPUCPP
#define __CPUCPP

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include "../h/cpu.h"
#include "../h/instruction.h"
extern InstructionArray* parse_instructions(const char* source_code);
void free_instruction_array(InstructionArray* instructions);

// Function prototypes for instruction handlers
int mov_instruction(Instruction* instruction, CPU* cpu);
int add_instruction(Instruction* instruction, CPU* cpu);
int sub_instruction(Instruction* instruction, CPU* cpu);
int jmp_instruction(Instruction* instruction, CPU* cpu);
int hlt_instruction(Instruction* instruction, CPU* cpu);

int create_cpu(CPU* cpu, const char* name, int64_t instruction_count) {
    if (cpu == nullptr) {
        throw std::invalid_argument("CPU pointer is null");
    }
    cpu->name = strdup(name);
    cpu->instruction_count = instruction_count;
    cpu->instruction_pointer = 0;
    cpu->stack_pointer = 0;
    cpu->base_pointer = 0;
    // maloc about 4mb of memory
    // TODO: make this a function call so that we can change the size of the memory outside of native code
    cpu->Memory = (int64_t*)malloc(sizeof(int64_t) * 1024 * 1024); // Assuming 4MB of memory is what we are going to stick with. though we should have a function do this instead.
    if (cpu->Memory == nullptr) {
        //TODO: make a error called horror that is a fatal error and prints a ascii art of a explosion
        throw std::runtime_error("Memory allocation failed for CPU memory");
    }
    memset(cpu->Registers, 0, sizeof(int64_t) * 24);
    return 0;
}

int destroy_cpu(CPU* cpu) {
    if (cpu == nullptr) {
        return -1;
    }
    free(cpu->name);
    free(cpu->Memory);
    return 0;
}

int runFile(const char* filename, CPU* cpu) {
    FILE* file = fopen(filename, "r");
    if (file == nullptr) {
        perror("Error opening file");
        return -1;
    }

    // Read the file line by line and process it
    char line[256];
    std::string source_code;
    while (fgets(line, sizeof(line), file)) {
        // Process each line of the file
        source_code += line;
    }
    fclose(file);
    InstructionArray* instructions = parse_instructions(source_code.c_str());
    if (!instructions) {
        fprintf(stderr, "Failed to parse instructions.\n");
        return -1;
    }
    free_instruction_array(instructions); // Free the allocated memory for instructions
    free_instruction_array(instructions);
    return 0;
}

int execute_instruction(Instruction* instruction, CPU* cpu) {
    if (instruction == nullptr || cpu == nullptr) {
        return -1;
    }

    switch (instruction->opcode) {
        case MOV:
            return mov_instruction(instruction, cpu);
        case ADD:
            return add_instruction(instruction, cpu);
        case SUB:
            return sub_instruction(instruction, cpu);
        case JMP:
            return jmp_instruction(instruction, cpu);
        case HLT:
            return hlt_instruction(instruction, cpu);
        case OUT: {
            // OUT port value
            int port = instruction->operands[0];
            int value = instruction->operands[1];

            if (port == 1) {
                // Output to stdout
                if (instruction->operand_types[1] == REGISTER) {
                    printf("%lld\n", cpu->Registers[value]);
                } else if (instruction->operand_types[1] == IMMEDIATE) {
                    printf("%lld\n", value);
                } else {
                    fprintf(stderr, "Error: Unsupported operand type for OUT instruction\n");
                    return -1;
                }
            } else if (port == 2) {
                // Output to stderr
                if (instruction->operand_types[1] == REGISTER) {
                    fprintf(stderr, "%lld\n", cpu->Registers[value]);
                } else if (instruction->operand_types[1] == IMMEDIATE) {
                    fprintf(stderr, "%lld\n", value);
                } else {
                    fprintf(stderr, "Error: Unsupported operand type for OUT instruction\n");
                    return -1;
                }
            } else {
                fprintf(stderr, "Error: Unsupported port for OUT instruction\n");
                return -1;
            }
            break;
        }
        default:
            printf("Unknown opcode: %d\n", instruction->opcode);
            return -1;
    }

    return 0;
}

int execute_program(CPU* cpu, InstructionArray* program) {
    if (cpu == nullptr || program == nullptr) {
        return -1;
    }

    cpu->instruction_pointer = 0;
    while (cpu->instruction_pointer < program->count) {
        Instruction* instruction = &program->instructions[cpu->instruction_pointer];
        int result = execute_instruction(instruction, cpu);
        if (result == 1) {
            break; // Halt
        } else if (result == 2) {
            // JMP
            continue;
        } else if (result == 0) {
            cpu->instruction_pointer++;
        } else {
            return -1; // Error
        }
    }

    return 0;
}

// Implementations for instruction handlers
int mov_instruction(Instruction* instruction, CPU* cpu) {
    cpu->Registers[instruction->operands[0]] = cpu->Registers[instruction->operands[1]];
    return 0;
}

int add_instruction(Instruction* instruction, CPU* cpu) {
    cpu->Registers[instruction->operands[0]] += cpu->Registers[instruction->operands[1]];
    return 0;
}

int sub_instruction(Instruction* instruction, CPU* cpu) {
    cpu->Registers[instruction->operands[0]] -= cpu->Registers[instruction->operands[1]];
    return 0;
}

int jmp_instruction(Instruction* instruction, CPU* cpu) {
    cpu->instruction_pointer = instruction->operands[0];
    return 2; // Special return to indicate instruction pointer change
}

int hlt_instruction(Instruction* instruction, CPU* cpu) {
    return 1; // Special return to indicate halt
}

void free_instruction_array(InstructionArray* instructions) {
    if (instructions) {
        free(instructions->instructions);
        free(instructions);
    }
}

#endif // __CPUCPP
