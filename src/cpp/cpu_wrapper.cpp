#include "../h/cpu.h"

extern "C" {
    int create_cpu_wrapper(CPU* cpu, const char* name, int64_t instruction_count) {
        return create_cpu(cpu, name, instruction_count);
    }

    int destroy_cpu_wrapper(CPU* cpu) {
        return destroy_cpu(cpu);
    }

    int execute_program_wrapper(CPU* cpu, InstructionArray* program) {
        return execute_program(cpu, program);
    }

    int runFile_wrapper(const char* filename, CPU* cpu) {
        return runFile(filename, cpu);
    }
}
