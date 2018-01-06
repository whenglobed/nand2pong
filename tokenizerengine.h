#ifndef TOKENIZERENGINE_H
#define TOKENIZERENGINE_H

#include <stdio.h>
#include "jacktokenizer.h"

//enum {INDENT_WIDTH = 2};
#define INDENT_WIDTH 2

// book API functions
void tokenize_class(token* t, FILE* infile, FILE* outfile, int* linenum, symboltable* classtable, symboltable* subtable);
void tokenize_class_var_dec(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);
void tokenize_subroutine(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);
void tokenize_parameter_list(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);
void tokenize_var_dec(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);
void tokenize_statements(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);
void tokenize_do(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);
void tokenize_let(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);
void tokenize_while(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);
void tokenize_return(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);
void tokenize_if(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);
void tokenize_expression(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);
void tokenize_term(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);
void tokenize_expression_list(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);

// my functions
void tokenize_subroutine_call(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent, symboltable* st);

#endif // TOKENIZERENGINE_H
