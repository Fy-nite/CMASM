#ifndef CPU_H
#define CPU_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "instruction.h"

// TODO: Add a header guard
// what is a header guard?

typedef struct {
    char* name;
    Instruction* instructions;
    int64_t instruction_count;
    int64_t instruction_pointer;
    int64_t stack_pointer;
    int64_t base_pointer;
    int64_t* Memory;
    int64_t Registers[24]; // Use a fixed-size array for registers
    
} CPU;

#ifdef __cplusplus
extern "C" {
#endif
    int create_cpu_wrapper(CPU* cpu, const char* name, int64_t instruction_count);
    int destroy_cpu_wrapper(CPU* cpu);
    int execute_program_wrapper(CPU* cpu, InstructionArray* program);
    int runFile_wrapper(const char* filename, CPU* cpu);
    InstructionArray* parse_instructions(const char* source_code);
#ifdef __cplusplus
}
#endif

int create_cpu(CPU* cpu, const char* name, int64_t instruction_count);
int destroy_cpu(CPU* cpu);
int runFile(const char* filename, CPU* cpu);
int64_t get_register_value(CPU* cpu, const char* register_name);
int set_register_value(CPU* cpu, const char* register_name, int64_t value);
int64_t get_memory_value(CPU* cpu, int64_t address);
int set_memory_value(CPU* cpu, int64_t address, int64_t value);
int64_t get_stack_value(CPU* cpu, int64_t offset);
int set_stack_value(CPU* cpu, int64_t offset, int64_t value);
int64_t get_base_value(CPU* cpu, int64_t offset);
int set_base_value(CPU* cpu, int64_t offset, int64_t value);
int64_t get_instruction_pointer(CPU* cpu);
int set_instruction_pointer(CPU* cpu, int64_t value);
int64_t get_stack_pointer(CPU* cpu);

int execute_instruction(Instruction* instruction, CPU* cpu);
int execute_program(CPU* cpu, InstructionArray* program);



#endif // CPU_H