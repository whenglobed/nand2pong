#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include "filehandling.h"
#include "jacktokenizer.h"

/*
* helper function that checks filename for .jack extension
*/
bool is_jack_file(const char* const filename)
{
    char* period = strchr(filename, '.');
    if (period == NULL)
    {
        return false;
    }
    else
    {
        if (strcmp(period, ".jack") == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

/*
* Opens directory and tokenizes all .jack files inside, creating a
* separate .xml output file for each one. Returns false if unable to
* open the provided name as a directory.
*/
bool tokenize_directory(const char* const directoryname)
{
    fprintf(stdout, "Attempting to open %s as a directory...\n", directoryname);

    DIR* directory = opendir(directoryname);
    if (directory == NULL)
    {
        fprintf(stdout, "Could not open %s as a directory.\n", directoryname);
        return false;
    }
    else
    {
        fprintf(stdout, "...success!\n", directoryname);

        FILE* infile = NULL;
        FILE* outfile = NULL;
        struct dirent* currententry;

        while ((currententry = readdir(directory)) != NULL)
        {
            if (is_jack_file(currententry->d_name)) // only attempt to parse .jack files
            {
                char* currentfilename = malloc((strlen(directoryname) + strlen(currententry->d_name) + 2) * sizeof(char)); // +1 for NUL, +1 for '\\'
                if (currentfilename == NULL)
                {
                    fprintf(stderr, "Error: could not allocate memory for currentfilename\n");
                }
                else
                {
                    // build an (abbreviated) filepath for the files in the directory
                    strcpy(currentfilename, directoryname);
                    strcat(currentfilename, "\\");
                    strcat(currentfilename, currententry->d_name);
                    if ((infile = fopen(currentfilename, "r")) == NULL)
                    {
                        fprintf(stderr, "Error: could not open file %s\n", currentfilename);
                    }
                    else
                    {
                        outfile = create_output_xml_file(currentfilename);
                        if (outfile == NULL)
                        {
                            fprintf(stderr, "Error: could not open outfile\n");
                        }
                        else
                        {
                            fprintf(stdout, "Tokenizing %s...\n", currentfilename);
                            tokenize(infile, outfile);
                        }
                    }
                    fclose(infile);
                    fclose(outfile);
                }
                free(currentfilename);
                currentfilename = NULL;
            }
        }
        closedir(directory);
        return true;
    }
}


/*
* Opens directory and compiles all .jack files inside, creating a
* separate .vm output file for each one. Returns false if unable to
* open the provided name as a directory.
*/
bool compile_directory(const char* const directoryname)
{
    fprintf(stdout, "Attempting to open %s as a directory...\n", directoryname);

    DIR* directory = opendir(directoryname);
    if (directory == NULL)
    {
        fprintf(stdout, "Could not open %s as a directory.\n", directoryname);
        return false;
    }
    else
    {
        fprintf(stdout, "...success!\n", directoryname);

        FILE* infile = NULL;
        FILE* outfile = NULL;
        struct dirent* currententry;

        while ((currententry = readdir(directory)) != NULL)
        {
            if (is_jack_file(currententry->d_name)) // only attempt to parse .jack files
            {
                char* currentfilename = malloc((strlen(directoryname) + strlen(currententry->d_name) + 2) * sizeof(char)); // +1 for NUL, +1 for '\\'
                if (currentfilename == NULL)
                {
                    fprintf(stderr, "Error: could not allocate memory for currentfilename\n");
                }
                else
                {
                    // build an (abbreviated) filepath for the files in the directory
                    strcpy(currentfilename, directoryname);
                    strcat(currentfilename, "\\");
                    strcat(currentfilename, currententry->d_name);
                    if ((infile = fopen(currentfilename, "r")) == NULL)
                    {
                        fprintf(stderr, "Error: could not open file %s\n", currentfilename);
                    }
                    else
                    {
                        outfile = create_output_vm_file(currentfilename);
                        if (outfile == NULL)
                        {
                            fprintf(stderr, "Error: could not open outfile\n");
                        }
                        else
                        {
                            fprintf(stdout, "Compiling %s...\n", currentfilename);
                            compile(infile, outfile);
                        }
                    }
                    fclose(infile);
                    fclose(outfile);
                }
                free(currentfilename);
                currentfilename = NULL;
            }
        }
        closedir(directory);
        return true;
    }
}


/*
/ Tokenizes a single .jack file, creating a single .xml file as output.
*/
void tokenize_single_file(const char* const infilename)
{
    fprintf(stdout, "Attempting to open %s as a single file...\n", infilename);

    FILE* infile = NULL;
    FILE* outfile = NULL;

    if (!is_jack_file(infilename))
    {
        fprintf(stderr, "Error: %s is not a .jack file\n", infilename);
    }
    else
    {
        if ((infile = fopen(infilename, "r")) == NULL)
        {
            fprintf(stderr, "Error: could not open file %s\n", infilename);
        }
        else
        {
            fprintf(stdout, "...success!\n");

            outfile = create_output_xml_file(infilename);
            if (outfile == NULL)
            {
                fprintf(stderr, "Error: could not open outfile\n");
            }
            else
            {
                fprintf(stdout, "Tokenizing %s...\n", infilename);
                tokenize(infile, outfile);
            }
        }
    }
}


/*
/ Compiles a single .jack file, creating a single .vm file as output.
*/
void compile_single_file(const char* const infilename)
{
    fprintf(stdout, "Attempting to open %s as a single file...\n", infilename);

    FILE* infile = NULL;
    FILE* outfile = NULL;

    if (!is_jack_file(infilename))
    {
        fprintf(stderr, "Error: %s is not a .jack file\n", infilename);
    }
    else
    {
        if ((infile = fopen(infilename, "r")) == NULL)
        {
            fprintf(stderr, "Error: could not open file %s\n", infilename);
        }
        else
        {
            fprintf(stdout, "...success!\n");

            outfile = create_output_vm_file(infilename);
            if (outfile == NULL)
            {
                fprintf(stderr, "Error: could not open outfile\n");
            }
            else
            {
                fprintf(stdout, "Compiling %s...\n", infilename);
                compile(infile, outfile);
            }
        }
    }
}


/*
* Creates an .xml filename to match the .jack input filename and opens
* the file for writing.
*/
FILE* create_output_xml_file(const char* const infilename)
{
    assert(is_jack_file(infilename)); // DEBUG

    FILE* outfile = NULL;
    char* outfilename = malloc((strlen(infilename) + 1) * sizeof(*outfilename)); // +1 for NUL
    if (outfilename == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for outfilename\n");
    }
    else
    {
        outfilename = strcpy(outfilename, infilename);
        char* period = strchr(outfilename, '.');
        period[1] = '\0'; // period is a pointer within outfilename, so this should affect outfilename as well
        strcat(period, "xml");
        outfile = fopen(outfilename, "w");
    }
    free(outfilename);

    return outfile;
}

/*
* Creates a .vm filename to match the .jack input filename and opens
* the file for writing.
*/
FILE* create_output_vm_file(const char* const infilename)
{
    assert(is_jack_file(infilename)); // DEBUG

    FILE* outfile = NULL;
    char* outfilename = malloc((strlen(infilename) + 1) * sizeof(*outfilename)); // +1 for NUL
    if (outfilename == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for outfilename\n");
    }
    else
    {
        outfilename = strcpy(outfilename, infilename);
        char* period = strchr(outfilename, '.');
        period[1] = '\0'; // period is a pointer within outfilename, so this should affect outfilename as well
        strcat(period, "vm");
        outfile = fopen(outfilename, "w");
    }
    free(outfilename);

    return outfile;
}


