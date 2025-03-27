#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// max uint
#define MAX_UINT 0xFFFFFFFF


typedef struct {
    char opcode;
    char* operands;
    int* operand_sizes;
    int* operand_types;
    int operand_count;
} Instruction;

typedef struct {
    Instruction* instructions;
} InstructionArray;

// Instructions available to use in the cpu
enum Instructions {
    MOV = 0x01,
    ADD = 0x02,
    SUB = 0x03,
    MUL = 0x04,
    DIV = 0x05,
    AND = 0x06,
    OR = 0x07,
    XOR = 0x08,
    NOT = 0x09,
    JMP = 0x0A,
    CMP = 0x0B,
    CALL = 0x0C,
    CALLE = 0x0D,
    CALLNE = 0x0E,
    RET = 0x0F,
    PUSH = 0x10,
    POP = 0x11,
    INC = 0x12,
    DEC = 0x13,
    SHL = 0x14,
    SHR = 0x15,
    NOP = 0x16,
    HLT = 0x17,
    JE = 0x18,
    JNE = 0x19,
    JG = 0x1A,
    JL = 0x1B,
    JGE = 0x1C,
    JLE = 0x1D,
    MNI = 0x1E,
    ENTER = 0x1F,
    LEAVE = 0x20,
    MOVTO = 0x21,
    MOVFROM = 0x22,
};



Instruction create_instruction(char opcodze, const char* operands); // Function to create an instruction    
Instruction parse_instruction(const char* str);// Function to parse an instruction from a string



void execute_instruction(Instruction instruction);// Function to execute an instruction
void print_instruction(Instruction instruction);// Function to print an instruction
int get_instruction_size(Instruction instruction);// Function to get the size of an instruction
int get_operand_count(Instruction instruction);// Function to get the number of operands of an instruction
char get_opcode(Instruction instruction);// Function to get the opcode of an instruction
const char* get_operands(Instruction instruction);// Function to get the operands of an instruction
const char* get_operand(Instruction instruction, int index);// Function to get the operand at a specific index
int get_operand_size(Instruction instruction, int index);// Function to get the size of an operand