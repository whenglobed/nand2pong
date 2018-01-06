#include "vmwriter.h"


/*
* Writes a VM push command.
*/
void write_push(FILE* outfile, vmsegment segment, int index)
{
    fprintf(outfile, "push %s %d\n", convert_vmsegment_to_string(segment), index);
}


/*
*  Writes a VM pop command.
*/
void write_pop(FILE* outfile, vmsegment segment, int index)
{
    fprintf(outfile, "pop %s %d\n", convert_vmsegment_to_string(segment), index);
}


/*
*  Writes a VM arithmetic command.
*/
void write_arithmetic(FILE* outfile, vmcommand command)
{
    fprintf(outfile, "%s\n", convert_vmcommand_to_string(command));
}


/*
*  Writes a VM label command.
*/
void write_label(FILE* outfile, const char* label)
{
    fprintf(outfile, "label %s\n", label);
}


/*
*  Writes a VM goto command.
*/
void write_goto(FILE* outfile, const char* label)
{
    fprintf(outfile, "goto %s\n", label);
}


/*
*  Writes a VM If-goto command.
*/
void write_if(FILE* outfile, const char* label)
{
    fprintf(outfile, "if-goto %s\n", label);
}


/*
*  Writes a VM call command.
*/
void write_call(FILE* outfile, const char* name, int numargs)
{
    fprintf(outfile, "call %s %d\n", name, numargs);
}


/*
*  Writes a VM function command.
*/
void write_function(FILE* outfile, const char* name, int numlocals)
{
    fprintf(outfile, "function %s %d\n", name, numlocals);
}


/*
* Writes a VM return command.
*/
void write_return(FILE* outfile)
{
    fprintf(outfile, "return\n");
}


/*
* Returns an empty string if unable to match.
*/
char* convert_vmcommand_to_string(vmcommand command)
{
    switch (command)
    {
        case VMC_NONE:
            return "NONE";
        case VMC_ADD:
            return "add";
        case VMC_SUB:
            return "sub";
        case VMC_NEG:
            return "neg";
        case VMC_EQ:
            return "eq";
        case VMC_GT:
            return "gt";
        case VMC_LT:
            return "lt";
        case VMC_AND:
            return "and";
        case VMC_OR:
            return "or";
        case VMC_NOT:
            return "not";

        // special cases for non-basic VM operations, handled by OS functions
        case VMC_MULT:
            return "call Math.multiply 2";
        case VMC_DIV:
            return "call Math.divide 2";

        default:
            return "";
    }
}


/*
* Returns an empty string if unable to match.
*/
char* convert_vmsegment_to_string(vmsegment segment)
{
    switch (segment)
    {
        case VMS_NONE:
            return "NONE";
        case VMS_CONST:
            return "constant";
        case VMS_ARG:
            return "argument";
        case VMS_LOCAL:
            return "local";
        case VMS_STATIC:
            return "static";
        case VMS_THIS:
            return "this";
        case VMS_THAT:
            return "that";
        case VMS_POINTER:
            return "pointer";
        case VMS_TEMP:
            return "temp";
        default:
            return "";
    }
}


vmcommand convert_unary_operator_to_vmcommand(char op)
{
    switch (op)
    {
        case '-':
            return VMC_NEG;
        case '~':
            return VMC_NOT;
        default:
            return VMC_NONE;
    }
}


vmcommand convert_binary_operator_to_vmcommand(char op)
{
    switch (op)
    {
        case '+':
            return VMC_ADD;
        case '-':
            return VMC_SUB;
        case '*':
            return VMC_MULT;
        case '/':
            return VMC_DIV;
        case '&':
            return VMC_AND;
        case '|':
            return VMC_OR;
        case '<':
            return VMC_LT;
        case '>':
            return VMC_GT;
        case '=':
            return VMC_EQ;
        default:
            return VMC_NONE;
    }
}


vmsegment convert_symbolkind_to_vmsegment(symbolkind kind)
{
    switch (kind)
    {
        case SK_STATIC:
            return VMS_STATIC;
        case SK_FIELD:
            return VMS_THIS;
        case SK_ARG:
            return VMS_ARG;
        case SK_VAR:
            return VMS_LOCAL;
        default:
            return VMS_NONE;
    }
}
