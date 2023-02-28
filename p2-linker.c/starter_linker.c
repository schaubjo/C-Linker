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

// for testing
void print_symbols(struct CombinedFiles combined);
void print_relocation(struct CombinedFiles combined);
void print_text(struct CombinedFiles combined);
void print_data(struct CombinedFiles combined);
void print_starting_lines(struct FileData files[], int num_files);

void parse_linked_files(CombinedFiles *, struct FileData files[], int num_files);

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
    print_text(linked_files);
    print_data(linked_files);
    print_symbols(linked_files);
    print_relocation(linked_files);
    print_starting_lines(files, num_files);
    return linked_files;
}

void parse_linked_files(CombinedFiles *combined, struct FileData files[], int num_files) {
    
    int line_number = 0;
    bool fill;
    int new_offset;
    // need to resolve everything in relocation table
    for (int r = 0; r < combined->relocTableSize; r++) {
        // if it is not a .fill, the line we need to resolve is in the text section
        if (strcmp(combined->relocTable[r].label, ".fill")) {
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
