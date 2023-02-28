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

int resolve_local(int curr_file, int num_files, struct FileData files[]);
//int resolve_global();

CombinedFiles combined_init(struct FileData files[], const int num_files);
void print_symbols(struct CombinedFiles combined);
void print_relocation(struct CombinedFiles combined);


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
    
//    for (int f = 0; f < num_files; f++) {
//
//        // iterate through each line in text section
//        for (int t = 0; t < files[f].textSize; t++) {
//            bool resolve = false;
//
//            // check the relocation table to see if it is a line that needs to be resolved
//            for (int r = 0; r < files[f].relocationTableSize; r++) {
//                if (files[f].relocTable[r].offset == t) {
//                    resolve = true;
//                    break;
//                }
//            }
//
//            // if this line was in the relocation table, we need to resolve the offset
//            if (resolve) {
//
//                // local labels
//                if (!isupper(files[f].relocTable[t].label[0])) {
//                    int new_offset = resolve_local(f, num_files, files);
//                    // output original text + new offset
//                    fprintf(outFilePtr, "%d\n", files[f].text[t] + new_offset);
//                }
//
//                // global labels
//                else {
//
//                }
//            }
//
//            else {
//                fprintf(outFilePtr, "%d\n", files[f].text[t]);
//            }
//        }
//    }

} // main

int resolve_local(int curr_file, int num_files, struct FileData files[]) {
    
    // resolve local labels that have to point to data section within its own file
    
    // add up the number of text lines before the current text file
    int before_text = 0;
    for (int b = 0; b < curr_file; b++) {
        before_text += files[b].textSize;
    }
    
    // add up the number of text lines after the current text file
    int after_text = 0;
    for (int a = curr_file + 1; a < num_files; a++) {
        after_text += files[a].textSize;
    }
    
    // add the number of data lines before the current file's data section
    int before_data = 0;
    for (int d = 0; d < curr_file; d++) {
        before_data += files[d].dataSize;
    }
    return before_text + after_text + before_data;
}

CombinedFiles combined_init(struct FileData files[], const int num_files) {
    
    struct CombinedFiles linked_files;
    
    // used as a counter for total text and data lines in all files
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
    
    // iterate through each file's data, and add it to the combined data
    for (int f = 0; f < num_files; f++) {
        // also initialize the start of each file's data section for logic later
        files[f].dataStartingLine = i;
        for (int d = 0; d < files[f].dataSize; d++) {
            linked_files.data[i] = files[f].data[d];
            i++;
        }
    }
    
    linked_files.dataSize = i - linked_files.textSize;
    
    // let i be used to count the number of symbols
    i = 0;
    
    // iterate through each file's symbol table, and add it to the combined data
    // and while updating the offsets
    for (int f = 0; f < num_files; f++) {
        for (int s = 0; s < files[f].symbolTableSize; s++) {
            linked_files.symTable[i] = files[f].symbolTable[s];
            i++;
        }
    }
    
    linked_files.symTableSize = i;
    
    // let i ibe used to count the number of relocations
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
    print_symbols(linked_files);
    print_relocation(linked_files);
    return linked_files;
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

