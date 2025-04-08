#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef MASM_H
#define MASM_H
// max uint
#define MAX_UINT 0xFFFFFFFF

typedef struct {
    int opcode; // Change from char to int to match the Instructions enum
    int operands[4]; // Array to hold operands (assuming max 4 operands)
    int operand_sizes[4];
    int operand_types[4];
    int operand_count;
    int count;
} Instruction;

typedef struct {
    Instruction* instructions;
    int count; // Number of instructions currently in the array
    int size;  // Total allocated size of the array
} InstructionArray;

typedef struct {
    char* name;
    char* value;
} MASMVariable;

typedef struct {
    char* name;
    char* value;
} MASMFunction;

typedef struct {
    MASMVariable* variables;
    int variable_count;
} MASMVariables;

struct interpreter {
    char* code;
    InstructionArray* instructions;
    MASMVariables* variables;
};

typedef struct {
    char* name;
    int instruction_index; // Store the instruction index instead of value
} MASMLabel;

typedef struct {
    MASMLabel* labels;
    int label_count;
    int label_capacity; // Add capacity for dynamic array
} MASMLabels;

struct MASMObject {
    char* name;
    char* code;
    char* source;
    char* file;
    long* memory;
    InstructionArray* instructions;
    MASMVariables* variables;
    MASMLabel* labels;
    MASMFunction* functions;
};

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
    DB = 0x23, // Define Byte
    OUT = 0x24, // Output to console
    COUT = 0x25, // Output to console as char
};

enum Registers {
    RAX = 0,
    RBX = 1,
    RCX = 2,
    RDX = 3,
    RDI = 4,
    RSI = 5,
    RBP = 6,
    RSP = 7,
    R0 = 8,
    R1 = 9,
    R2 = 10,
    R3 = 11,
    R4 = 12,
    R5 = 13,
    R6 = 14,
    R7 = 15,
    R8 = 16,
    R9 = 17,
    R10 = 18,
    R11 = 19,
    R12 = 20,
    R13 = 21,
    R14 = 22,
    R15 = 23,
};

enum OperandTypes {
    REGISTER = 0,
    IMMEDIATE = 1,
    MEMORY = 2,
    LABEL = 3,
    MNIFUNC = 4,
    NONE = 5,
    NOPERATOR = 6,
};


enum OperandSizes {
    BYTE = 0,
    WORD = 1,
    DWORD = 2,
    QWORD = 3,
    ITSLARGEOK = 4,
};

#endif // MASM_H