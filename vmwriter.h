#ifndef VMWRITER_H
#define VMWRITER_H

#include <stdio.h>
#include "symboltable.h"

typedef enum vmsegment
{
    VMS_NONE, // default
    VMS_CONST,
    VMS_ARG,
    VMS_LOCAL,
    VMS_STATIC,
    VMS_THIS,
    VMS_THAT,
    VMS_POINTER,
    VMS_TEMP
} vmsegment;

typedef enum vmcommand
{
    VMC_NONE, // default
    VMC_ADD,
    VMC_SUB,
    VMC_NEG,
    VMC_EQ,
    VMC_GT,
    VMC_LT,
    VMC_AND,
    VMC_OR,
    VMC_NOT,
    VMC_MULT, // these are not basic vmcommands and must be converted to OS functions
    VMC_DIV   // they are included here because * and / are in the list of recognized Jack operators
} vmcommand;


// book API functions
void write_push(FILE* outfile, vmsegment segment, int index);
void write_pop(FILE* outfile, vmsegment segment, int index);
void write_arithmetic(FILE* outfile, vmcommand command);
void write_label(FILE* outfile, const char* label);
void write_goto(FILE* outfile, const char* label);
void write_if(FILE* outfile, const char* label);
void write_call(FILE* outfile, const char* name, int numargs);
void write_function(FILE* outfile, const char* name, int numlocals);
void write_return(FILE* outfile);

// my functions
char* convert_vmcommand_to_string(vmcommand command);
char* convert_vmsegment_to_string(vmsegment segment);
vmcommand convert_unary_operator_to_vmcommand(char op);
vmcommand convert_binary_operator_to_vmcommand(char op);
vmsegment convert_symbolkind_to_vmsegment(symbolkind kind);

#endif // VMWRITER_H
