#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>


// if your function's names are a array, your names will be joined with a dot
// example: ["foo", "bar"] -> "foo.bar"
// this is because of how MNI Functions work internally and in the languages
bool register_mni_function(const char *name, void *function);
// this returns a pointer to a function that is registered with the name, yes this does include the dots in the name
void *get_mni_function(const char *name);
// your crappy code runs in this function
bool execute_mni_function(const char *name, void *args, void *result);
