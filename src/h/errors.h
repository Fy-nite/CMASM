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
// Function to free a MASMException
void free_masm_exception(MASMException* exception) {
    if (exception != NULL) {
        free(exception->message);
        free(exception->file);
        free(exception->function);
        free(exception->context);
        free(exception->source);
        free(exception);
    }
}