// ExtMain for CMASM

#ifndef _EXTMAIN_C
#define _EXTMAIN_C
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../h/cpu.h"

#include "memory.h"
#include "../h/errors.h"
#include "../headers/mni.h"


int main(int argc, char** argv) {
    MASMException* exception = NULL;
    //create a test masm exception
    exception = create_masm_exception("Test exception", 1, 1, 1, "test.c", "main", "test context", "test source");
    //print the exception
    print_masm_exception(exception);
    //free the exception
    free_masm_exception(exception);

}





#endif