#include "compilationengine.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "vmwriter.h"


/*
* This file, compilationengine.c, was originally written to parse, tokenize, and
* print Jack code as XML output. This is a modified version that translates the
* code into VM commands. The original version has been preserved as
* tokenizerengine.c.
*
* Currently, this program assumes the input is valid and does not check for or
* report unexpected tokens other than in assert() statements at the beginning of
* some functions.
*
* TODO: Break up functions. compile_term() and the subroutine functions are particularly messy.
*/
void compile_class(token* t, FILE* infile, FILE* outfile, int* linenum, symboltable* classtable, symboltable* subtable)
{
    assert (t->key == K_CLASS); // DEBUG

    if (t->key != K_CLASS)
    {
        fprintf(stderr, "line %d: Unexpected token '%s', expected 'class'\n", *linenum, t->name); // TODO: add filename to this message
    }
    else
    {
        int indentstart = 0;
        int* indent = &indentstart;
        // fprintf(outfile, "%*s<class>\n", *indent, "");
        (*indent) += INDENT_WIDTH;

        get_next_token(t, infile, linenum); // class name

        char* classname = malloc((strlen(t->name) + 1 ) * sizeof(*classname)); // +1 for NUL
        if (classname == NULL)
        {
            fprintf(stderr, "Error: could not allocate memory for classname\n");
            exit(1);
        }
        strcpy(classname, t->name);

        get_next_token(t, infile, linenum); // '{'
        get_next_token(t, infile, linenum);

        while(t->key == K_STATIC || t->key == K_FIELD)
        {
            compile_class_var_dec(t, infile, linenum, indent, classtable);
        }
        while(t->key == K_CONSTRUCTOR || t->key == K_FUNCTION || t->key == K_METHOD || t->key == K_VOID)
        {
            compile_subroutine(t, infile, outfile, linenum, indent, classtable, subtable, classname);
        }

        (*indent) -= INDENT_WIDTH;
        // fprintf(outfile, "%*s</class>\n", *indent, "");

        printf("\nClasstable:\n"); // DEBUG
        print_symbol_table(classtable); // DEBUG

        // cleanup
        free(classname);
        classname = NULL;
    }
}


/*
*
*/
void compile_class_var_dec(token* t, FILE* infile, int* linenum, int* indent, symboltable* classtable)
{
    // fprintf(outfile, "%*s<classVarDec>\n", *indent, "");
    (*indent) += INDENT_WIDTH;
    if (t->key == K_STATIC)
    {
        t->symboldata->kind = SK_STATIC;
    }
    else if (t->key == K_FIELD)
    {
        t->symboldata->kind = SK_FIELD;
    }
    t->symboldata->is_being_defined = true;

    get_next_token(t, infile, linenum); // type
    strcpy(t->symboldata->type, t->name);
    get_next_token(t, infile, linenum);

    while( !(t->type == T_SYMBOL && get_symbol(t) == ';') )
    {
        copy_symboldata_into_symbol_table(t, classtable);
        get_next_token(t, infile, linenum);
    }

    get_next_token(t, infile, linenum);

    t->symboldata->is_being_defined = false;
    t->symboldata->kind = SK_NONE;
    (*indent) -= INDENT_WIDTH;
    // fprintf(outfile, "%*s</classVarDec>\n", *indent, "");
}


/*
*
*/
void compile_subroutine(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
                        symboltable* classtable, symboltable* subtable, const char* classname)
{
    char* functionname = NULL;
    vmlabelcounts labelcounts;
    labelcounts.whilecount = 0;
    labelcounts.ifcount = 0;
    vmlabelcounts* labelcountspointer = &labelcounts;

    // fprintf(outfile, "%*s<subroutineDec>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    start_subroutine(subtable); // clear symboltable for each subroutine

    // current symbol is either "constructor," "function," or "method" ... use this later to determine whether to push "this" argument?
    bool is_constructor = false;
    bool is_method = false;
    if (t->key == K_CONSTRUCTOR)
    {
        is_constructor = true;
    }
    else if (t->key == K_METHOD)
    {
        is_method = true;
        subtable->argindex = 1; // methods have "this" pushed as first argument, so index starts at 1
    }

    get_next_token(t, infile, linenum); // return type
    get_next_token(t, infile, linenum); // function name

    functionname = malloc((strlen(classname) + strlen(t->name) + 2) * sizeof(*functionname)); // +1 for '.' and +1 for NUL
    if (functionname == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for functionname\n");
        exit(1);
    }
    strcpy(functionname, classname);
    strcat(functionname, ".");
    strcat(functionname, t->name);

    get_next_token(t, infile, linenum); // '('
    get_next_token(t, infile, linenum);
    compile_parameter_list(t, infile, linenum, indent, subtable);

    get_next_token(t, infile, linenum); // '{'

    // fprintf(outfile, "%*s<subroutineBody>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    get_next_token(t, infile, linenum);
    while (t->key == K_VAR)
    {
        compile_var_dec(t, infile, linenum, indent, subtable);
    }

    write_function(outfile, functionname, var_count(subtable, SK_VAR));

    if (is_constructor)
    {
        write_push(outfile, VMS_CONST, var_count(classtable, SK_FIELD));
        write_call(outfile, "Memory.alloc", 1);
        write_pop(outfile, VMS_POINTER, 0);
    }
    else if (is_method)
    {
        write_push(outfile, VMS_ARG, 0);
        write_pop(outfile, VMS_POINTER, 0);
    }

    compile_statements(t, infile, outfile, linenum, indent, classtable, subtable, labelcountspointer, classname);

    get_next_token(t, infile, linenum);

    (*indent) -= INDENT_WIDTH;
    // fprintf(outfile, "%*s</subroutineBody>\n", *indent, "");
    (*indent) -= INDENT_WIDTH;
    // fprintf(outfile, "%*s</subroutineDec>\n", *indent, "");

    printf("\nSubtable:\n"); // DEBUG
    print_symbol_table(subtable); // DEBUG

    // cleanup
    free(functionname);
}


/*
*
*/
void compile_parameter_list(token* t, FILE* infile, int* linenum, int* indent, symboltable* subtable)
{
    // fprintf(outfile, "%*s<parameterList>\n", *indent, "");
    (*indent) += INDENT_WIDTH;
    t->symboldata->kind = SK_ARG;

    while ( !(t->type == T_SYMBOL && get_symbol(t) == ')'))
    {
        strcpy(t->symboldata->type, t->name);

        get_next_token(t, infile, linenum); // name
        copy_symboldata_into_symbol_table(t, subtable);

        get_next_token(t, infile, linenum);

        if (t->type == T_SYMBOL && get_symbol(t) == ',')
        {
            get_next_token(t, infile, linenum);
        }
    }

    t->symboldata->kind = SK_NONE;
    (*indent) -= INDENT_WIDTH;
    // fprintf(outfile, "%*s</parameterList>\n", *indent, "");
}


/*
*
*/
void compile_var_dec(token* t, FILE* infile, int* linenum, int* indent, symboltable* subtable)
{
    assert(t->key == K_VAR); // DEBUG

    // fprintf(outfile, "%*s<varDec>\n", *indent, "");
    (*indent) += INDENT_WIDTH;
    t->symboldata->kind = SK_VAR;
    t->symboldata->is_being_defined = true;

    get_next_token(t, infile, linenum); // type
    strcpy(t->symboldata->type, t->name);
    get_next_token(t, infile, linenum);

    while ( !(t->type == T_SYMBOL && get_symbol(t) == ';') )
    {
        copy_symboldata_into_symbol_table(t, subtable);
        get_next_token(t, infile, linenum);
    }
    get_next_token(t, infile, linenum);

    t->symboldata->is_being_defined = false;
    t->symboldata->kind = SK_NONE;
    (*indent) -= INDENT_WIDTH;
    // fprintf(outfile, "%*s</varDec>\n", *indent, "");
}


/*
*
*/
void compile_statements(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
                        symboltable* classtable, symboltable* subtable, vmlabelcounts* labelcounts, const char* classname)
{
    //assert(is_statement(t)); // DEBUG, currently this will cause an abort for empty while blocks

    // fprintf(outfile, "%*s<statements>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    while (is_statement(t))
    {
        // K_ELSE is handled in compile_if() and not included here
        if (t->key == K_LET)
        {
            compile_let(t, infile, outfile, linenum, indent, classtable, subtable, classname);
        }
        else if (t->key == K_IF)
        {
            compile_if(t, infile, outfile, linenum, indent, classtable, subtable, labelcounts, classname);
        }
        else if (t->key == K_WHILE)
        {
            compile_while(t, infile, outfile, linenum, indent, classtable, subtable, labelcounts, classname);
        }
        else if (t->key == K_DO)
        {
            compile_do(t, infile, outfile, linenum, indent, classtable, subtable, classname);
        }
        else if (t->key == K_RETURN)
        {
            compile_return(t, infile, outfile, linenum, indent, classtable, subtable, classname);
        }
    }
    (*indent) -= INDENT_WIDTH;
    // fprintf(outfile, "%*s</statements>\n", *indent, "");
}


/*
*
*/
void compile_do(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
                symboltable* classtable, symboltable* subtable, const char* classname)
{
    assert(t->key == K_DO); // DEBUG

    // fprintf(outfile, "%*s<doStatement>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    get_next_token(t, infile, linenum);
    compile_subroutine_call(t, infile, outfile, linenum, indent, classtable, subtable, classname);

    // do statements call subroutines, but do not assign the return value to any variable, so the
    // return value is popped to a temp variable to effectively "discard" it
    write_pop(outfile, VMS_TEMP, 0);

    get_next_token(t, infile, linenum);

    (*indent) -= INDENT_WIDTH;
    // fprintf(outfile, "%*s</doStatement>\n", *indent, "");
}


/*
*
*/
void compile_let(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
                 symboltable* classtable, symboltable* subtable, const char* classname)
{
    assert(t->key == K_LET); // DEBUG

    //fprintf(outfile, "%*s<letStatement>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    get_next_token(t, infile, linenum); // variable name
    symbolkind tempkind = kind_of(subtable, t->name);
    unsigned int tempindex = index_of(subtable, t->name);
    if (tempkind == SK_NONE)
    {
        // if symbol isn't found in subroutine scope, look it up in the class symboltable
        tempkind = kind_of(classtable, t->name);
        tempindex = index_of(classtable, t->name);
    }
    if (tempkind == SK_NONE)
    {
        fprintf(stderr, "Error: could not find %s in symbol table\n", t->name);
    }

    get_next_token(t, infile, linenum);
    if (t->type == T_SYMBOL && get_symbol(t) == '[')
    {
        // This is an array entry, so push the variable (which points to the base
        // of the array), then calculate the index and add. Next, we set the "that"
        // segment to point to this memory location, then push "that 0", which puts
        // the value found at that memory location on the stack.

        // This is an array entry, so push the variable (which points to the base of the
        // array), then calculate the index and add it to the base. Next, we set the "that"
        // segment to point to this memory location (via pointer 1). After calculating the
        // value we want to store, we can pop it to "that 0".

        // BUT, because the "that" segment can change when calculating the upcoming expression,
        // the location is stored in a temp variable until we're ready to pop the value we want to assign.

        write_push(outfile, convert_symbolkind_to_vmsegment(tempkind), tempindex);
        get_next_token(t, infile, linenum);
        compile_expression(t, infile, outfile, linenum, indent, classtable, subtable, classname); // the index
        write_arithmetic(outfile, VMC_ADD);
        write_pop(outfile, VMS_TEMP, 1);

        get_next_token(t, infile, linenum); // ']'
        get_next_token(t, infile, linenum);
        compile_expression(t, infile, outfile, linenum, indent, classtable, subtable, classname); // the value we want to store

        write_push(outfile, VMS_TEMP, 1); // put the pointer value for the array element back on the stack
        write_pop(outfile, VMS_POINTER, 1);
        write_pop(outfile, VMS_THAT, 0);
    }
    else
    {
        get_next_token(t, infile, linenum);
        compile_expression(t, infile, outfile, linenum, indent, classtable, subtable, classname);

        write_pop(outfile, convert_symbolkind_to_vmsegment(tempkind), tempindex);
    }

    get_next_token(t, infile, linenum);

    (*indent) -= INDENT_WIDTH;
    //fprintf(outfile, "%*s</letStatement>\n", *indent, "");
}


/*
*
*/
void compile_while(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
                   symboltable* classtable, symboltable* subtable, vmlabelcounts* labelcounts, const char* classname)
{
    assert(t->key == K_WHILE); // DEBUG

    char startlabel[16];
    char endlabel[16];
    strcpy(startlabel, "WHILE_EXP");
    strcpy(endlabel, "WHILE_END");
    char whilenumber[4]; // room for 3 digits + NUL
    sprintf(whilenumber, "%u", labelcounts->whilecount);
    strcat(startlabel, whilenumber);
    strcat(endlabel, whilenumber);
    labelcounts->whilecount++;

    write_label(outfile, startlabel);

    // fprintf(outfile, "%*s<whileStatement>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    get_next_token(t, infile, linenum); // '('
    get_next_token(t, infile, linenum);
    compile_expression(t, infile, outfile, linenum, indent, classtable, subtable, classname);
    get_next_token(t, infile, linenum); // ')'

    // negate before comparison
    // we only want to jump to the end of the loop if the condition is false
    write_arithmetic(outfile, VMC_NOT);
    write_if(outfile, endlabel);

    get_next_token(t, infile, linenum); // '{'
    compile_statements(t, infile, outfile, linenum, indent, classtable, subtable, labelcounts, classname);
    write_goto(outfile, startlabel);

    get_next_token(t, infile, linenum);

    write_label(outfile, endlabel);

    (*indent) -= INDENT_WIDTH;
    // fprintf(outfile, "%*s</whileStatement>\n", *indent, "");
}


/*
*
*/
void compile_return(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
                    symboltable* classtable, symboltable* subtable, const char* classname)
{
    assert(t->key == K_RETURN); // DEBUG

    // fprintf(outfile, "%*s<returnStatement>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    get_next_token(t, infile, linenum);
    if (t->type == T_SYMBOL && get_symbol(t) == ';')
    {
        // nothing specific is being returned, so push 0 first
        write_push(outfile, VMS_CONST, 0);
        write_return(outfile);
    }
    else
    {
        compile_expression(t, infile, outfile, linenum, indent, classtable, subtable, classname);
        write_return(outfile);
    }
    get_next_token(t, infile, linenum);

    (*indent) -= INDENT_WIDTH;
    // fprintf(outfile, "%*s</returnStatement>\n", *indent, "");
}


/*
*
*/
void compile_if(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
                symboltable* classtable, symboltable* subtable, vmlabelcounts* labelcounts, const char* classname)
{
    assert(t->key == K_IF); // DEBUG

    char iftruelabel[16];
    char iffalselabel[16];
    char ifendlabel[16];
    strcpy(iftruelabel, "IF_TRUE");
    strcpy(iffalselabel, "IF_FALSE");
    strcpy(ifendlabel, "IF_END");
    char ifnumber[4]; // room for 3 digits + NUL
    sprintf(ifnumber, "%u", labelcounts->ifcount);
    strcat(iftruelabel, ifnumber);
    strcat(iffalselabel, ifnumber);
    strcat(ifendlabel, ifnumber);
    labelcounts->ifcount++;

    // fprintf(outfile, "%*s<ifStatement>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    get_next_token(t, infile, linenum); // '('
    get_next_token(t, infile, linenum);
    compile_expression(t, infile, outfile, linenum, indent, classtable, subtable, classname);

    write_if(outfile, iftruelabel);
    write_goto(outfile, iffalselabel);
    write_label(outfile, iftruelabel);

    get_next_token(t, infile, linenum); // '{'
    get_next_token(t, infile, linenum);
    compile_statements(t, infile, outfile, linenum, indent, classtable, subtable, labelcounts, classname);

    get_next_token(t, infile, linenum);

    if (t->key == K_ELSE)
    {
        write_goto(outfile, ifendlabel);
        write_label(outfile, iffalselabel);

        get_next_token(t, infile, linenum); // '{'
        get_next_token(t, infile, linenum);
        compile_statements(t, infile, outfile, linenum, indent, classtable, subtable, labelcounts, classname);

        write_label(outfile, ifendlabel);

        get_next_token(t, infile, linenum);
    }
    else
    {
        write_label(outfile, iffalselabel);
    }

    (*indent) -= INDENT_WIDTH;
    // fprintf(outfile, "%*s</ifStatement>\n", *indent, "");
}

/*
*
*/
void compile_expression(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
                        symboltable* classtable, symboltable* subtable, const char* classname)
{
    // fprintf(outfile, "%*s<expression>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    compile_term(t, infile, outfile, linenum, indent, classtable, subtable, classname);

    while (is_binary_operator(t))
    {
        char temp = get_symbol(t);

        get_next_token(t, infile, linenum);
        compile_term(t, infile, outfile, linenum, indent, classtable, subtable, classname);

        write_arithmetic(outfile, convert_binary_operator_to_vmcommand(temp));
    }

    (*indent) -= INDENT_WIDTH;
    // fprintf(outfile, "%*s</expression>\n", *indent, "");
}


/*
*
*/
void compile_term(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
                  symboltable* classtable, symboltable* subtable, const char* classname)
{
    // fprintf(outfile, "%*s<term>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    char nextchar = peek_at_next_token_start(infile, linenum);

    if (nextchar == '[')
    {
        // This is an array entry, so push the variable (which points to the base
        // of the array), then calculate the index and add. Next, we set the "that"
        // segment to point to this memory location, then push "that 0", which puts
        // the value found at that memory location on the stack.
        symbolkind tempkind = kind_of(subtable, t->name);
        unsigned int index = index_of(subtable, t->name);
        if (tempkind == SK_NONE)
        {
            // not found in subroutine symbol table, try looking in class symbol table
            tempkind = kind_of(classtable, t->name);
            index = index_of(classtable, t->name);
        }
        if (tempkind == SK_NONE)
        {
            fprintf(stderr, "Error: could not find %s in symbol table\n", t->name);
        }
        write_push(outfile, convert_symbolkind_to_vmsegment(tempkind), index);

        get_next_token(t, infile, linenum); // '['
        get_next_token(t, infile, linenum);
        compile_expression(t, infile, outfile, linenum, indent, classtable, subtable, classname); // array index
        write_arithmetic(outfile, VMC_ADD);
        write_pop(outfile, VMS_POINTER, 1);
        write_push(outfile, VMS_THAT, 0);

        get_next_token(t, infile, linenum);
    }
    else if (t->type == T_SYMBOL && get_symbol(t) == '(')
    {
        get_next_token(t, infile, linenum);
        compile_expression(t, infile, outfile, linenum, indent, classtable, subtable, classname);

        get_next_token(t, infile, linenum);
    }
    else if (is_unary_operator(t))
    {
        char tempsymbol = get_symbol(t);
        get_next_token(t, infile, linenum);
        compile_term(t, infile, outfile, linenum, indent, classtable, subtable, classname);

        write_arithmetic(outfile, convert_unary_operator_to_vmcommand(tempsymbol));
    }
    else if (nextchar == '.' || nextchar == '(')
    {
        // beware of catching nested expressions, e.g. ((a+2)-1), with this condition
        // also, unary operators with parentheses, e.g. -(a+3)
        // shouldn't happen due to order of ifs, but maybe put in an explicit check
        compile_subroutine_call(t, infile, outfile, linenum, indent, classtable, subtable, classname);
    }
    else if (t->key == K_THIS)
    {
        write_push(outfile, VMS_POINTER, 0);
        get_next_token(t, infile, linenum);
    }
    else if (t->type == T_STRING_CONST)
    {
        char* stringcopy = malloc((strlen(t->name) + 1) * sizeof(*stringcopy)); // + 1 for NUL
        if (stringcopy == NULL)
        {
            fprintf(stderr, "Error: could not allocate memory for stringcopy\n");
            exit(1);
        }
        get_stringval(stringcopy, t);
        write_push(outfile, VMS_CONST, strlen(stringcopy));
        write_call(outfile, "String.new", 1);
        for (size_t i = 0; i < strlen(stringcopy); i++)
        {
            write_push(outfile, VMS_CONST, stringcopy[i]); // implicitly casting to int when passing
            write_call(outfile, "String.appendChar", 2);
        }
        free(stringcopy);
        stringcopy = NULL;

        get_next_token(t, infile, linenum);
    }
    else
    {
        // integer constants and true/false/null keywords get simple pushes
        if (t->type == T_INT_CONST)
        {
            write_push(outfile, VMS_CONST, get_intval(t));
        }
        else if (t->key == K_TRUE)
        {
            write_push(outfile, VMS_CONST, 1);
            write_arithmetic(outfile, VMC_NEG);
        }
        else if (t->key == K_FALSE || t->key == K_NULL)
        {
            write_push(outfile, VMS_CONST, 0);
        }
        // variables get looked up in the symbol table and then pushed
        else if(t->type == T_IDENTIFIER)
        {
            symbolkind tempkind = kind_of(subtable, t->name);
            unsigned int index = index_of(subtable, t->name);
            if (tempkind == SK_NONE)
            {
                // not found in subroutine symbol table, try looking in class symbol table
                tempkind = kind_of(classtable, t->name);
                index = index_of(classtable, t->name);
            }
            if (tempkind == SK_NONE)
            {
                fprintf(stderr, "Error: could not find %s in symbol table\n", t->name);
            }
            write_push(outfile, convert_symbolkind_to_vmsegment(tempkind), index);
        }

        get_next_token(t, infile, linenum);
    }

    (*indent) -= INDENT_WIDTH;
    // fprintf(outfile, "%*s</term>\n", *indent, "");
}

/*
* Returns number of expressions found in the list.
*/
unsigned int compile_expression_list(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
                                     symboltable* classtable, symboltable* subtable, const char* classname)
{
    // fprintf(outfile, "%*s<expressionList>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    unsigned int numexpressions = 0;

    if ( !(t->type == T_SYMBOL && get_symbol(t) == ')')) // make sure it's not an empty expressionList, e.g. ()
    {
        compile_expression(t, infile, outfile, linenum, indent, classtable, subtable, classname);
        numexpressions++;

        while (t->type == T_SYMBOL && get_symbol(t) == ',')
        {
            get_next_token(t, infile, linenum);
            compile_expression(t, infile, outfile, linenum, indent, classtable, subtable, classname);
            numexpressions++;
        }
    }

    (*indent) -= INDENT_WIDTH;
    // fprintf(outfile, "%*s</expressionList>\n", *indent, "");

    return numexpressions;
}


/*
* If the subroutine call does not include a period, it's a method. If the call does include a
* period, check the current token to see if it's a variable that's been added to the symbol
* table. If found, it's a method call.
* Method calls must first push a reference to the object being operated on.
*/
void compile_subroutine_call(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
                             symboltable* classtable, symboltable* subtable, const char* classname)
{
    unsigned int numargs = 0;
    bool is_method = false;
    bool is_method_calling_method = false;
    char* type = NULL;

    char nextchar = peek_at_next_token_start(infile, linenum);
    if (nextchar != '.')
    {
        // if there is no period, this is a case of a method calling another method
        // push pointer 0, which points to the "this" object (i.e. the "this" memory segment)
        // the current method is already operating on "this," but pushing the pointer
        // makes sure any called methods receive a reference to the same (current) object
        is_method = true;
        is_method_calling_method = true;
        write_push(outfile, VMS_POINTER, 0);
    }
    else if (kind_of(subtable, t->name) != SK_NONE || kind_of(classtable, t->name) != SK_NONE)
    {
        is_method = true;
        symbolkind symbolkind = kind_of(subtable, t->name);
        unsigned int index = index_of(subtable, t->name);
        type = type_of(subtable, t->name);

        if (symbolkind == SK_NONE)
        {
            // not found in subroutine symbol table, try looking in class symbol table
            symbolkind = kind_of(classtable, t->name);
            index = index_of(classtable, t->name);
            type = type_of(classtable, t->name);
        }
        write_push(outfile, convert_symbolkind_to_vmsegment(symbolkind), index);
    }

    char* subroutinename = malloc(1 * sizeof(*subroutinename));
    if (subroutinename == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for subroutinename\n");
        exit(1);
    }
    strcpy(subroutinename, "");

    if (is_method_calling_method)
    {
        // need to manually prefix the method call with the class name to make a valid VM command
        subroutinename = realloc(subroutinename, (strlen(classname) + strlen(t->name) + 2)); // +1 for NUL, +1 for '.'
        if (subroutinename == NULL)
        {
            fprintf(stderr, "Error: could not reallocate memory for subroutinename\n");
            exit(1);
        }
        strcat(subroutinename, classname);
        strcat(subroutinename, ".");
        strcat(subroutinename, t->name);

        get_next_token(t, infile, linenum);
    }
    else if (is_method)
    {
        subroutinename = realloc(subroutinename, (strlen(type) + 1));
        if (subroutinename == NULL)
        {
            fprintf(stderr, "Error: could not reallocate memory for subroutinename\n");
            exit(1);
        }
        strcat(subroutinename, type);
        get_next_token(t, infile, linenum);
    }

    while ( !(t->type == T_SYMBOL && get_symbol(t) == '(') )
    {
        subroutinename = realloc(subroutinename, (strlen(subroutinename) + strlen(t->name) + 1) * sizeof(*subroutinename)); // +1 for NUL
        if (subroutinename == NULL)
        {
            fprintf(stderr, "Error: could not reallocate memory for subroutinename\n");
            exit(1);
        }
        strcat(subroutinename, t->name);
        get_next_token(t, infile, linenum);
    }

    get_next_token(t, infile, linenum);
    numargs = compile_expression_list(t, infile, outfile, linenum, indent, classtable, subtable, classname);

    get_next_token(t, infile, linenum);

    if (is_method)
    {
        write_call(outfile, subroutinename, numargs + 1); // +1 because a reference to the object was pushed first
    }
    else
    {
        write_call(outfile, subroutinename, numargs);
    }

    free(subroutinename);
}
