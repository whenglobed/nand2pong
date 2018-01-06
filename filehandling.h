#ifndef FILEHANDLING_H
#define FILEHANDLING_H

#include <stdio.h>
#include <stdbool.h>

bool is_jack_file(const char* const filename);
bool tokenize_directory(const char* const directoryname);
bool compile_directory(const char* const directoryname);
void tokenize_single_file(const char* const infilename);
void compile_single_file(const char* const infilename);
FILE* create_output_xml_file(const char* const infilename); // creates an .xml filename to match .jack input filename, opens file for writing
FILE* create_output_vm_file(const char* const infilename); // creates a .vm filename to match .jack input filename, opens file for writing

#endif // FILEHANDLING_H
