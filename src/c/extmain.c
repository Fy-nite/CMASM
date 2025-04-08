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

    // Test case: Simple MicroASM program with clear label ordering
    const char* test_program = 
        "#include \"stdio.print\"\n"  // Include the stdio print functions
        "lbl main\n"
        "MOV RAX 1\n"
        "MOV RBX 100\n" // setup the mem
        "DB $100 \"Hello world\"\n" // Define a byte string
        "CALL #printf\n" // Call the printf function
        "hlt\n";

    printf("Test Program:\n%s\n", test_program);

    // Create CPU
    CPU cpu;
    create_cpu_wrapper(&cpu, "test_cpu", 100);

    // Parse instructions directly
    InstructionArray* instructions = parse_instructions(test_program);
    if (!instructions) {
        fprintf(stderr, "Failed to parse instructions.\n");
    } else {
        // Execute the parsed instructions
        execute_program_wrapper(&cpu, instructions);
        free_instruction_array(instructions);
    }

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