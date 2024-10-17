#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define INITIAL_CAPACITY 10
#define MAX_TOKEN_LENGTH 32

// DataSegment struct definition
typedef struct {
    unsigned char *data;
    size_t size;
    size_t capacity;
} DataSegment;

// Function prototypes
void initDataSegment(DataSegment *segment);
void addToDataSegment(DataSegment *segment, const char *binary);
const char* getBinaryFromHex(char hex);
void convertHexToBinary(const char *hex, char *binary);
void convertIntToBinary(int value, char *binary);
int isDataDeclaration(const char *type);
void writeBinaryToFile(FILE *outputFile, const char *binary);
void secondPass(const char *line, DataSegment *dataSegment);

#endif // TEST_H
