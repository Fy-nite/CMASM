#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <stdbool.h>

// Add necessary includes for readlink on Unix systems
#ifdef __linux__
#include <unistd.h>
#endif

#include "../h/instruction.h"

// Forward declarations
int get_register_enum(const char* reg);
InstructionArray* parse_instructions(const char* source_code);
InstructionArray* parse_code(const char* source_code);

InstructionArray *create_instruction_array(int size) {
    InstructionArray* instruction_array = (InstructionArray*)malloc(sizeof(InstructionArray));
    if (instruction_array == NULL) {
        fprintf(stderr, "Memory allocation failed for InstructionArray\n");
        return NULL;
    }
    instruction_array->instructions = (Instruction*)malloc(sizeof(Instruction) * size);
    if (instruction_array->instructions == NULL) {
        fprintf(stderr, "Memory allocation failed for instructions\n");
        free(instruction_array);
        return NULL;
    }
    instruction_array->size = size;
    instruction_array->count = 0; // Initialize count to 0
    return instruction_array;
}

void free_instruction_array(InstructionArray* instruction_array) {
    if (instruction_array != NULL) {
        for (int i = 0; i < instruction_array->count; i++) {
            //free(instruction_array->instructions[i].operand_types);
        }
        free(instruction_array->instructions);
        free(instruction_array);
    }
}

// Helper function to map a string opcode to the Instructions enum
int get_instruction_opcode(const char* opcode) {
    if (strcasecmp(opcode, "MOV") == 0) return MOV;
    if (strcasecmp(opcode, "ADD") == 0) return ADD;
    if (strcasecmp(opcode, "SUB") == 0) return SUB;
    if (strcasecmp(opcode, "MUL") == 0) return MUL;
    if (strcasecmp(opcode, "DIV") == 0) return DIV;
    if (strcasecmp(opcode, "JMP") == 0) return JMP;
    if (strcasecmp(opcode, "HLT") == 0) return HLT;
    if (strcasecmp(opcode, "LBL") == 0) return NONE; // Labels are not instructions
    if (strcasecmp(opcode, "NOP") == 0) return NOP;
    if (strcasecmp(opcode, "DB") == 0) return DB;    // Add support for DB instruction
    if (strcasecmp(opcode, "CALL") == 0) return CALL; // Add support for CALL instruction
    if (strcasecmp(opcode, "RET") == 0) return RET;   // Add support for RET instruction
    if (strncasecmp(opcode, "#include", 8) == 0) return NONE; // Include directives are not instructions
    if (strcasecmp(opcode, "OUT") == 0) return OUT;   // Add support for OUT instruction
    if (strcasecmp(opcode, "COUT") == 0) return COUT; // Add support for COUT instruction
    return -1; // Invalid opcode
}

// Helper function to convert dot notation to directory path
char* convert_dot_path(const char* dot_path) {
    char* dir_path = strdup(dot_path);
    if (!dir_path) {
        return NULL;
    }
    
    // Replace dots with directory separators
    for (char* p = dir_path; *p; p++) {
        if (*p == '.') {
            *p = '/';  // Use '/' for Unix-like systems, or '\\' for Windows
        }
    }
    
    return dir_path;
}

// Global buffer to store accumulated included content
static char* included_content = NULL;
static size_t included_content_size = 0;

// To prevent infinite recursion when including files
static bool currently_processing_includes = false;

// Global variables to keep track of collected labels from included files
static MASMLabels included_labels = {0};
static bool label_collection_initialized = false;

// Helper function to initialize the included_labels structure
void init_included_labels() {
    if (!label_collection_initialized) {
        included_labels.labels = (MASMLabel*)malloc(sizeof(MASMLabel) * 10);
        included_labels.label_count = 0;
        included_labels.label_capacity = 10;
        label_collection_initialized = true;
    }
}

// Helper function to add a label to the included_labels
void add_included_label(const char* name, int instruction_index) {
    init_included_labels();
    
    // Check if label already exists
    for (int i = 0; i < included_labels.label_count; i++) {
        if (strcmp(included_labels.labels[i].name, name) == 0) {
            return; // Label already exists, don't add it again
        }
    }
    
    // Add the label
    if (included_labels.label_count >= included_labels.label_capacity) {
        included_labels.label_capacity *= 2;
        included_labels.labels = (MASMLabel*)realloc(included_labels.labels, 
                                  sizeof(MASMLabel) * included_labels.label_capacity);
    }
    
    included_labels.labels[included_labels.label_count].name = strdup(name);
    included_labels.labels[included_labels.label_count].instruction_index = instruction_index;
    included_labels.label_count++;
}

// Helper function to process an include directive
bool process_include_directive(const char* include_line) {
    // Skip "#include" and any whitespace
    const char* path_start = include_line + 8; // Move past "#include"
    while (isspace(*path_start)) path_start++;
    
    // Check if the path is enclosed in quotes
    if (*path_start == '"') {
        path_start++; // Skip opening quote
        const char* path_end = strchr(path_start, '"');
        if (path_end) {
            int path_len = path_end - path_start;
            char path[256]; // Assuming max path length of 255
            strncpy(path, path_start, path_len);
            path[path_len] = '\0';
            
            // Convert dot notation to directory path
            char* dir_path = convert_dot_path(path);
            if (!dir_path) {
                fprintf(stderr, "Memory allocation failed for include path conversion\n");
                return false;
            }
            
            // Try different file extensions and locations
            FILE* include_file = NULL;
            char full_path[512];
            char* extensions[] = {"", ".masm", ".mas"};
            int num_extensions = sizeof(extensions) / sizeof(extensions[0]);
            
            // Get executable directory
            char exe_path[512] = {0};
            #ifdef _WIN32
            GetModuleFileName(NULL, exe_path, sizeof(exe_path));
            char* last_slash = strrchr(exe_path, '\\');
            if (last_slash) *last_slash = '\0';
            #else
            // For Unix-like systems
            #ifdef __linux__
            readlink("/proc/self/exe", exe_path, sizeof(exe_path));
            #else
            // Fallback for other systems
            strcpy(exe_path, ".");
            #endif
            char* last_slash = strrchr(exe_path, '/');
            if (last_slash) *last_slash = '\0';
            #endif
            
            // Try each extension with each possible path
            for (int i = 0; i < num_extensions; i++) {
                // 1. Try include directory
                snprintf(full_path, sizeof(full_path), "include/%s%s", dir_path, extensions[i]);
                printf("Trying to open %s...\n", full_path);
                include_file = fopen(full_path, "r");
                if (include_file) break;
                
                // 2. Try src/include directory
                snprintf(full_path, sizeof(full_path), "src/include/%s%s", dir_path, extensions[i]);
                printf("Trying to open %s...\n", full_path);
                include_file = fopen(full_path, "r");
                if (include_file) break;
                
                // 3. Try stdlib directory
                snprintf(full_path, sizeof(full_path), "include/stdlib/%s%s", dir_path, extensions[i]);
                printf("Trying to open %s...\n", full_path);
                include_file = fopen(full_path, "r");
                if (include_file) break;
                
                // 4. Try lib directory
                snprintf(full_path, sizeof(full_path), "lib/%s%s", dir_path, extensions[i]);
                printf("Trying to open %s...\n", full_path);
                include_file = fopen(full_path, "r");
                if (include_file) break;
                
                // 5. Try executable directory
                snprintf(full_path, sizeof(full_path), "%s/%s%s", exe_path, dir_path, extensions[i]);
                printf("Trying to open %s...\n", full_path);
                include_file = fopen(full_path, "r");
                if (include_file) break;
                
                // 6. Try current directory
                snprintf(full_path, sizeof(full_path), "%s%s", dir_path, extensions[i]);
                printf("Trying to open %s...\n", full_path);
                include_file = fopen(full_path, "r");
                if (include_file) break;
            }
            
            if (include_file) {
                // Read the file content
                fseek(include_file, 0, SEEK_END);
                long file_size = ftell(include_file);
                fseek(include_file, 0, SEEK_SET);
                
                char* file_content = (char*)malloc(file_size + 1);
                if (!file_content) {
                    fprintf(stderr, "Memory allocation failed for include file content\n");
                    fclose(include_file);
                    free(dir_path);
                    return false;
                }
                
                size_t read_size = fread(file_content, 1, file_size, include_file);
                file_content[read_size] = '\0';
                fclose(include_file);
                
                // Print success message
                printf("Including file: %s (found at %s)\n", path, full_path);
                
                // Analyze included file for labels before adding its content
                char* file_copy = strdup(file_content);
                if (file_copy) {
                    char* line = strtok(file_copy, "\n");
                    int local_instruction_index = 0;
                    
                    while (line) {
                        // Skip empty lines and comments
                        char* trimmed = line;
                        while (isspace(*trimmed)) trimmed++;
                        if (*trimmed == '\0' || *trimmed == ';') {
                            line = strtok(NULL, "\n");
                            continue;
                        }
                        
                        // Check if the line is a label definition
                        if (strncasecmp(trimmed, "LBL", 3) == 0) {
                            // Extract label name
                            char* label_name = trimmed + 3;
                            while (isspace(*label_name)) label_name++;
                            char* label_end = label_name;
                            while (*label_end != '\0' && !isspace(*label_end)) label_end++;
                            int label_len = label_end - label_name;
                            char name[32]; // Assuming max label length of 31 characters
                            strncpy(name, label_name, label_len);
                            name[label_len] = '\0';
                            
                            // Add to included labels, offset by current instruction count
                            add_included_label(name, local_instruction_index);
                        } else if (strncasecmp(trimmed, "#include", 8) != 0) {
                            // Only increment for actual instructions
                            local_instruction_index++;
                        }
                        
                        line = strtok(NULL, "\n");
                    }
                    
                    free(file_copy);
                }
                
                // Accumulate the included content in our global buffer
                if (!included_content) {
                    included_content = (char*)malloc(read_size + 1);
                    if (!included_content) {
                        fprintf(stderr, "Memory allocation failed for included content buffer\n");
                        free(file_content);
                        free(dir_path);
                        return false;
                    }
                    strcpy(included_content, file_content);
                    included_content_size = read_size;
                } else {
                    included_content = (char*)realloc(included_content, 
                                                     included_content_size + read_size + 1);
                    if (!included_content) {
                        fprintf(stderr, "Memory reallocation failed for included content buffer\n");
                        free(file_content);
                        free(dir_path);
                        return false;
                    }
                    // Add a newline between includes to avoid issues
                    strcat(included_content, "\n");
                    strcat(included_content, file_content);
                    included_content_size += read_size + 1; // +1 for the newline
                }
                
                // Don't attempt to separately parse the included file
                // We will include its content in the main source code instead
                
                free(file_content);
                free(dir_path);
                return true;
            } else {
                fprintf(stderr, "Error: Could not open include file: %s\n", path);
                free(dir_path);
                return false;
            }
        }
    }
    
    fprintf(stderr, "Error: Invalid include directive: %s\n", include_line);
    return false;
}

// Helper function to find a label by name
int find_label(MASMLabels* labels, const char* name) {
    // First check in main program labels
    for (int i = 0; i < labels->label_count; i++) {
        if (strcmp(labels->labels[i].name, name) == 0) {
            return labels->labels[i].instruction_index;
        }
    }
    
    // Then check in included labels
    for (int i = 0; i < included_labels.label_count; i++) {
        if (strcmp(included_labels.labels[i].name, name) == 0) {
            return included_labels.labels[i].instruction_index;
        }
    }
    
    return -1; // Label not found
}

// Tokenize a single line of assembly code into an Instruction
bool tokenize_instruction(const char* line, Instruction* instruction, int* operands_array, int* operand_types_array, MASMLabels* labels) {
    char* line_copy = strdup(line);
    if (!line_copy) {
        fprintf(stderr, "Memory allocation failed for line copy\n");
        return false;
    }

    // Find the first space to separate opcode from operands
    char* space = strchr(line_copy, ' ');
    if (!space) {
        // No space found, just an opcode with no operands
        int opcode = get_instruction_opcode(line_copy);
        if (opcode == -1) {
            fprintf(stderr, "Unknown opcode: %s\n", line_copy);
            free(line_copy);
            return false;
        }
        instruction->opcode = opcode;
        instruction->operand_count = 0; // No operands for instructions like RET, HLT, etc.
        free(line_copy);
        return true;
    }

    // Temporarily terminate the string at the space to extract opcode
    *space = '\0';
    int opcode = get_instruction_opcode(line_copy);
    if (opcode == -1) {
        fprintf(stderr, "Unknown opcode: %s\n", line_copy);
        free(line_copy);
        return false;
    }
    instruction->opcode = opcode;

    // Special handling for CALL instruction - similar to JMP but needs to handle both labels and external functions
    if (opcode == CALL) {
        // Now get the operands (skip any extra spaces)
        char* operands_start = space + 1;
        while (*operands_start == ' ') operands_start++;
        // Check if it's a label (prefixed with #) or an external function (prefixed with $)
        if (operands_start[0] == '#') {
            // Label - calls an internal function defined with LBL
            const char* label_name = operands_start + 1;
            int label_index = find_label(labels, label_name);
            if (label_index != -1) {
                operands_array[0] = label_index; // Store instruction index
                operand_types_array[0] = LABEL;
            } else {
                fprintf(stderr, "Error: Undefined label '%s' in CALL instruction\n", label_name);
                free(line_copy);
                return false;
            }
        } else if (operands_start[0] == '$') {
            // External function (MNI function) - not defined within the program
            operands_array[0] = (intptr_t)strdup(operands_start + 1); // Skip $ and store function name
            operand_types_array[0] = MNIFUNC; // Using MNIFUNC type for external functions
        } else {
            // Check if it's a register
            int reg_enum = get_register_enum(operands_start);
            if (reg_enum != -1) {
                operands_array[0] = reg_enum;
                operand_types_array[0] = REGISTER;
            } else {
                // Assume it's an immediate value
                operands_array[0] = atoi(operands_start);
                operand_types_array[0] = IMMEDIATE;
            }
        }
        // Set the operand count to 1 (just the target)
        instruction->operand_count = 1;
        free(line_copy);
        return true;
    }

    // Special handling for DB instruction
    if (opcode == DB) {
        // Now get the operands (skip any extra spaces)
        char* operands_start = space + 1;
        while (*operands_start == ' ') operands_start++;
        // Handle first operand (address)
        char* address_end = strchr(operands_start, ' ');
        if (!address_end) {
            fprintf(stderr, "Error: DB instruction requires an address and a string\n");
            free(line_copy);
            return false;
        }
        *address_end = '\0';
        // Check if it's a memory address with $ prefix
        if (operands_start[0] == '$') {
            operands_array[0] = atoi(operands_start + 1); // Skip the $ and convert to integer
            operand_types_array[0] = IMMEDIATE;
        } else {
            // If it's a register
            int reg_enum = get_register_enum(operands_start);
            if (reg_enum != -1) {
                operands_array[0] = reg_enum;
                operand_types_array[0] = REGISTER;
            } else {
                operands_array[0] = atoi(operands_start); // Try to convert as a number
                operand_types_array[0] = IMMEDIATE;
            }
        }
        // Move to the string part
        char* string_start = address_end + 1;
        while (*string_start == ' ') string_start++;
        // Check if the string is enclosed in quotes
        if (*string_start == '"') {
            string_start++; // Skip opening quote
            char* string_end = strrchr(string_start, '"');
            if (string_end) {
                *string_end = '\0'; // Terminate the string at the closing quote
                
                // For DB, we'll store a pointer to the string in operands[1]
                // This is a simplified approach - in a real implementation,
                // you'd need to handle memory management for the string
                operands_array[1] = (intptr_t)strdup(string_start);
                operand_types_array[1] = IMMEDIATE; // Using IMMEDIATE for simplicity
                
                // Set the operand count to 2 (address and string)
                instruction->operand_count = 2;
                free(line_copy);
                return true;
            }
        }
        fprintf(stderr, "Error: Invalid string format in DB instruction\n");
        free(line_copy);
        return false;
    }
    
    // Now get the operands (skip any extra spaces)
    char* operands_start = space + 1;
    while (*operands_start == ' ') operands_start++;
    int i = 0;
    char* operand_ptr = operands_start;
    while (*operand_ptr != '\0' && i < 4) {
        // Skip leading whitespace
        while (isspace(*operand_ptr)) operand_ptr++;
        // Find the end of the operand
        char* operand_end = operand_ptr;
        while (*operand_end != '\0' && !isspace(*operand_end) && *operand_end != ',') operand_end++;
        // Calculate the length of the operand
        int operand_len = operand_end - operand_ptr;
        // Copy the operand into a temporary buffer
        char operand[32]; // Assuming max operand length of 31 characters
        strncpy(operand, operand_ptr, operand_len);
        operand[operand_len] = '\0';
        // Check if the operand is a register
        int reg_enum = get_register_enum(operand);
        if (reg_enum != -1) {
            operands_array[i] = reg_enum;
            operand_types_array[i] = REGISTER;
        } else {
            // Otherwise, treat it as an immediate value or label
            if (operand[0] == '#') {
                // Label
                const char* label_name = operand + 1;
                int label_index = find_label(labels, label_name);
                if (label_index != -1) {
                    operands_array[i] = label_index; // Store instruction index
                    operand_types_array[i] = LABEL;
                } else {
                    fprintf(stderr, "Error: Undefined label '%s'\n", label_name);
                    free(line_copy);
                    return false;
                }
            } else {
                operands_array[i] = atoi(operand); // Convert to integer
                operand_types_array[i] = IMMEDIATE;
            }
        }
        // Move to the next operand
        operand_ptr = operand_end;
        
        if (*operand_ptr == ',') operand_ptr++; // Skip comma
        i++;
    }
    instruction->operand_count = i;
    free(line_copy);
    return true;
}

// Create a completely new version of parse_instructions with a cleaner approach
InstructionArray* parse_instructions(const char* source_code) {
    // Clean up any previous state
    static bool first_call = true;
    
    // We'll store all included files' content here
    static char* global_includes = NULL;
    
    if (first_call) {
        // Only initialize on first call
        first_call = false;
        
        // Copy source code for processing includes
        char* includes_copy = strdup(source_code);
        if (!includes_copy) {
            fprintf(stderr, "Memory allocation failed for source copy\n");
            return NULL;
        }
        
        // First, extract all include directives and process them
        char* line = strtok(includes_copy, "\n");
        bool any_includes = false;
        
        while (line) {
            // Trim whitespace
            char* trimmed = line;
            while (isspace(*trimmed)) trimmed++;
            
            // Process includes
            if (strncasecmp(trimmed, "#include", 8) == 0) {
                any_includes = true;
                process_include_directive(trimmed);
            }
            
            line = strtok(NULL, "\n");
        }
        
        free(includes_copy);
        
        // If we have any includes, combine them with the source
        if (any_includes && included_content) {
            size_t source_len = strlen(source_code);
            size_t include_len = strlen(included_content);
            
            // Store includes for cleanup later
            global_includes = included_content;
            
            // Create a combined source with includes first, then original code
            char* combined = (char*)malloc(include_len + source_len + 2); // +2 for newline and null
            if (!combined) {
                fprintf(stderr, "Memory allocation failed for combined source\n");
                return NULL;
            }
            
            strcpy(combined, included_content);
            strcat(combined, "\n");
            strcat(combined, source_code);
            
            // Process the combined source
            InstructionArray* result = parse_code(combined);
            
            // Clean up
            free(combined);
            free(global_includes);
            global_includes = NULL;
            included_content = NULL;
            included_content_size = 0;
            
            // Reset for next call
            first_call = true;
            
            return result;
        } else {
            // No includes, just parse the original code
            return parse_code(source_code);
        }
    } else {
        // This is a recursive or subsequent call, just parse the code directly
        return parse_code(source_code);
    }
}

// Helper function to parse the actual code after handling includes
InstructionArray* parse_code(const char* source_code) {
    // Create the instruction array
    InstructionArray* instruction_array = create_instruction_array(100); // Initial size
    if (!instruction_array) {
        return NULL;
    }
    
    // Initialize labels array
    MASMLabels labels = {0};
    labels.labels = (MASMLabel*)malloc(sizeof(MASMLabel) * 10); // Initial capacity
    if (!labels.labels) {
        fprintf(stderr, "Memory allocation failed for labels\n");
        free_instruction_array(instruction_array);
        return NULL;
    }
    labels.label_count = 0;
    labels.label_capacity = 10;
    
    // Make a copy of the source code for processing
    char* code_copy = strdup(source_code);
    if (!code_copy) {
        fprintf(stderr, "Memory allocation failed for source code copy\n");
        free_instruction_array(instruction_array);
        free(labels.labels);
        return NULL;
    }
    
    // First pass: Collect all labels
    // This will give us a complete label map before processing instructions
    char* pass1 = strdup(code_copy);
    if (!pass1) {
        fprintf(stderr, "Memory allocation failed for first pass copy\n");
        free(code_copy);
        free_instruction_array(instruction_array);
        free(labels.labels);
        return NULL;
    }
    
    int instruction_index = 0;
    char* line = strtok(pass1, "\n");
    
    while (line) {
        // Skip empty lines, comments, and includes
        char* trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        
        if (*trimmed == '\0' || *trimmed == ';' || strncasecmp(trimmed, "#include", 8) == 0) {
            line = strtok(NULL, "\n");
            continue;
        }
        
        // Check for label definitions
        if (strncasecmp(trimmed, "LBL", 3) == 0) {
            // Extract label name
            char* label_name = trimmed + 3;
            while (isspace(*label_name)) label_name++;
            char* label_end = label_name;
            while (*label_end && !isspace(*label_end)) label_end++;
            
            int label_len = label_end - label_name;
            char name[64] = {0}; // Larger buffer for safety
            strncpy(name, label_name, label_len);
            
            // Check for duplicate labels
            for (int i = 0; i < labels.label_count; i++) {
                if (strcmp(labels.labels[i].name, name) == 0) {
                    fprintf(stderr, "Warning: Duplicate label '%s' - using first definition\n", name);
                    line = strtok(NULL, "\n");
                    goto next_line;  // Skip this duplicate label
                }
            }
            
            // Add label to the array
            if (labels.label_count >= labels.label_capacity) {
                labels.label_capacity *= 2;
                labels.labels = (MASMLabel*)realloc(labels.labels, 
                                              sizeof(MASMLabel) * labels.label_capacity);
                if (!labels.labels) {
                    fprintf(stderr, "Memory reallocation failed for labels\n");
                    free(pass1);
                    free(code_copy);
                    free_instruction_array(instruction_array);
                    return NULL;
                }
            }
            
            labels.labels[labels.label_count].name = strdup(name);
            labels.labels[labels.label_count].instruction_index = instruction_index;
            labels.label_count++;
        } else {
            // Only increment for actual instructions (not labels, includes, etc.)
            instruction_index++;
        }
        
next_line:
        line = strtok(NULL, "\n");
    }
    
    free(pass1);
    
    // Debug output - print all collected labels
    printf("Collected %d labels:\n", labels.label_count);
    for (int i = 0; i < labels.label_count; i++) {
        printf("  Label: %s at instruction %d\n", 
               labels.labels[i].name, labels.labels[i].instruction_index);
    }
    
    // Second pass: Now process the instructions
    instruction_index = 0;
    char* line_start = code_copy;
    char* next_line;
    
    while (line_start && *line_start) {
        // Find the end of the current line
        next_line = strchr(line_start, '\n');
        if (next_line) {
            *next_line = '\0'; // Temporarily null-terminate this line
            next_line++; // Move to start of next line
        }
        
        // Skip empty lines, comments, and includes/labels
        char* trimmed = line_start;
        while (isspace(*trimmed)) trimmed++;
        
        if (*trimmed && *trimmed != ';' && 
            strncasecmp(trimmed, "#include", 8) != 0 && 
            strncasecmp(trimmed, "LBL", 3) != 0) {
            
            // Parse this instruction
            Instruction instruction = {0};
            int operands_array[4] = {0};
            int operand_types_array[4] = {0};
            
            if (!tokenize_instruction(trimmed, &instruction, operands_array, operand_types_array, &labels)) {
                fprintf(stderr, "Error parsing instruction: %s\n", trimmed);
                free(code_copy);
                free_instruction_array(instruction_array);
                
                // Free label names
                for (int i = 0; i < labels.label_count; i++) {
                    free(labels.labels[i].name);
                }
                free(labels.labels);
                return NULL;
            }
            
            // Copy operands to instruction
            for (int i = 0; i < 4; i++) {
                instruction.operands[i] = operands_array[i];
                instruction.operand_types[i] = operand_types_array[i];
            }
            
            // Add instruction to array
            if (instruction_array->count >= instruction_array->size) {
                instruction_array->size *= 2;
                instruction_array->instructions = (Instruction*)realloc(
                    instruction_array->instructions, 
                    sizeof(Instruction) * instruction_array->size);
                
                if (!instruction_array->instructions) {
                    fprintf(stderr, "Memory reallocation failed for instructions\n");
                    free(code_copy);
                    free_instruction_array(instruction_array);
                    
                    // Free label names
                    for (int i = 0; i < labels.label_count; i++) {
                        free(labels.labels[i].name);
                    }
                    free(labels.labels);
                    return NULL;
                }
            }
            
            instruction_array->instructions[instruction_array->count++] = instruction;
            instruction_index++;
        }
        
        line_start = next_line; // Move to next line
    }
    
    free(code_copy);
    
    // Free label names
    for (int i = 0; i < labels.label_count; i++) {
        free(labels.labels[i].name);
    }
    free(labels.labels);
    
    return instruction_array;
}

