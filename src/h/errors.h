#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// MASMException

// MASMException is a custom exception type for the MASM assembler.
typedef struct {
    char* message; // Error message
    int code;     // Error code
    int line;     // Line number where the error occurred
    int column;   // Column number where the error occurred
    char* file;   // File name where the error occurred
    char* function; // Function name where the error occurred
    char* context; // Context of the error
    char* source;  // Source code where the error occurred
} MASMException;

// Function to create a new MASMException
MASMException* create_masm_exception(const char* message, int code, int line, int column, const char* file, const char* function, const char* context, const char* source) {
    MASMException* exception = (MASMException*)malloc(sizeof(MASMException));
    if (exception == NULL) {
        fprintf(stderr, "Memory allocation failed for MASMException\n");
        exit(EXIT_FAILURE);
    }
    exception->message = strdup(message);
    exception->code = code;
    exception->line = line;
    exception->column = column;
    exception->file = strdup(file);
    exception->function = strdup(function);
    exception->context = strdup(context);
    exception->source = strdup(source);
    return exception;
}
// Function to free and exit the program with a masm exception
void free_masm_exception(MASMException* exception) {
    if (exception != NULL) {
        free(exception->message);
        free(exception->file);
        free(exception->function);
        free(exception->context);
        free(exception->source);
        free(exception);
    }
    exit(EXIT_FAILURE);
}
// Function to print the MASMException
void print_masm_exception(MASMException* exception) {
    if (exception != NULL) {
        fprintf(stderr, "\033[31m"); // Set text color to red
        fprintf(stderr, "MASMException: %s\n", exception->message);
        fprintf(stderr, "Error code: %d\n", exception->code);
        fprintf(stderr, "Line: %d\n", exception->line);
        fprintf(stderr, "Column: %d\n", exception->column);
        fprintf(stderr, "File: %s\n", exception->file);
        fprintf(stderr, "Function: %s\n", exception->function);
        fprintf(stderr, "Context: %s\n", exception->context);
        fprintf(stderr, "Source: %s\n", exception->source);
        fprintf(stderr, "\033[0m"); // Reset text color
    }
}
// horror execption
typedef struct {
    char* message; // Error message
    int code;     // Error code
    char* stack_trace; // Stack trace
    char* context; // Context of the error
    char* source;  // Source code where the error occurred
} HorrorException;

// Function to create a new HorrorException
HorrorException* create_horror_exception(const char* message, int code, const char* stack_trace, const char* context, const char* source) {
    HorrorException* exception = (HorrorException*)malloc(sizeof(HorrorException));
    if (exception == NULL) {
        fprintf(stderr, "Memory allocation failed for HorrorException\n");
        exit(EXIT_FAILURE);
    }
    exception->message = strdup(message);
    exception->code = code;
    exception->stack_trace = strdup(stack_trace);
    exception->context = strdup(context);
    exception->source = strdup(source);
    return exception;
}
// Function to print the HorrorException with an ASCII art of an explosion, free and exit
void print_horror_exception_and_exit(HorrorException* exception) {
    if (exception != NULL) {
        fprintf(stderr, "\033[31m"); // Set text color to red
        fprintf(stderr, "HorrorException: %s\n", exception->message);
        fprintf(stderr, "Error code: %d\n", exception->code);
        fprintf(stderr, "Stack trace: %s\n", exception->stack_trace);
        fprintf(stderr, "Context: %s\n", exception->context);
        fprintf(stderr, "Source: %s\n", exception->source);
        fprintf(stderr, "\n");
        fprintf(stderr, "      BOOM!      \n");
        fprintf(stderr, "     /     \\     \n");
        fprintf(stderr, "    /       \\    \n");
        fprintf(stderr, "   /         \\   \n");
        fprintf(stderr, "  /           \\  \n");
        fprintf(stderr, " /             \\ \n");
        fprintf(stderr, "*****************\n");
        fprintf(stderr, "\033[0m"); // Reset text color
    }
    if (exception != NULL) {
        free(exception->message);
        free(exception->stack_trace);
        free(exception->context);
        free(exception->source);
        free(exception);
    }
    exit(EXIT_FAILURE);
}
