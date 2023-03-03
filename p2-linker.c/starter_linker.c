/**
 * Project 2
 * LC-2K Linker
 */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXSIZE 300
#define MAXLINELENGTH 1000
#define MAXFILES 6

typedef struct FileData FileData;
typedef struct SymbolTableEntry SymbolTableEntry;
typedef struct RelocationTableEntry RelocationTableEntry;
typedef struct CombinedFiles CombinedFiles;


struct SymbolTableEntry {
	char label[7];
	char location;
	int offset;
};

struct RelocationTableEntry {
	int offset;
	char inst[7];
	char label[7];
	int file;
};

struct FileData {
	int textSize;
	int dataSize;
	int symbolTableSize;
	int relocationTableSize;
	int textStartingLine; // in final executable
	int dataStartingLine; // in final executable
	int text[MAXSIZE];
	int data[MAXSIZE];
	SymbolTableEntry symbolTable[MAXSIZE];
	RelocationTableEntry relocTable[MAXSIZE];
};

struct CombinedFiles {
	int text[MAXSIZE];
	int data[MAXSIZE];
	SymbolTableEntry     symTable[MAXSIZE];
	RelocationTableEntry relocTable[MAXSIZE];
	int textSize;
	int dataSize;
	int symTableSize;
	int relocTableSize;
};

CombinedFiles combined_init(struct FileData files[], const int num_files);
void create_symbol_table(CombinedFiles *, struct FileData files[], const int num_files);
bool error_duplicate_definition(CombinedFiles *, struct FileData files[], int curr_size, char *label);
void stack_error(CombinedFiles *, struct FileData files[], const int num_files);

// for testing
void print_symbols(struct CombinedFiles combined);
void print_relocation(struct CombinedFiles combined);
void print_text(struct CombinedFiles combined);
void print_data(struct CombinedFiles combined);
void print_starting_lines(struct FileData files[], int num_files);

void parse_linked_files(CombinedFiles *, struct FileData files[], int num_files);
void resolve_local(CombinedFiles *, struct FileData files[], int r);
void resolve_global(CombinedFiles *, struct FileData files[], int r);

void final_output(struct CombinedFiles combined, FILE *outFilePtr);

bool is_data(struct CombinedFiles combined, int line_number, int file_text_size, bool fill);

int main(int argc, char *argv[])
{
	char *inFileString, *outFileString;
	FILE *inFilePtr, *outFilePtr; 
	int i, j;

	if (argc <= 2) {
		printf("error: usage: %s <obj file> ... <output-exe-file>\n",
				argv[0]);
		exit(1);
	}

	outFileString = argv[argc - 1];

	outFilePtr = fopen(outFileString, "w");
	if (outFilePtr == NULL) {
		printf("error in opening %s\n", outFileString);
		exit(1);
	}

	FileData files[MAXFILES];

  // read in all files and combine into a "master" file
	for (i = 0; i < argc - 2; i++) {
		inFileString = argv[i+1];

		inFilePtr = fopen(inFileString, "r");
		printf("opening %s\n", inFileString);

		if (inFilePtr == NULL) {
			printf("error in opening %s\n", inFileString);
			exit(1);
		}

		char line[MAXLINELENGTH];
		int sizeText, sizeData, sizeSymbol, sizeReloc;

		// parse first line of file
		fgets(line, MAXSIZE, inFilePtr);
		sscanf(line, "%d %d %d %d",
				&sizeText, &sizeData, &sizeSymbol, &sizeReloc);

		files[i].textSize = sizeText;
		files[i].dataSize = sizeData;
		files[i].symbolTableSize = sizeSymbol;
		files[i].relocationTableSize = sizeReloc;

		// read in text section
		int instr;
		for (j = 0; j < sizeText; j++) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			instr = atoi(line);
			files[i].text[j] = instr;
		}

		// read in data section
		int data;
		for (j = 0; j < sizeData; j++) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			data = atoi(line);
			files[i].data[j] = data;
		}

		// read in the symbol table
		char label[7];
		char type;
		int addr;
		for (j = 0; j < sizeSymbol; j++) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%s %c %d",
					label, &type, &addr);
			files[i].symbolTable[j].offset = addr;
			strcpy(files[i].symbolTable[j].label, label);
			files[i].symbolTable[j].location = type;
		}

		// read in relocation table
		char opcode[7];
		for (j = 0; j < sizeReloc; j++) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%d %s %s",
					&addr, opcode, label);
			files[i].relocTable[j].offset = addr;
			strcpy(files[i].relocTable[j].inst, opcode);
			strcpy(files[i].relocTable[j].label, label);
			files[i].relocTable[j].file	= i;
		}
		fclose(inFilePtr);
	} // end reading files

	// *** INSERT YOUR CODE BELOW ***
	//    Begin the linking process
	//    Happy coding!!!
    
    // iterate through all files
    const int num_files = argc - 2;
    struct CombinedFiles linked_files = combined_init(files, num_files);
    
    parse_linked_files(&linked_files, files, num_files);
    
    // output text + data from resolved linked files
    final_output(linked_files, outFilePtr);

} // main


CombinedFiles combined_init(struct FileData files[], const int num_files) {
    
    struct CombinedFiles linked_files;
    
    // used as a counter for total text lines in all files
    int i = 0;
    
    // iterate through each file's text, and add it to the combined text
    for (int f = 0; f < num_files; f++) {
        // also initialize the start of each file's text section for logic later
        files[f].textStartingLine = i;
        for (int t = 0; t < files[f].textSize; t++) {
            linked_files.text[i] = files[f].text[t];
            i++;
        }
    }
    
    linked_files.textSize = i;
    
    // let i be used as a counter for total data lines in all files
    i = 0;
    // iterate through each file's data, and add it to the combined data
    for (int f = 0; f < num_files; f++) {
        // also initialize the start of each file's data section for logic later
        files[f].dataStartingLine = i;
        for (int d = 0; d < files[f].dataSize; d++) {
            linked_files.data[i] = files[f].data[d];
            i++;
        }
    }
    
    linked_files.dataSize = i;
    
//    // let i be used to count the number of symbols
//    i = 0;
//
//    // TODO: symbols
//    // iterate through each file's symbol table, and add it to the combined data
//    // and while updating the offsets
//    for (int f = 0; f < num_files; f++) {
//        for (int s = 0; s < files[f].symbolTableSize; s++) {
//            linked_files.symTable[i] = files[f].symbolTable[s];
//            i++;
//        }
//    }
//
//    linked_files.symTableSize = i;
    create_symbol_table(&linked_files, files, num_files);
    
    // let i be used to count the number of relocations
    i = 0;
    
    // iterate through each file's relocation table, and add it to the combined data
    for (int f = 0; f < num_files; f++) {
        for (int r = 0; r < files[f].relocationTableSize; r++) {
            linked_files.relocTable[i] = files[f].relocTable[r];
            i++;
        }
    }
    
    linked_files.relocTableSize = i;
    
    
    // for testing
    print_text(linked_files);
    print_data(linked_files);
    print_symbols(linked_files);
    print_relocation(linked_files);
    print_starting_lines(files, num_files);
    return linked_files;
}

void create_symbol_table(CombinedFiles *combined, struct FileData files[], const int num_files) {
    
    // current size of new symbol table
    int i = 0;
    // iterate through each file's symbol table
    for (int f = 0; f < num_files; f++) {
        for (int s = 0; s < files[f].symbolTableSize; s++) {
            
            // check that there isn't a label definition for this already
            if (files[f].symbolTable[s].location != 'U') {
                if (error_duplicate_definition(combined, files, i, files[f].symbolTable[s].label)) {
                    exit(1);
                }
            }
            
            // if symbol was in the data section of the original file
            if (files[f].symbolTable[s].location == 'D') {
                // we want to add the entire combined text size + any previous data sections
                int combined_text_size = combined->textSize;
                int previous_data_size = files[f].dataStartingLine;
                
                files[f].symbolTable[s].offset += combined_text_size +previous_data_size;
                combined->symTable[i] = files[f].symbolTable[s];
                i++;
            }
            
            // if symbol was in the text section of the original file
            else if (files[f].symbolTable[s].location == 'T') {
                // we want to just add the previous text sections
                int previous_text_size = files[f].textStartingLine;
                
                files[f].symbolTable[s].offset += previous_text_size;
                combined->symTable[i] = files[f].symbolTable[s];
                i++;
            }
        } // for s
    } // for f
    
    // initialize linked symbol table size
    combined->symTableSize = i;
    
    // check that stack is not defined
    stack_error(combined, files, num_files);
}

void stack_error(CombinedFiles *combined, struct FileData files[], const int num_files) {
    for (int f = 0; f < num_files; f++) {
        for (int s = 0; s < files[f].symbolTableSize; s++) {
            if (!strcmp(files[f].symbolTable[s].label, "Stack") && files[f].symbolTable[s].location != 'U') {
                exit(1);
            }
        }
    }
//    for (int s = 0; s < combined->symTableSize; s++) {
//        if (!strcmp(combined->symTable[s].label, "Stack") && combined->symTable[s].location != 'U') {
//            exit(1);
//        }
//    }
}

// TODO: fix
bool error_duplicate_definition(CombinedFiles *combined, struct FileData files[], int curr_size, char *label) {
    
    // iterate through current linked symbol table and return true if a label definition is already present
    for (int s = 0; s < curr_size; s++) {
        if (!strcmp(label, combined->symTable[s].label)) {
            return true;
        }
    }
    return false;
}



void parse_linked_files(CombinedFiles *combined, struct FileData files[], int num_files) {
    
    // need to resolve everything in relocation table
    for (int r = 0; r < combined->relocTableSize; r++) {

        // local label needs to be resolved
        if (!isupper(combined->relocTable[r].label[0])) {
            resolve_local(combined, files, r);
        }
        
        // global label needs to be resolved
        else {
            resolve_global(combined, files, r);
        }
    }
}


void resolve_global(CombinedFiles *combined, struct FileData files[], int r) {
    int line_number;
//    int new_offset;
//    bool fill;
    
    
    // STACK
    if (!strcmp(combined->relocTable[r].label, "Stack")) {
        // figure out the line number depending if it is in text or data
        
        // text line
        if (strcmp(combined->relocTable[r].inst, ".fill")) {
            int line_number = files[combined->relocTable[r].file].textStartingLine + combined->relocTable[r].offset;
            combined->text[line_number] += combined->textSize + combined->dataSize;
        }
        
        // data line
        else {
            int line_number = files[combined->relocTable[r].file].dataStartingLine + combined->relocTable[r].offset;
            combined->data[line_number] += combined->textSize + combined->dataSize;
        }
        return;
    }
    // look for where the label is defined in the symbol table
    for (int s = 0; s < combined->symTableSize; s++) {
        if (!strcmp(combined->relocTable[r].label, combined->symTable[s].label)) {
            // if it is not .fill, the line will be in the text section
            if (strcmp(combined->relocTable[r].inst, ".fill")) {
                line_number = files[combined->relocTable[r].file].textStartingLine + combined->relocTable[r].offset;
                
                // zero it out
                combined->text[line_number] = combined->text[line_number] & 0xFFFF0000;
                
                combined->text[line_number] += combined->symTable[s].offset;
            }
            
            // line will be in the data section
            else {

                line_number = files[combined->relocTable[r].file].dataStartingLine + combined->relocTable[r].offset;
                
                // zero it out
                combined->data[line_number] = 0;
                
                combined->data[line_number] += combined->symTable[s].offset;
            }
            
            return;
        }
    }
    
    // label definition was not found, return error
    exit(1);
}


void resolve_local(CombinedFiles *combined, struct FileData files[], int r) {
    
    int line_number;
    int new_offset;
    bool fill;
    
    // if it is not a .fill, the line we need to resolve is in the text section
    if (strcmp(combined->relocTable[r].inst, ".fill")) {
        line_number = files[combined->relocTable[r].file].textStartingLine + combined->relocTable[r].offset;
        fill = false;
        
        int file_text_size = files[combined->relocTable[r].file].textSize;
        bool isData = is_data(*combined, line_number, file_text_size, fill);
        
        
        // resolve labels that are declared in data section
        if (isData) {
            new_offset = combined->textSize - file_text_size + files[combined->relocTable[r].file].dataStartingLine;
            combined->text[line_number] += new_offset;
            
        }
        
        // resolve labels that are declared in text section
        else {
            combined->text[line_number] += files[combined->relocTable[r].file].textStartingLine;
        }
        
    }
    
    // .fill, we need to resolve data section
    else {
        line_number = files[combined->relocTable[r].file].dataStartingLine + combined->relocTable[r].offset;
        fill = true;
        
        int file_text_size = files[combined->relocTable[r].file].textSize;
        bool isData = is_data(*combined, line_number, file_text_size, fill);
        
        
        // resolve labels that are declared in data section
        if (isData) {
            new_offset = combined->textSize - file_text_size + files[combined->relocTable[r].file].dataStartingLine;
            combined->data[line_number] += new_offset;
            
        }
        
        // resolve labels that are declared in text section
        else {
            combined->data[line_number] += files[combined->relocTable[r].file].textStartingLine;
        }
    }

}








bool is_data(struct CombinedFiles combined, int line_number, int file_text_size, bool fill) {
    
    int offset;
    
    // label is used in the text section
    if (!fill) {
        offset = combined.text[line_number] & 0xFFFF;
    }
    
    // label is used in the data section
    else {
        offset = combined.data[line_number] & 0xFFFF;
    }
    
    // label is declared in data section
    if (offset >= file_text_size) {
        return true;
    }
    
    // label is declared in text section
    return false;
}

void print_text(struct CombinedFiles combined) {
    printf("PRINTING LINKED TEXT\n");
    printf("text size = %d\n", combined.textSize);
    
    for (int i = 0; i < combined.textSize; i++) {
        printf("%d\n", combined.text[i]);
    }
    
    printf("\n\n");
}

void print_data(struct CombinedFiles combined) {
    printf("PRINTING LINKED DATA\n");
    printf("data size = %d\n", combined.dataSize);
    
    for (int i = 0; i < combined.dataSize; i++) {
        printf("%d\n", combined.data[i]);
    }
    
    printf("\n\n");
}

void print_symbols(struct CombinedFiles combined) {
    printf("\n\n");
    printf("PRINTING ALL SYMBOLS FROM LINKED FILES\n");
    printf("numSymbols = %d\n", combined.symTableSize);
    
    for (int i = 0; i < combined.symTableSize; i++) {
        printf("%s ", combined.symTable[i].label);
        printf("%c ", combined.symTable[i].location);
        printf("%d\n", combined.symTable[i].offset);
    }
    printf("\n\n");
}

void print_relocation(struct CombinedFiles combined) {
    printf("PRINTING RELOCATION TABLE FROM LINKED FILES\n");
    printf("relocation table size = %d\n", combined.relocTableSize);
    
    for (int i = 0; i < combined.relocTableSize; i++) {
        printf("FILE: %d,     ", combined.relocTable[i].file);

        printf("%d ", combined.relocTable[i].offset);
        printf("%s ", combined.relocTable[i].inst);
        printf("%s\n", combined.relocTable[i].label);
    }
    printf("\n\n");
}

void print_starting_lines(struct FileData files[], int num_files) {
    printf("PRINTING STARTING LINES\n");
    
    for (int i = 0; i < num_files; i++) {
        printf("FILE %d, ", i);
        printf("TEXT STARTING LINE: %d\n", files[i].textStartingLine);
    }
    for (int i = 0; i < num_files; i++) {
        printf("FILE %d, ", i);
        printf("DATA STARTING LINE: %d\n", files[i].dataStartingLine);
    }
}

void final_output(struct CombinedFiles combined, FILE *outFilePtr) {
    for (int t = 0; t < combined.textSize; t++) {
        fprintf(outFilePtr, "%d\n", combined.text[t]);
    }
    
    for (int d = 0; d < combined.dataSize; d++) {
        fprintf(outFilePtr, "%d\n", combined.data[d]);
    }
}
