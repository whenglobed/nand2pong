#include "tokenizerengine.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/*
* tokenize_class() is the only function called by tokenizer(). All other functions
* are sub-functions called by tokenize_class(), or called by other sub-functions.
* Thus, the call stack reflects the parse tree for the .jack file as well as the
* structure of the XML output. Comments with characters (e.g. // '{') show the
* token that is expected to be found while parsing the instruction at that point.
* Currently, this program assumes the input is valid and does not check for or
* report unexpected tokens other than in assert() statements at the beginning of
* some functions.
*
* TODO: A lot of the print & get calls could be condensed into loops. Also,
* print & get are called together so frequently it would be helpful to make a
* print_and_get_next() function that calls both. Maybe call it tokenize_token()?
*/
void tokenize_class(token* t, FILE* infile, FILE* outfile, int* linenum, symboltable* classtable, symboltable* subtable)
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
        fprintf(outfile, "%*s<class>\n", *indent, "");
        (*indent) += INDENT_WIDTH;

        print_terminal_with_tags(t, classtable, outfile, indent);
        get_next_token(t, infile, linenum);
        print_terminal_with_tags(t, classtable, outfile, indent);
        get_next_token(t, infile, linenum);
        print_terminal_with_tags(t, classtable, outfile, indent); // '{'
        get_next_token(t, infile, linenum);

        while(t->key == K_STATIC || t->key == K_FIELD)
        {
            tokenize_class_var_dec(t, infile, outfile, linenum, indent, classtable);
        }
        while(t->key == K_CONSTRUCTOR || t->key == K_FUNCTION || t->key == K_METHOD || t->key == K_VOID)
        {
            tokenize_subroutine(t, infile, outfile, linenum, indent, subtable);
        }
        print_terminal_with_tags(t, classtable, outfile, indent); // '}'

        (*indent) -= INDENT_WIDTH;
        fprintf(outfile, "%*s</class>\n", *indent, "");
    }
}


void tokenize_class_var_dec(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    fprintf(outfile, "%*s<classVarDec>\n", *indent, "");
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

    print_terminal_with_tags(t, st, outfile, indent); // field or static
    get_next_token(t, infile, linenum);

    strcpy(t->symboldata->type, t->name);

    print_terminal_with_tags(t, st, outfile, indent); // type
    get_next_token(t, infile, linenum);

    while( !(t->type == T_SYMBOL && get_symbol(t) == ';') )
    {
        print_terminal_with_tags(t, st, outfile, indent);
        copy_symboldata_into_symbol_table(t, st);
        get_next_token(t, infile, linenum);
    }

    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);

    t->symboldata->is_being_defined = false;
    t->symboldata->kind = SK_NONE;
    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</classVarDec>\n", *indent, "");
}


void tokenize_subroutine(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    fprintf(outfile, "%*s<subroutineDec>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    start_subroutine(st); // clear symboltable for each subroutine

    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);
    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);
    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);
    print_terminal_with_tags(t, st, outfile, indent); // '('
    get_next_token(t, infile, linenum);
    tokenize_parameter_list(t, infile, outfile, linenum, indent, st);
    print_terminal_with_tags(t, st, outfile, indent); // ')'
    get_next_token(t, infile, linenum);
    fprintf(outfile, "%*s<subroutineBody>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    print_terminal_with_tags(t, st, outfile, indent); // '{'
    get_next_token(t, infile, linenum);
    while (t->key == K_VAR)
    {
        tokenize_var_dec(t, infile, outfile, linenum, indent, st);
    }
    if (is_statement(t))
    {
        tokenize_statements(t, infile, outfile, linenum, indent, st);
    }
    print_terminal_with_tags(t, st, outfile, indent); // '}'
    get_next_token(t, infile, linenum);

    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</subroutineBody>\n", *indent, "");
    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</subroutineDec>\n", *indent, "");
}


void tokenize_parameter_list(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    fprintf(outfile, "%*s<parameterList>\n", *indent, "");
    (*indent) += INDENT_WIDTH;
    t->symboldata->kind = SK_ARG;

    while ( !(t->type == T_SYMBOL && get_symbol(t) == ')'))
    {
        strcpy(t->symboldata->type, t->name);

        print_terminal_with_tags(t, st, outfile, indent); // type
        get_next_token(t, infile, linenum);
        print_terminal_with_tags(t, st, outfile, indent); // name
        copy_symboldata_into_symbol_table(t, st);

        get_next_token(t, infile, linenum);

        if (t->type == T_SYMBOL && get_symbol(t) == ',')
        {
            print_terminal_with_tags(t, st, outfile, indent);
            get_next_token(t, infile, linenum);
        }
    }

    t->symboldata->kind = SK_NONE;
    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</parameterList>\n", *indent, "");
}


void tokenize_var_dec(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    assert(t->key == K_VAR); // DEBUG

    fprintf(outfile, "%*s<varDec>\n", *indent, "");
    (*indent) += INDENT_WIDTH;
    t->symboldata->kind = SK_VAR;
    t->symboldata->is_being_defined = true;

    print_terminal_with_tags(t, st, outfile, indent); // var
    get_next_token(t, infile, linenum);

    strcpy(t->symboldata->type, t->name);

    print_terminal_with_tags(t, st, outfile, indent); // type
    get_next_token(t, infile, linenum);

    while ( !(t->type == T_SYMBOL && get_symbol(t) == ';') )
    {
        print_terminal_with_tags(t, st, outfile, indent);
        copy_symboldata_into_symbol_table(t, st);
        get_next_token(t, infile, linenum);
    }
    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);

    t->symboldata->is_being_defined = false;
    t->symboldata->kind = SK_NONE;
    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</varDec>\n", *indent, "");
}


void tokenize_statements(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    // assert(is_statement(t)); // DEBUG, currently this will cause an abort for empty while blocks

    fprintf(outfile, "%*s<statements>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    while (is_statement(t))
    {
        // K_ELSE is handled in tokenize_if() and not included here
        if (t->key == K_LET)
        {
            tokenize_let(t, infile, outfile, linenum, indent, st);
        }
        else if (t->key == K_IF)
        {
            tokenize_if(t, infile, outfile, linenum, indent, st);
        }
        else if (t->key == K_WHILE)
        {
            tokenize_while(t, infile, outfile, linenum, indent, st);
        }
        else if (t->key == K_DO)
        {
            tokenize_do(t, infile, outfile, linenum, indent, st);
        }
        else if (t->key == K_RETURN)
        {
            tokenize_return(t, infile, outfile, linenum, indent, st);
        }
    }
    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</statements>\n", *indent, "");
}


void tokenize_do(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    assert(t->key == K_DO); // DEBUG

    fprintf(outfile, "%*s<doStatement>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);
    tokenize_subroutine_call(t, infile, outfile, linenum, indent, st);
    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);

    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</doStatement>\n", *indent, "");
}


void tokenize_let(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    assert(t->key == K_LET); // DEBUG

    fprintf(outfile, "%*s<letStatement>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);
    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);
    if (t->type == T_SYMBOL && get_symbol(t) == '[')
    {
        print_terminal_with_tags(t, st, outfile, indent);
        get_next_token(t, infile, linenum);
        tokenize_expression(t, infile, outfile, linenum, indent, st);
        print_terminal_with_tags(t, st, outfile, indent); // ']'
        get_next_token(t, infile, linenum);
    }
    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);
    tokenize_expression(t, infile, outfile, linenum, indent, st);
    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);

    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</letStatement>\n", *indent, "");
}


void tokenize_while(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    assert(t->key == K_WHILE); // DEBUG

    fprintf(outfile, "%*s<whileStatement>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);
    if (t->type == T_SYMBOL && get_symbol(t) == '(')
    {
        print_terminal_with_tags(t, st, outfile, indent);
        get_next_token(t, infile, linenum);
        tokenize_expression(t, infile, outfile, linenum, indent, st);
        print_terminal_with_tags(t, st, outfile, indent); // ')'
        get_next_token(t, infile, linenum);
    }
    print_terminal_with_tags(t, st, outfile, indent); // '{'
    get_next_token(t, infile, linenum);
    tokenize_statements(t, infile, outfile, linenum, indent, st);
    print_terminal_with_tags(t, st, outfile, indent); // '}'
    get_next_token(t, infile, linenum);

    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</whileStatement>\n", *indent, "");
}


void tokenize_return(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    assert(t->key == K_RETURN); // DEBUG

    fprintf(outfile, "%*s<returnStatement>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);
    if ( !(t->type == T_SYMBOL && get_symbol(t) == ';') )
    {
        tokenize_expression(t, infile, outfile, linenum, indent, st);
    }
    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);

    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</returnStatement>\n", *indent, "");
}


void tokenize_if(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    assert(t->key == K_IF); // DEBUG

    fprintf(outfile, "%*s<ifStatement>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    print_terminal_with_tags(t, st, outfile, indent);
    get_next_token(t, infile, linenum);
    print_terminal_with_tags(t, st, outfile, indent); // '('
    get_next_token(t, infile, linenum);
    tokenize_expression(t, infile, outfile, linenum, indent, st);
    print_terminal_with_tags(t, st, outfile, indent); // ')'
    get_next_token(t, infile, linenum);
    print_terminal_with_tags(t, st, outfile, indent); // '{'
    get_next_token(t, infile, linenum);
    tokenize_statements(t, infile, outfile, linenum, indent, st);
    print_terminal_with_tags(t, st, outfile, indent); // '}'
    get_next_token(t, infile, linenum);
    if (t->key == K_ELSE)
    {
        print_terminal_with_tags(t, st, outfile, indent);
        get_next_token(t, infile, linenum);
        print_terminal_with_tags(t, st, outfile, indent); // '{'
        get_next_token(t, infile, linenum);
        tokenize_statements(t, infile, outfile, linenum, indent, st);
        print_terminal_with_tags(t, st, outfile, indent); // '}'
        get_next_token(t, infile, linenum);
    }

    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</ifStatement>\n", *indent, "");
}


void tokenize_expression(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    fprintf(outfile, "%*s<expression>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    tokenize_term(t, infile, outfile, linenum, indent, st);
    while (is_binary_operator(t))
    {
        print_terminal_with_tags(t, st, outfile, indent);
        get_next_token(t, infile, linenum);
        tokenize_term(t, infile, outfile, linenum, indent, st);
    }

    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</expression>\n", *indent, "");
}


void tokenize_term(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    fprintf(outfile, "%*s<term>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    char nextchar = peek_at_next_token_start(infile, linenum);

    if (nextchar == '[')
    {
        print_terminal_with_tags(t, st, outfile, indent);
        get_next_token(t, infile, linenum);
        print_terminal_with_tags(t, st, outfile, indent); // '['
        get_next_token(t, infile, linenum);
        tokenize_expression(t, infile, outfile, linenum, indent, st);
        print_terminal_with_tags(t, st, outfile, indent); // ']'
        get_next_token(t, infile, linenum);
    }
    else if (t->type == T_SYMBOL && get_symbol(t) == '(')
    {
        print_terminal_with_tags(t, st, outfile, indent);
        get_next_token(t, infile, linenum);
        tokenize_expression(t, infile, outfile, linenum, indent, st);
        print_terminal_with_tags(t, st, outfile, indent); // ')'
        get_next_token(t, infile, linenum);
    }
    else if (is_unary_operator(t))
    {
        print_terminal_with_tags(t, st, outfile, indent);
        get_next_token(t, infile, linenum);
        tokenize_term(t, infile, outfile, linenum, indent, st);
    }
    else if (nextchar == '.' || nextchar == '(')
    {
        // beware of catching nested expressions, e.g. ((a+2)-1), with this condition
        // also, unary operators with parentheses, e.g. -(a+3)
        // shouldn't happen due to order of ifs, but maybe put in an explicit check
        tokenize_subroutine_call(t, infile, outfile, linenum, indent, st);
    }
    else
    {
        print_terminal_with_tags(t, st, outfile, indent);
        get_next_token(t, infile, linenum);
    }

    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</term>\n", *indent, "");
}


void tokenize_expression_list(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    fprintf(outfile, "%*s<expressionList>\n", *indent, "");
    (*indent) += INDENT_WIDTH;

    if ( !(t->type == T_SYMBOL && get_symbol(t) == ')')) // make sure it's not an empty expressionList, e.g. ()
    {
        tokenize_expression(t, infile, outfile, linenum, indent, st);
        while (t->type == T_SYMBOL && get_symbol(t) == ',')
        {
            print_terminal_with_tags(t, st, outfile, indent);
            get_next_token(t, infile, linenum);
            tokenize_expression(t, infile, outfile, linenum, indent, st);
        }
    }

    (*indent) -= INDENT_WIDTH;
    fprintf(outfile, "%*s</expressionList>\n", *indent, "");
}


void tokenize_subroutine_call(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st)
{
    // this is a helper function for tokenize_do() and tokenize_term(), so no <tags> are needed
    while ( !(t->type == T_SYMBOL && get_symbol(t) == '(') )
    {
        print_terminal_with_tags(t, st, outfile, indent);
        get_next_token(t, infile, linenum);
    }
    print_terminal_with_tags(t, st, outfile, indent); // '('
    get_next_token(t, infile, linenum);
    tokenize_expression_list(t, infile, outfile, linenum, indent, st);
    print_terminal_with_tags(t, st, outfile, indent); // ')'
    get_next_token(t, infile, linenum);
}
