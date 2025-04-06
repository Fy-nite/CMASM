// ExtMain for CMASM

#ifndef _EXTMAIN_C
#define _EXTMAIN_C
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../h/cpu.h"
#include "../h/instruction.h"
#include "memory.h"
#include "../h/errors.h"
#include "../headers/mni.h"

InstructionArray *create_instruction_array(int size);
InstructionArray* parse_instructions(const char* source_code);
void free_instruction_array(InstructionArray* instruction_array);

void run_tests() {
    printf("Running tests...\n");

    // Test case: Simple MicroASM program
    const char* test_program = 
        "MOV RAX RBX\n"
        "ADD RAX R1\n"
        "JMP #label\n"
        "LBL label\n"
        "HLT\n";

    printf("Test Program:\n%s\n", test_program);

    // Create CPU
    CPU cpu;
    create_cpu_wrapper(&cpu, "test_cpu", 100);

    // Execute program
    runFile_wrapper("test.masm", &cpu);

    // Print register values
    printf("Register values:\n");
    printf("RAX: %lld\n", cpu.Registers[RAX]);
    printf("RBX: %lld\n", cpu.Registers[RBX]);

    destroy_cpu_wrapper(&cpu);
    printf("Tests completed.\n");
}

int main(int argc, char** argv) {
    printf("Starting CMASM...\n");

    // Run tests
    run_tests();

    // for now, if this keeps showing, thats good. we haven't broken anything yet.
    // at somepoint i do have to actualy make the code that runs this
    HorrorException* horror = NULL;
    //create a test horror exception
    horror = create_horror_exception("Test horror",1, "test.c", "test context", "test source");
    //print the exception
    print_horror_exception_and_exit(horror);

    return 0;
}

#endif