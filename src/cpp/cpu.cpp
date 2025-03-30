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
    cpu->Registers = (int64_t*)malloc(sizeof(int64_t) * 16); // Assuming 16 registers
    if (cpu->Registers == nullptr) {
        free(cpu->Memory);
        throw std::runtime_error("Memory allocation failed for CPU registers");
    }
    memset(cpu->Registers, 0, sizeof(int64_t) * 16);
    return 0;
}
int destroy_cpu(CPU* cpu) {
    if (cpu == nullptr) {
        return -1;
    }
    free(cpu->name);
    free(cpu->Memory);
    free(cpu->Registers);
    return 0;
}

#endif // __CPUCPP