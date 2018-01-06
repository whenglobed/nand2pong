#include "symboltable.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>


/*
* Frees all nodes in the provided symboltable and creates a new one.
*/
void start_subroutine(symboltable* subtable)
{
    free_symbol_table_nodes(subtable);
    initialize_symbol_table(subtable);
}


/*
* Copies name, type, and kind into node fields. A "setter" function.
*/
void define(tablenode* node, const char* name, const char* type, const symbolkind kind)
{
    assert (node != NULL); // DEBUG

    strcpy (node->name, name);
    strcpy (node->type, type);
    node->kind = kind;
}


/*
* Returns the number of variables of the given kind already defined in the symboltable.
*/
unsigned int var_count(const symboltable* st, const symbolkind kind)
{
    switch (kind)
    {
        case SK_STATIC:
            return st->staticindex;
        case SK_FIELD:
            return st->fieldindex;
        case SK_ARG:
            return st->argindex;
        case SK_VAR:
            return st->varindex;
        case SK_NONE:
            return 0;
        default:
            return 0;
    }
}


/*
* Returns the kind of the named identifier in the provided symbol table.
* If the identifier is unknown in the table, returns SK_NONE.
*/
symbolkind kind_of(const symboltable* st, const char* name)
{
    tablenode* temp = search_symbol_table(st, name);
    if (temp == NULL)
    {
        return SK_NONE;
    }
    else
    {
        return temp->kind;
    }
}


/*
* Returns the type of the named identifier in the provided symbol table.
* If the identifier is unknown in the table, returns NONE.
*/
char* type_of(const symboltable* st, const char* name)
{
    tablenode* temp = search_symbol_table(st, name);
    if (temp == NULL)
    {
        return "NONE";
    }
    else
    {
        return temp->type;
    }
}


/*
* Returns the type of the named identifier in the provided symbol table.
* If the identifier is unknown in the table, returns -1.
*/
int index_of(const symboltable* st, const char* name)
{
    tablenode* temp = search_symbol_table(st, name);
    if (temp == NULL)
    {
        return -1;
    }
    else
    {
        return (int)temp->index;
    }
}


/*
* Sets all tablenode pointers in the array to NULL and sets indices to 0.
*/
void initialize_symbol_table(symboltable* st)
{
    assert (st != NULL); // DEBUG

    for (size_t i = 0; i < TABLE_SIZE; i++)
    {
        st->headnodes[i] = NULL;
    }
    st->staticindex = 0;
    st->fieldindex = 0;
    st->argindex = 0;
    st->varindex = 0;
}

/*
* Initializes the fields of the pointed-to node with default values.
* Memory for this node should be allocated elsewhere first.
*/
void initialize_blank_node(tablenode* node)
{
    strcpy(node->name, "none");
    strcpy(node->type, "none");
    node->kind = SK_NONE;
    node->index = 0;
    node->is_being_defined = false;
    node->next = NULL;
}

/*
* Prints all entries in the symbol table, including the index
* of the current node. For nodes that begin linked lists,
* the list is traversed and printed with -> to show links.
*/
void print_symbol_table(const symboltable* st)
{
    for (size_t i = 0; i < TABLE_SIZE; i++)
    {
        if (st->headnodes[i] == NULL)
        {
            // printf("[%d] -> NULL\n", i);
        }
        else
        {
            printf("[%d] NAME: %s, TYPE: %s, KIND: %s, INDEX: %d\n", i, st->headnodes[i]->name, st->headnodes[i]->type,
                convert_symbolkind_to_string(st->headnodes[i]->kind), st->headnodes[i]->index);

            tablenode* temp = st->headnodes[i]->next;
            while (temp != NULL)
            {
                printf("    -> NAME: %s, TYPE: %s, KIND: %s, INDEX: %d\n", temp->name, temp->type,
                convert_symbolkind_to_string(temp->kind), temp->index);
                temp = temp->next;
            }
        }
    }
}

/*
* Converts a symbolkind enum to a string literal and returns it.
* Used for printing purposes.
*/
const char* convert_symbolkind_to_string(symbolkind sk)
{
    switch (sk)
    {
        case SK_NONE:
            return "none";
        case SK_STATIC:
            return "static";
        case SK_FIELD:
            return "field";
        case SK_ARG:
            return "argument";
        case SK_VAR:
            return "var";
        default:
            return "ERROR";
    }
}

/*
* Frees all allocated memory for nodes in the symboltable.
*/
void free_symbol_table_nodes(symboltable* st)
{
    for (size_t i = 0; i < TABLE_SIZE; i++)
    {
        if (st->headnodes[i] != NULL)
        {
            free_tablenode(st->headnodes[i]);
            st->headnodes[i] = NULL;
        }
    }
}

/*
* Frees the memory allocated for a single node.
* If a linked list is encountered, frees all nodes in the list.
*/
void free_tablenode(tablenode* node)
{
    tablenode* temp = node;
    tablenode* temp2 = NULL;
    while (temp != NULL)
    {
        temp2 = temp->next;
        free(temp);
        temp = temp2;
    }
    /*
    Alterate recursive method:
    if (node != NULL)
    {
        free_tablenode(node->next);
        free(node);
    }
    */
}


/*
* Returns a hash value for the provided string.
*/
unsigned int hash_string(const char* s)
{
    unsigned int hashval = 0;
    for(size_t i = 0; i < strlen(s); i++)
    {
        hashval = s[i] + ((hashval << 5) - hashval);
    }
    return hashval % TABLE_SIZE;
}

/*
* Hashes the name string of newnode and appends the node to the
* symboltable at the appropriate location.
*/
void append_node(symboltable* st, tablenode* newnode)
{
    unsigned int hashval = hash_string(newnode->name);

    if (st->headnodes[hashval] == NULL)
    {
        st->headnodes[hashval] = newnode;
    }
    else
    {
        // already a symbol at this index, so traverse linked list until the end and insert
        tablenode* temp = st->headnodes[hashval];

        while (temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = newnode;
    }
}

/*
* Returns a pointer to the first node in the symboltable with a "name" field
* equal to the provided name string. Returns NULL if no match is found.
*/
tablenode* search_symbol_table(const symboltable* st, const char* name)
{
    unsigned int hashval = hash_string(name);
    tablenode* temp = st->headnodes[hashval];

    while (temp != NULL && strcmp(name, temp->name) != 0)
    {
        temp = temp->next;
    }

    return temp;
}
