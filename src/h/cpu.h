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
    int64_t* Registers;
    
} CPU;


int execute_instruction(Instruction* Instruction, CPU* cpu);
int execute_program(CPU* cpu, InstructionArray* program);



#endif // CPU_H