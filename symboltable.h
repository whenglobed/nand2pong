#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <stdlib.h>
#include <stdbool.h>

#define TABLE_SIZE 31 // prime, for hashing purposes
#define MAX_NAME_LENGTH 64 // arbitrary assumption about max variable name length
#define MAX_TYPE_LENGTH 64 // arbitrary assumption about max type name length

typedef enum symbolkind
{
    SK_NONE,
    SK_STATIC,
    SK_FIELD,
    SK_ARG,
    SK_VAR,
} symbolkind;

typedef struct tablenode
{
    char name[MAX_NAME_LENGTH];
    char type[MAX_TYPE_LENGTH];
    symbolkind kind;
    unsigned int index;
    bool is_being_defined;
    struct tablenode* next;
} tablenode;

typedef struct symboltable
{
    tablenode* headnodes[TABLE_SIZE];
    unsigned int staticindex;
    unsigned int fieldindex;
    unsigned int argindex;
    unsigned int varindex;
} symboltable;


// book API functions
void start_subroutine(symboltable* subtable);
void define(tablenode* node, const char* name, const char* type, const symbolkind kind);
unsigned int var_count(const symboltable* st, const symbolkind kind);
symbolkind kind_of(const symboltable* st, const char* name);
char* type_of(const symboltable* st, const char* name);
int index_of(const symboltable* st, const char* name);

// my functions
void initialize_symbol_table(symboltable* const st);
void initialize_blank_node(tablenode* node);
const char* convert_symbolkind_to_string(symbolkind sk);
void print_symbol_table(const symboltable* st);
void free_symbol_table_nodes(symboltable* st);
void free_tablenode(tablenode* node);
unsigned int hash_string(const char* s);
void append_node(symboltable* st, tablenode* newnode);
tablenode* search_symbol_table(const symboltable* st, const char* name);

#endif // SYMBOLTABLE_H
