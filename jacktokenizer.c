#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "jacktokenizer.h"
#include "tokenizerengine.h"
#include "compilationengine.h"

/*
* Reads characters from infile, skipping comment lines/blocks and spaces.
* Finds tokens, identifies them, and prints them to outfile, wrapped in
* XML tags. Most of the work is done here, calling other functions as
* appropriate.
*/
void tokenize(FILE* infile, FILE* outfile)
{
    int linenum = 1;

    token* t = malloc(sizeof(*t));
    symboltable* classtable = malloc(sizeof(*classtable));
    symboltable* subtable = malloc(sizeof(*subtable));

    if (t == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for token\n");
    }
    if (classtable == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for classtable\n");
    }
    if (subtable == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for subtable\n");
    }
    if (t != NULL && classtable != NULL && subtable != NULL)
    {
        initialize_token(t);
        initialize_symbol_table(classtable);
        initialize_symbol_table(subtable);

        get_next_token(t, infile, &linenum);
        tokenize_class(t, infile, outfile, &linenum, classtable, subtable); // should only be one class per .jack file, so we don't need a loop
    }

    // cleanup
    free_token_fields(t);
    free(t);
    free_symbol_table_nodes(classtable);
    free_symbol_table_nodes(subtable);
    free(classtable);
    free(subtable);
}

/*
* Similar to tokenize(), but instead of printing tokens with XML tags, this
* function actually compiles the .jack code into VM commands.
*/
void compile(FILE* infile, FILE* outfile)
{
    int linenum = 1;

    token* t = malloc(sizeof(*t));
    symboltable* classtable = malloc(sizeof(*classtable));
    symboltable* subtable = malloc(sizeof(*subtable));

    if (t == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for token\n");
    }
    if (classtable == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for classtable\n");
    }
    if (subtable == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for subtable\n");
    }
    if (t != NULL && classtable != NULL && subtable != NULL)
    {
        initialize_token(t);
        initialize_symbol_table(classtable);
        initialize_symbol_table(subtable);

        get_next_token(t, infile, &linenum);
        compile_class(t, infile, outfile, &linenum, classtable, subtable); // should only be one class per .jack file, so we don't need a loop
    }

    // cleanup
    free_token_fields(t);
    free(t);
    free_symbol_table_nodes(classtable);
    free_symbol_table_nodes(subtable);
    free(classtable);
    free(subtable);
}


/*
* Fill the token fields with default values, allocates memory for name and symboldata,
* and initializes the symboldata node.
*/
void initialize_token(token* t)
{
    t->name = malloc(MAX_TOKEN_LENGTH * sizeof(*(t->name)));

    if (t->name == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for t->name\n");
        exit(1);
    }

    t->type = T_DEFAULT;
    t->key = K_NA;
    t->symboldata = malloc(sizeof(*(t->symboldata)));

    if (t->symboldata == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for t->symboldata\n");
        exit(1);
    }

    initialize_blank_node(t->symboldata);
}

void free_token_fields(token* t)
{
    free(t->name);
    free(t->symboldata);
}

/*
* Reads characters from infile, skipping comment lines/blocks and spaces,
* until it finds and returns a valid token character.
* Increments linenum upon encountering a newline.
*/
char find_next_token_start(FILE* infile, int* linenum)
{
    char c = fgetc(infile);
    char c2 = '\0';

    while (!feof(infile) && (c == ' ' || c == '\t' || c == '/' || c == '\n'))
    {
        if (c == '/')
        {
            // check to see if it's the start of a comment
            c2 = fgetc(infile);
            if (c2 == '/')
            {
                skip_comment_line(infile);
                (*linenum)++;
            }
            else if (c2 == '*')
            {
                int skippedlines = skip_comment_block(infile);
                (*linenum) += skippedlines;
            }
            else
            {
                ungetc(c2, infile); // put c2 back into the input stream, since this isn't a comment after all
                break;
            }
        }
        else if (c == '\n')
        {
            (*linenum)++;
        }
        c = fgetc(infile);
    }
    return c;
}

/*
* Returns the next valid token character from infile, but puts the character
* back into the input stream after reading so the file position is either not
* advanced, or is only advanced until just before the next valid token.
* Unlike get_next_token_start(), this function does not check for comments
* mid-line. It should only be called in the middle of compiling an expression
* and assumes the expression (and the next token) is valid.
*/
char peek_at_next_token_start(FILE* infile, int* const linenum)
{
    char c = fgetc(infile);
    while (!feof(infile) && (c == ' ' || c == '\t' || c == '\n'))
    {
        if (c == '\n')
        {
            (*linenum)++;
        }
        c = fgetc(infile);
    }
    ungetc(c, infile);
    return c;
}

/*
* Reads a string of characters from infile that represent a .jack code token, delineated
* by whitespace or special characters, and copies it into token.
*/
void get_next_token(token* t, FILE* infile, int* linenum)
{
    char c = find_next_token_start(infile, linenum);
    assert(c != ' '); // DEBUG

    if (c == EOF)
    {
        t->name[0] = '\0';
        t->type = T_EOF;
    }
    else
    {
        if (c == '{' || c == '}' || c == '(' || c == ')' || c == '[' || c == ']' || c == '.' || c == ','
        || c == ';' || c == '+' || c == '-' || c == '*' || c == '/' || c == '&' || c == '|' || c == '<'
        || c == '>' || c == '=' || c == '~')
        {
            t->name[0] = c;
            t->name[1] = '\0';
            set_token_type(t);
            set_token_key(t);
        }
        else if (c == '"') // start of a string constant, copy until next ", include double quotes but not newlines
        {
            t->name[0] = '"';
            size_t maxlength = MAX_TOKEN_LENGTH;
            size_t i = 1;
            c = fgetc(infile);

            while (!feof(infile) && c != '"')
            {
                if (c == '\n')
                {
                    (*linenum)++;
                }
                else
                {
                    // every time we reach (last index - 1) of token, double the allocated number of chars
                    // make sure we always leave room for the closing " and NUL
                    if (i == (maxlength - 2))
                    {
                        t->name = realloc(t->name, (maxlength *= 2) * sizeof(*(t->name)));
                        if (t->name == NULL)
                        {
                            fprintf(stderr, "Error: could not reallocate memory for token\n");
                            exit(1);
                        }
                    }
                    t->name[i] = c;
                    i++;
                }
                c = fgetc(infile);
            }
            t->name[i] = '"';
            t->name[i+1] = '\0';
        }
        else if (c >= '0' && c <= '9')
        {
            int i = 0;
            while (!feof(infile) && (c >= '0' && c <= '9'))
            {
                t->name[i] = c;
                i++;
                c = fgetc(infile);
            }
            ungetc(c, infile); // last char read wasn't a digit, so put it back in the input stream in case it's part of the next token
            t->name[i] = '\0';
        }
        else
        {
            // copy until whitespace, newline, or special character
            int i = 0;
            while (!feof(infile))
            {
                if (c == ' ' || c == '\n' ||c == '{' || c == '}' || c == '(' || c == ')' || c == '[' || c == ']'
                    || c == '.' || c == ',' || c == ';' || c == '+' || c == '-' || c == '*' || c == '/' || c == '&'
                    || c == '|' || c == '<' || c == '>' || c == '=' || c == '~')
                {
                    break;
                }
                else
                {
                    t->name[i] = c;
                    i++;
                    c = fgetc(infile);
                }
            }
            ungetc(c, infile); // last char read wasn't part of this token, so put it back in the input stream
            t->name[i] = '\0';
        }
    set_token_type(t);
    set_token_key(t);
    }
}



/*
* Sets the type field of the token based on the token's name field.
*/
void set_token_type(token* const t)
{
    if (t->name[0] >= '0' && t->name[0] <= '9')
    {
        t->type = T_INT_CONST;
    }
    else if (t->name[0] == '"')
    {
        t->type = T_STRING_CONST;
    }
    else if (t->name[0] == '{' || t->name[0] == '}' || t->name[0] == '(' || t->name[0] == ')' || t->name[0] == '[' || t->name[0] == ']'
             || t->name[0] == '.' || t->name[0] == ',' || t->name[0] == ';' || t->name[0] == '+' || t->name[0] == '-' || t->name[0] == '*'
             || t->name[0] == '/' || t->name[0] == '&' || t->name[0] == '|' || t->name[0] == '<' || t->name[0] == '>' || t->name[0] == '='
             || t->name[0] == '~')
    {
        t->type = T_SYMBOL;
    }
    else if (strcmp(t->name, "class") == 0 || strcmp(t->name, "constructor") == 0 || strcmp(t->name, "function") == 0 || strcmp(t->name, "method") == 0
        || strcmp(t->name, "field") == 0 || strcmp(t->name, "static") == 0 || strcmp(t->name, "var") == 0 || strcmp(t->name, "int") == 0
        || strcmp(t->name, "char") == 0 || strcmp(t->name, "boolean") == 0 || strcmp(t->name, "void") == 0 || strcmp(t->name, "true") == 0
        || strcmp(t->name, "false") == 0 || strcmp(t->name, "null") == 0 || strcmp(t->name, "this") == 0 || strcmp(t->name, "let") == 0
        || strcmp(t->name, "do") == 0 || strcmp(t->name, "if") == 0 || strcmp(t->name, "else") == 0 || strcmp(t->name, "while") == 0
        || strcmp(t->name, "return") == 0)
    {
        t->type = T_KEYWORD;
    }
    else // assume token is valid and all other tokens must be identifiers
    {
        t->type = T_IDENTIFIER;
    }
}

/*
* Sets the key field of the token, based on the token's name.
*/
void set_token_key(token* t)
{
    if (strcmp(t->name, "class") == 0) {
        t->key = K_CLASS;
    }
    else if (strcmp(t->name, "constructor") == 0) {
        t->key = K_CONSTRUCTOR;
    }
    else if (strcmp(t->name, "function") == 0) {
        t->key = K_FUNCTION;
    }
    else if (strcmp(t->name, "method") == 0) {
        t->key = K_METHOD;
    }
    else if (strcmp(t->name, "field") == 0) {
        t->key = K_FIELD;
    }
    else if (strcmp(t->name, "static") == 0) {
        t->key = K_STATIC;
    }
    else if (strcmp(t->name, "var") == 0) {
        t->key = K_VAR;
    }
    else if (strcmp(t->name, "int") == 0) {
        t->key = K_INT;
    }
    else if (strcmp(t->name, "char") == 0) {
        t->key = K_CHAR;
    }
    else if (strcmp(t->name, "boolean") == 0) {
        t->key = K_BOOLEAN;
    }
    else if (strcmp(t->name, "void") == 0) {
        t->key = K_VOID;
    }
    else if (strcmp(t->name, "true") == 0) {
        t->key = K_TRUE;
    }
    else if (strcmp(t->name, "false") == 0) {
        t->key = K_FALSE;
    }
    else if (strcmp(t->name, "null") == 0) {
        t->key = K_NULL;
    }
    else if (strcmp(t->name, "this") == 0) {
        t->key = K_THIS;
    }
    else if (strcmp(t->name, "let") == 0) {
        t->key = K_LET;
    }
    else if (strcmp(t->name, "do") == 0) {
        t->key = K_DO;
    }
    else if (strcmp(t->name, "if") == 0) {
        t->key = K_IF;
    }
    else if (strcmp(t->name, "else") == 0) {
        t->key = K_ELSE;
    }
    else if (strcmp(t->name, "while") == 0) {
        t->key = K_WHILE;
    }
    else if (strcmp(t->name, "return") == 0) {
        t->key = K_RETURN;
    }
    else {
        t->key = K_NA; // default for non-keyword tokens
    }
}

/*
* For one-character token, returns the single character instead of the char* string.
*/
char get_symbol(const token* t)
{
    assert (t->type == T_SYMBOL); // DEBUG, but shouldn't cause problems if called on other types
    return t->name[0];
}


/*
* Returns the integer value of the current token. Should only be called when
* type is T_INT_CONST.
*/
int get_intval(const token* t)
{
    assert(t->type == T_INT_CONST); // DEBUG
    return atoi(t->name);
}

/*
* Copies the string contents of t->name into destination, ignoring double quotes.
*/
void get_stringval(char* destination, const token* t)
{
    size_t i = 0;
    for (size_t j = 0; j < strlen(t->name); j++)
    {
        if (t->name[j] != '"')
        {
            destination[i] = t->name[j];
            i++;
        }
    }
    destination[i] = '\0';
}


/*
* Reads from the file until the end of the line, ignoring whatever is read.
*/
void skip_comment_line(FILE* infile)
{
    char c = '\0';
    while (!feof(infile) && c != '\n')
    {
        c = fgetc(infile);
    }
}


/*
* Reads from the file until the star-slash combination is encountered, ignoring
* whatever is read. Returns the number of newlines encountered.
*/
int skip_comment_block(FILE* infile)
{
    int skippedlines = 0;
    char c = '\0';
    char c2 = '\0';
    while (!feof(infile))
    {
        c = fgetc(infile);
        if (c == '\n')
        {
            skippedlines++;
        }
        if (c == '*' && !feof(infile))
        {
            // might be the end of the comment block, check next character
            c2 = fgetc(infile);
            if (c2 == '/')
            {
                break;
            }
            else
            {
                ungetc(c2, infile);
            }
        }
    }
    return skippedlines;
}


/*
* Determines the appropriate XML tags for the current token, then prints the token,
* wrapped in tags and offset with spaces, e.g: <tag> token </tag>
*/
void print_terminal_with_tags(const token* t, const symboltable* st, FILE* outfile, const int* const indent)
{
    char* tagtype = malloc(32 * sizeof(*tagtype));
    char* namecopy = malloc((strlen(t->name) + 1 ) * sizeof(*namecopy)); // +1 for NUL
    if (tagtype == NULL || namecopy == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for either tagtype or namecopy\n");
    }
    else
    {
        strcpy(namecopy, t->name);

        switch (t->type)
        {
            case T_KEYWORD:
                strcpy(tagtype, "keyword");
                break;
            case T_SYMBOL:
                strcpy(tagtype, "symbol");
                char symbol = get_symbol(t);
                if (symbol == '<' || symbol == '>' || symbol == '&')
                {
                    // need more space for converted symbols
                    // "&amp;" is the longest, so make sure we have room for 5 chars + 1 NUL
                    char* temp = realloc(namecopy, 6 * sizeof(*temp));
                    if (temp == NULL)
                    {
                        fprintf(stderr, "Error: could not reallocate memory for namecopy\n");
                        strcpy(namecopy, "0");
                    }
                    else
                    {
                        namecopy = temp;
                        if (get_symbol(t) == '<')
                        {
                            strcpy(namecopy, "&lt;");
                        }
                        else if (get_symbol(t) == '>')
                        {
                            strcpy(namecopy, "&gt;");
                        }
                        else if (get_symbol(t) == '&')
                        {
                            strcpy(namecopy, "&amp;");
                        }
                    }
                }
                break;
            case T_INT_CONST:
                strcpy(tagtype, "integerConstant");
                break;
            case T_STRING_CONST:
                strcpy(tagtype, "stringConstant");
                get_stringval(namecopy, t);
                break;
            case T_IDENTIFIER:
                strcpy(tagtype, "identifier");
                break;
            default:
                strcpy(tagtype, "DEFAULT");
        }

        fprintf(outfile, "%*s<%s> %s </%s>\n", *indent, "", tagtype, namecopy, tagtype);

        print_symboldata_for_identifiers(t, st, outfile, indent); // DEBUG

        free(tagtype);
        free(namecopy);
    }
}


/*
* Prints symbol information for identifier tokens. Used for debugging, will
* show what needs to be added to the symboltable.
*/
void print_symboldata_for_identifiers(const token* t, const symboltable* st, FILE* outfile, const int* const indent)
{
    if (t->type == T_IDENTIFIER)
    {
        strcpy(t->symboldata->name, t->name);

        fprintf(outfile, "%*sNAME: %s, TYPE: %s, KIND: %s, ", (*indent) + INDENT_WIDTH, "", t->symboldata->name, t->symboldata->type,
            convert_symbolkind_to_string(t->symboldata->kind));

        if (t->symboldata->is_being_defined == true || t->symboldata->kind == SK_ARG)
        {
            if (t->symboldata->kind == SK_STATIC)
            {
                fprintf(outfile, "STATICINDEX: %u\n", st->staticindex);
            }
            else if (t->symboldata->kind == SK_FIELD)
            {
                fprintf(outfile, "FIELDINDEX: %u\n", st->fieldindex);
            }
            else if (t->symboldata->kind == SK_ARG)
            {
                fprintf(outfile, "ARGINDEX: %u\n", st->argindex);
            }
            else if (t->symboldata->kind == SK_VAR)
            {
                fprintf(outfile, "VARINDEX: %u\n", st->varindex);
            }
        }
        else
        {
            fprintf(outfile, "INDEX: NA\n");
        }
    }
}

/*
* Creates a new tablenode with the same attributes as t->symboldata, then
* appends the node to symboltable and updates the indices accordingly.
* NOTE: accepts all tokens, but only adds identifiers to symboltable if they
* are being defined, or if they are being passed as arguments.
*/
void copy_symboldata_into_symbol_table(const token* t, symboltable* st)
{
    if (t->type == T_IDENTIFIER && (t->symboldata->is_being_defined == true || t->symboldata->kind == SK_ARG))
    {
        strcpy(t->symboldata->name, t->name);

        tablenode* newnode = malloc(sizeof(*newnode));
        if (newnode == NULL)
        {
            fprintf(stderr, "Error: could not allocate memory for newnode\n");
        }
        else
        {
            initialize_blank_node(newnode);

            strcpy(newnode->name, t->symboldata->name);
            strcpy(newnode->type, t->symboldata->type);
            newnode->kind = t->symboldata->kind;

            switch (newnode->kind)
            {
                case SK_STATIC:
                    newnode->index = st->staticindex;
                    (st->staticindex)++;
                    break;
                case SK_FIELD:
                    newnode->index = st->fieldindex;
                    (st->fieldindex)++;
                    break;
                case SK_ARG:
                    newnode->index = st->argindex;
                    (st->argindex)++;
                    break;
                case SK_VAR:
                    newnode->index = st->varindex;
                    (st->varindex)++;
                default:
                    break;
            }

            append_node(st, newnode);
        }
    }
}


bool is_statement(const token* const t)
{
    // K_ELSE is not included because else is already a part of an if statement
	if (t->key == K_LET || t->key == K_IF || t->key == K_WHILE
        || t->key == K_DO || t->key == K_RETURN)
	{
	    return true;
	}
	else
    {
        return false;
    }
}


bool is_binary_operator(const token* const t)
{
    if (t->type == T_SYMBOL && (get_symbol(t) == '+' || get_symbol(t) == '-' || get_symbol(t) == '*'
        || get_symbol(t) == '/' || get_symbol(t) == '&' || get_symbol(t) == '|' || get_symbol(t) == '<'
        || get_symbol(t) == '>' || get_symbol(t) == '='))
    {
        return true;
    }
    else
    {
        return false;
    }
}


bool is_unary_operator(const token* const t)
{
    if (t->type == T_SYMBOL && (get_symbol(t) == '-' || get_symbol(t) == '~'))
    {
        return true;
    }
    else
    {
        return false;
    }
}
