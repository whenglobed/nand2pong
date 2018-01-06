#ifndef JACKTOKENIZER_H
#define JACKTOKENIZER_H

#include <stdio.h>
#include <stdbool.h>
#include "symboltable.h"

#define MAX_TOKEN_LENGTH 32 // arbitrary assumption about max length of tokens to simplify malloc calls and reduce realloc

typedef enum tokentype
{
    T_KEYWORD,
    T_SYMBOL,
    T_IDENTIFIER,
    T_INT_CONST,
    T_STRING_CONST,
    T_DEFAULT, // used for initialization only
    T_EOF // used for dummy tokens created when end of file is reached
} tokentype;

typedef enum keyword
{
    K_CLASS, K_METHOD, K_FUNCTION, K_CONSTRUCTOR, K_INT, K_BOOLEAN, K_CHAR, K_VOID, K_VAR, K_STATIC,
    K_FIELD, K_LET, K_DO, K_IF, K_ELSE, K_WHILE, K_RETURN, K_TRUE, K_FALSE, K_NULL, K_THIS,
    K_NA // used for all tokens that aren't T_KEYWORD type
} keyword;


typedef struct token
{
    char* name;
    tokentype type;
    keyword key;
    tablenode* symboldata;
} token;


void initialize_token(token* t);
void tokenize(FILE* infile, FILE* outfile); // tokenizes and prints tokens with XML tags
void compile(FILE* infile, FILE* outfile); // tokenizes and compiles into VM commands

char find_next_token_start(FILE* infile, int* const linenum);
char peek_at_next_token_start(FILE* infile, int* const linenum);
void get_next_token(token* t, FILE* infile, int* const linenum); // replaces advance() in book API
void set_token_type(token* t); // book API
void set_token_key(token* t); // book API
char get_symbol(const token* t); // book API
int get_intval(const token* t); // book API
void get_stringval(char* destination, const token* t); // book API
int skip_comment_block(FILE* infile); // returns number of comment lines skipped
void skip_comment_line(FILE* infile);
void print_terminal_with_tags(const token* t, const symboltable* st, FILE* outfile, const int* const indent); // prints token wrapped in appropriate XML tags
void print_symboldata_for_identifiers(const token* t, const symboltable* st, FILE* outfile, const int* const indent);
void copy_symboldata_into_symbol_table(const token* t, symboltable* st);
void free_token_fields(token* t);
bool is_statement(const token* const t);
bool is_binary_operator(const token* const t);
bool is_unary_operator(const token* const t);

#endif // JACKTOKENIZER_H
