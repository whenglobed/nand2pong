/*****************************************************************************************************************
* Elements of Computing Systems, Chapter 11
* Compiler II: Code Generation
*
* This program parses .jack files and translates Jack code
* into VM code. If a directory name is provided, it will
* translate each .jack file in that directory into its own
* .vm file (ignoring sub-directories).
*
* NOTES:
* 1) When the user provides a directory name, this program
* expects that directory to be located in the same folder as
* elements_11.exe. It does not handle complete filepaths
* (e.g. C:\Program Files\somedirectory\somefile.jack).
*
* 2) This program assumes the first period '.' in the filename
* marks the beginning of the file extension. Filepaths with
* multiple periods will cause errors.
*
* TODO:
* 1) Replace "char c" with "int c" anytime fgetc() is being used, since fgetc() returns type int.
* 2) Add compilation errors with filenames and linenums whenever an unexpected token is encountered.
*       -Make a general check_expected_token() function?
* 3) Break up functions in general.
* 4) Expand functionality to handle complete filepaths.
* 6) Be smarter when checking file extensions. Start at end of filename and search backward for periods?
******************************************************************************************************************/

#include "filehandling.h"

/*
* Main function.
* Attempts to open the provided filename as a directory and compile all
* .jack files within. If that fails, attempts to compile it as a single file.
*/
int main(int argc, char** argv)
{
    if (argc != 2) // ensure correct usage
    {
        fprintf(stderr, "Usage: elements_11 [directory name or filename.jack]\n");
        return 1;
    }

    // the tokenizing functions create XML output for debugging purposes
    // they do not affect the actual VM compilation
    if (tokenize_directory(argv[1]) == false || compile_directory(argv[1]) == false)
    {
        tokenize_single_file(argv[1]);
        compile_single_file(argv[1]);
    }

    return 0;
}
