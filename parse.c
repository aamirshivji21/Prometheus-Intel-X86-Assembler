#include "encode.h"
#include "test.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define INITIAL_CAPACITY 10
#define INITIAL_SIZE 256
#define SYMBOL_NAME_MAX_LENGTH 32
#define MAX_TOKEN_LENGTH 32
#define MAX_TOKENS 1000
#define INITIAL_SEGMENT_CAPACITY 1024
#define MAX_TOKENS_PER_LINE 10
#define MAX_CODE_SEGMENT_SIZE 1024

typedef struct {
    char instruction[MAX_TOKEN_LENGTH];
    char operand1[MAX_TOKEN_LENGTH];
    char operand2[MAX_TOKEN_LENGTH];
    int num_operands;
} ParsedInstruction;

typedef struct {
    char name[SYMBOL_NAME_MAX_LENGTH];
    size_t offset;
} SymbolTableEntry;

typedef struct {
    SymbolTableEntry *entries;
    size_t size;
    size_t capacity;
} SymbolTable;

typedef struct {
    char directive[MAX_TOKEN_LENGTH];
    char operands[MAX_TOKENS_PER_LINE - 1][MAX_TOKEN_LENGTH];
    int operandCount;
} Directive;

typedef struct {
    Directive *directives;
    size_t size;
    size_t capacity;
} DirectivesSegment;
// Token structure
typedef struct {
    char value[MAX_TOKEN_LENGTH];
} Token;


typedef struct {
    Token tokens[MAX_TOKENS_PER_LINE];
    int tokenCount;
} TokenArray;

typedef struct {
    unsigned int *data;
    size_t size;
    size_t capacity;
} CodeSegment;

struct segmentinfo {
    int32_t base;
    int32_t limit;
    bool granularity;
    bool in_memory;
};

// Global Variables
DirectivesSegment directivesSegment;
SymbolTable symbolTable;
CodeSegment codeSegment;
DataSegment dataSegment;

size_t currentOffset = 0;
size_t totalDataSize = 0;
size_t number_of_lines = 0;
size_t code_size = 0;
size_t stack_size = 0;
size_t header_size = sizeof(struct segmentinfo) * 4;
size_t base = 0;

struct segmentinfo headerSegment, dataSegmentInfo, codeSegmentInfo, stackSegment;

// Function Definitions
void initSymbolTable(SymbolTable *table) {
    table->entries = malloc(INITIAL_CAPACITY * sizeof(SymbolTableEntry));
    table->size = 0;
    table->capacity = INITIAL_CAPACITY;
}

void initDirectivesSegment(DirectivesSegment *segment) {
    segment->directives = malloc(INITIAL_CAPACITY * sizeof(Directive));
    segment->size = 0;
    segment->capacity = INITIAL_CAPACITY;
}

void initCodeSegment(CodeSegment *segment) {
    segment->data = malloc(INITIAL_CAPACITY * sizeof(unsigned int));
    segment->size = 0;
    segment->capacity = INITIAL_CAPACITY;
}

void initSegment(struct segmentinfo *segment, size_t base, size_t size) {
    segment->base = base;
    segment->limit = base + size - 1;
    segment->granularity = 0;
    segment->in_memory = 0;
}

void addSymbol(SymbolTable *table, const char *name, size_t offset) {
    if (table->size >= table->capacity) {
        table->capacity *= 2;
        table->entries = realloc(table->entries, table->capacity * sizeof(SymbolTableEntry));
    }
    strncpy(table->entries[table->size].name, name, SYMBOL_NAME_MAX_LENGTH);
    table->entries[table->size].offset = offset;
    table->size++;
}

void addDirective(DirectivesSegment *segment, const TokenArray *tokenArray) {
    if (segment->size >= segment->capacity) {
        segment->capacity *= 2;
        segment->directives = realloc(segment->directives, segment->capacity * sizeof(Directive));
    }

    Directive *newDirective = &segment->directives[segment->size];
    strncpy(newDirective->directive, tokenArray->tokens[0].value, MAX_TOKEN_LENGTH);
    newDirective->operandCount = tokenArray->tokenCount - 1;

    for (int i = 0; i < newDirective->operandCount; ++i) {
        strncpy(newDirective->operands[i], tokenArray->tokens[i + 1].value, MAX_TOKEN_LENGTH);
    }

    segment->size++;
}
void addCode(CodeSegment *segment, unsigned int machineCode) {
    if (segment->size >= segment->capacity) {
        segment->capacity *= 2;
        segment->data = realloc(segment->data, segment->capacity * sizeof(unsigned int));
    }
    segment->data[segment->size] = machineCode;
    segment->size++;
}

void lexer(char *line, TokenArray *tokenArray) {
    const char *delim = " ,\n\t";
    char *token;

    char *comment_start = strchr(line, ';');
    if (comment_start) {
        *comment_start = '\0';
    }
    tokenArray->tokenCount = 0;

    token = strtok(line, delim);

    while (token != NULL) {
        strncpy(tokenArray->tokens[tokenArray->tokenCount].value, token, MAX_TOKEN_LENGTH);
        tokenArray->tokenCount++;
        token = strtok(NULL, delim);
    }
}


char *instructionSet[] = {
    "mov", "call", "push", "pop", "cmp", "jz", "jmp", "lea", "xor", "inc", "test", "jnz", "ret", "shr", "shl", "add", "sub", "mul", "div", "int", "jb"
};
const int instructionSetSize = sizeof(instructionSet) / sizeof(instructionSet[0]);

int isInstruction(const char *token) {
    for (int i = 0; i < instructionSetSize; ++i) {
        if (strcmp(token, instructionSet[i]) == 0) {
            return 1;
        }
    }
    return 0;
}
void parseTokens(const TokenArray *tokenArray) {
    if (tokenArray->tokenCount == 0) return;

    const char *firstToken = tokenArray->tokens[0].value;

    if (tokenArray->tokens[0].value[0] == '.') {
        addDirective(&directivesSegment, tokenArray);
        if (strcmp(tokenArray->tokens[0].value, ".code") == 0 || strcmp(tokenArray->tokens[0].value, ".CODE") == 0) {
            currentOffset = 0;
        }
        if (strcmp(tokenArray->tokens[0].value, ".stack") == 0) {
            stack_size = (size_t)strtol(tokenArray->tokens[1].value, NULL, 16);
        }
    } else if (firstToken[strlen(firstToken) - 1] == ':') {
        char label[MAX_TOKEN_LENGTH];
        strncpy(label, firstToken, strlen(firstToken) - 1);
        label[strlen(firstToken) - 1] = '\0';
        addSymbol(&symbolTable, label, currentOffset);
    } else if (strcmp(tokenArray->tokens[1].value, "db") == 0) {
        addSymbol(&symbolTable, tokenArray->tokens[0].value, currentOffset);
        if (strncmp(tokenArray->tokens[3].value, "DUP(", 4) == 0) {
            int count = (int)strtol(tokenArray->tokens[2].value, NULL, 0);
            size_t len = strlen(tokenArray->tokens[3].value);
            if (tokenArray->tokens[3].value[len - 1] == ')') {
                unsigned short defaultValue = 0;
                for (int i = 0; i < count; ++i) {
                    currentOffset += 1;
                    totalDataSize += 1;
                }
            } else {
                printf("Syntax error: missing closing parenthesis in DUP directive\n");
            }
        } else {
            for (int i = 2; i < tokenArray->tokenCount; ++i) {
                unsigned char value = (unsigned char)strtol(tokenArray->tokens[i].value, NULL, 0);
                currentOffset += 1;
                totalDataSize += 1;
            }
        }
    } else if (strcmp(tokenArray->tokens[1].value, "dd") == 0) {
        addSymbol(&symbolTable, tokenArray->tokens[0].value, currentOffset);

        if (strncmp(tokenArray->tokens[3].value, "DUP(", 4) == 0) {
            int count = (int)strtol(tokenArray->tokens[2].value, NULL, 0);
            size_t len = strlen(tokenArray->tokens[3].value);
            if (tokenArray->tokens[3].value[len - 1] == ')') {
                unsigned short defaultValue = 0;
                for (int i = 0; i < count; ++i) {
                    currentOffset += 4;
                    totalDataSize += 4;
                }
            } else {
                printf("Syntax error: missing closing parenthesis in DUP directive\n");
            }
         } else {
            for (int i = 2; i < tokenArray->tokenCount; ++i) {
                unsigned int value = (unsigned int)strtol(tokenArray->tokens[i].value, NULL, 0);
                currentOffset += 4;
                totalDataSize += 4;
            }
        }
    } else if (strcmp(tokenArray->tokens[1].value, "dw") == 0) {
        addSymbol(&symbolTable, tokenArray->tokens[0].value, currentOffset);
        if (strncmp(tokenArray->tokens[3].value, "DUP(", 4) == 0) {
            int count = (int)strtol(tokenArray->tokens[2].value, NULL, 0);
            size_t len = strlen(tokenArray->tokens[3].value);
            if (tokenArray->tokens[3].value[len - 1] == ')') {
                unsigned short defaultValue = 0;
                for (int i = 0; i < count; ++i) {
                    currentOffset += 2;
                    totalDataSize += 2;
                }
            } else {
                printf("Syntax error: missing closing parenthesis in DUP directive\n");
            }
        } else {
            for (int i = 2; i < tokenArray->tokenCount; ++i) {
                unsigned short value = (unsigned short)strtol(tokenArray->tokens[i].value, NULL, 0);
                currentOffset += 2;
                totalDataSize += 2;
            }
        }
    } else if (isInstruction(firstToken)) {
        number_of_lines++;
        code_size = 4 * number_of_lines;

        char instr[MAX_TOKEN_LENGTH];
        char dest[MAX_TOKEN_LENGTH];
        char src[MAX_TOKEN_LENGTH] = "";

        strncpy(instr, tokenArray->tokens[0].value, MAX_TOKEN_LENGTH);
        strncpy(dest, tokenArray->tokens[1].value, MAX_TOKEN_LENGTH);

        if (tokenArray->tokenCount > 2) {
            strncpy(src, tokenArray->tokens[2].value, MAX_TOKEN_LENGTH);
        }

        unsigned int machineCode = encodeInstruction(instr, dest, src);
        addCode(&codeSegment, machineCode);
        char binaryOutput[33];
        intToBinaryString(machineCode, 32, binaryOutput);
    } else if (strcmp(firstToken, "END") == 0) {
        addDirective(&directivesSegment, tokenArray);
    } else {
        printf("Error: Unknown token %s\n", firstToken);
    }
}

void parseFile(const char *filename) {
    FILE *inputFile = fopen(filename, "r");
    if (inputFile == NULL) {
        printf("Error: Cannot open file %s\n", filename);
        return;
    }

    char line[MAX_TOKEN_LENGTH * MAX_TOKENS_PER_LINE];
    int lineNumber = 0;
    TokenArray tokenArray;

    while (fgets(line, sizeof(line), inputFile) != NULL) {
        lineNumber++;
        lexer(line, &tokenArray);
        parseTokens(&tokenArray);
    }

    fclose(inputFile);

    printf("Assembly completed successfully.\n");
}
void writeSegmentToFile(FILE *outputFile, const unsigned char *segmentData, size_t segmentSize) {
    fwrite(&segmentSize, sizeof(size_t), 1, outputFile);
    fwrite(segmentData, sizeof(unsigned char), segmentSize, outputFile);
}

void writeSymbolTableToFile(const SymbolTable *table, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error: Cannot open file %s for writing\n", filename);
        return;
    }

    fprintf(file, "Symbol Table:\n");
    for (size_t i = 0; i < table->size; ++i) {
        fprintf(file, "Symbol: %s, Offset: %zu\n", table->entries[i].name, table->entries[i].offset);
    }

    fclose(file);
    printf("Symbol table written to %s\n", filename);
}

void initializeSegmentDescriptors() {
    initSegment(&headerSegment, base, header_size);
    base += header_size;

    initSegment(&dataSegmentInfo, base, totalDataSize);
    base += totalDataSize;

    initSegment(&codeSegmentInfo, base, code_size);
    base += code_size;

    initSegment(&stackSegment, base, stack_size);
}

void writeSegmentDescriptors(FILE *outputFile) {
    fwrite(&headerSegment, sizeof(struct segmentinfo), 1, outputFile);
    fwrite(&dataSegmentInfo, sizeof(struct segmentinfo), 1, outputFile);
    fwrite(&codeSegmentInfo, sizeof(struct segmentinfo), 1, outputFile);
    fwrite(&stackSegment, sizeof(struct segmentinfo), 1, outputFile);
}
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s input.asm output.o\n", argv[0]);
        return 1;
    }

    initSymbolTable(&symbolTable);
    initDataSegment(&dataSegment);
    initCodeSegment(&codeSegment);

    FILE *inputFile = fopen(argv[1], "r");
    if (inputFile == NULL) {
        printf("Error: Could not open input file %s\n", argv[1]);
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), inputFile)) {
        secondPass(line, &dataSegment);
        TokenArray tokenArray;
        lexer(line, &tokenArray);
        parseTokens(&tokenArray);
    }

    fclose(inputFile);
    printf("Total Data Segment Size: %zu bytes\n", totalDataSize);
    printf("Number of instruction lines: %zu\n", number_of_lines);
    printf("Code segment size: %zu bytes\n", code_size);
    printf("Stack segment size: %zu bytes\n", stack_size);
    writeSymbolTableToFile(&symbolTable, "symbol_table.txt");

    FILE *outputFile = fopen(argv[2], "wb");
    if (outputFile == NULL) {
        printf("Error: Could not open output file %s\n", argv[2]);
        return 1;
    }

    initializeSegmentDescriptors();
    writeSegmentDescriptors(outputFile);

    writeSegmentToFile(outputFile, dataSegment.data, dataSegment.size);
    writeSegmentToFile(outputFile, (unsigned char *)codeSegment.data, codeSegment.size * sizeof(unsigned int));

    fclose(outputFile);
    free(dataSegment.data);
    free(codeSegment.data);

    return 0;
}
