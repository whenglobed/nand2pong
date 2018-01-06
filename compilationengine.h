#ifndef COMPILATIONENGINE_H
#define COMPILATIONENGINE_H

#include <stdio.h>
#include "jacktokenizer.h"
#include "vmwriter.h"

//enum {INDENT_WIDTH = 2};
#define INDENT_WIDTH 2

// these counts are incremented when while and if statements are called, and are
// used to make labels unique within subroutines
typedef struct vmlabelcounts
{
    unsigned int whilecount;
    unsigned int ifcount;
} vmlabelcounts;

// book API functions
void compile_class(token* t, FILE* infile, FILE* outfile, int* linenum,
    symboltable* classtable, symboltable* subtable);
void compile_class_var_dec(token* t, FILE* infile, int* linenum, int* indent,
    symboltable* classtable);
void compile_subroutine(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
    symboltable* classtable, symboltable* subtable, const char* classname);
void compile_parameter_list(token* t, FILE* infile, int* linenum, int* indent,
    symboltable* subtable);
void compile_var_dec(token* t, FILE* infile, int* linenum, int* indent,
    symboltable* subtable);
void compile_statements(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
    symboltable* classtable, symboltable* subtable, vmlabelcounts* labelcounts, const char* classname);
void compile_do(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
    symboltable* classtable, symboltable* subtable, const char* classname);
void compile_let(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
    symboltable* classtable, symboltable* subtable, const char* classname);
void compile_while(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
    symboltable* classtable, symboltable* subtable, vmlabelcounts* labelcounts, const char* classname);
void compile_return(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
    symboltable* classtable, symboltable* subtable, const char* classname);
void compile_if(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
    symboltable* classtable, symboltable* subtable, vmlabelcounts* labelcounts, const char* classname);
void compile_expression(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
    symboltable* classtable, symboltable* subtable, const char* classname);
void compile_term(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
    symboltable* classtable, symboltable* subtable, const char* classname);
unsigned int compile_expression_list(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
    symboltable* classtable, symboltable* subtable, const char* classname); // returns number of expressions found in list

// my functions
void compile_subroutine_call(token* t, FILE* infile, FILE* outfile, int* linenum, int* indent,
    symboltable* classtable, symboltable* subtable, const char* classname);

#endif // COMPILATIONENGINE_H
