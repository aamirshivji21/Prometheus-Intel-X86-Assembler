#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "test.h"

#define INITIAL_CAPACITY 10
#define MAX_TOKEN_LENGTH 32



//data segment to hold binary format of data declarations
void initDataSegment(DataSegment *segment) {
    segment->data = malloc(INITIAL_CAPACITY * sizeof(unsigned char));
    segment->size = 0;
    segment->capacity = INITIAL_CAPACITY;
}

void addToDataSegment(DataSegment *segment, const char *binary) {
    if (segment->size + 4 > segment->capacity) {
        segment->capacity *= 2;
        segment->data = realloc(segment->data, segment->capacity * sizeof(unsigned char));
    }

    unsigned int value = strtoul(binary, NULL, 2);
    memcpy(&segment->data[segment->size], &value, 4);
    segment->size += 4;
}

// Hex to Binary mapping
const char *hexToBinary[16] = {
    "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111",
    "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"
};

// Function to get binary string from hex character
const char* getBinaryFromHex(char hex) {
    if (hex >= '0' && hex <= '9') {
        return hexToBinary[hex - '0'];
    } else if (hex >= 'A' && hex <= 'F') {
        return hexToBinary[hex - 'A' + 10];
    } else if (hex >= 'a' && hex <= 'f') {
        return hexToBinary[hex - 'a' + 10];
    } else {
        return NULL; // Invalid hex character
    }
}
// Function to convert hex string to binary string and pad to 32 bits
void convertHexToBinary(const char *hex, char *binary) {
    int len = strlen(hex); // Length of hex string
    binary[0] = '\0'; // Initialize the binary string

    for (int i = 0; i < len; i++) {
        const char *bin = getBinaryFromHex(hex[i]);
        if (bin) {
            strcat(binary, bin);
        } else {
            printf("Error: Invalid hex character %c\n", hex[i]);
            return;
        }
    }

    // Calculate padding length
    int binaryLen = strlen(binary);
    int paddingLen = 32 - binaryLen;

    // Create a temporary buffer for the padded binary string
    char paddedBinary[33];
    paddedBinary[0] = '\0';

    // Add leading zeros
    for (int i = 0; i < paddingLen; i++) {
        strcat(paddedBinary, "0");
    }

    // Append the original binary string
    strcat(paddedBinary, binary);

    // Copy the padded binary string back to the output
    strcpy(binary, paddedBinary);
}

// Function to convert decimal integer to binary string and pad to 32 bits
void convertIntToBinary(int value, char *binary) {
    binary[0] = '\0'; // Initialize the binary string

    for (int i = 31; i >= 0; i--) {
        strcat(binary, (value & (1 << i)) ? "1" : "0");
    }
}
// Function to check if a line is a data declaration
int isDataDeclaration(const char *type) {
    return strcmp(type, "db") == 0 || strcmp(type, "dw") == 0 || strcmp(type, "dd") == 0;
}

// Function to write binary data to a .o file
void writeBinaryToFile(FILE *outputFile, const char *binary) {
    // Convert the 32-bit binary string to an unsigned int
    unsigned int value = strtoul(binary, NULL, 2);

    // Write the 32-bit value to the output file in binary format
    fwrite(&value, sizeof(unsigned int), 1, outputFile);
}

// Function to perform the second pass
void secondPass(const char *line, DataSegment *dataSegment) {
    // Skip empty lines or lines starting with a comment
    if (line[0] == '\0' || line[0] == ';') {
        return;
    }

    char varName[MAX_TOKEN_LENGTH], type[MAX_TOKEN_LENGTH], values[MAX_TOKEN_LENGTH * 10];
    sscanf(line, "%s %s %[^\n]", varName, type, values); // Read the whole line and split into tokens

    if (!isDataDeclaration(type)) {
        return; // Skip non-data declaration lines
    }

    if (strstr(values, "DUP") != NULL) {
        // Handle DUP directive
        int count;
        char dupValue[MAX_TOKEN_LENGTH];

        // Parse the DUP directive
        sscanf(values, "%d DUP(%[^)])", &count, dupValue);

        char binary[33]; // 32 bits + null terminator
        if (dupValue[strlen(dupValue) - 1] == 'h') {
            // Handle hexadecimal value
            dupValue[strlen(dupValue) - 1] = '\0'; // Remove the trailing 'h'
            convertHexToBinary(dupValue, binary);
        } else if (strcmp(dupValue, "?") == 0) {
            // Handle uninitialized data (represented as all zeros)
            memset(binary, '0', 32);
            binary[32] = '\0';
        } else {
            // Handle decimal value
            int intValue = atoi(dupValue);
            convertIntToBinary(intValue, binary);
        }

        // Write the binary data for each duplicated value to the .o file
        for (int i = 0; i < count; i++) {
            addToDataSegment(dataSegment, binary);
        }
    } else {
        // Handle regular values
        char *token = strtok(values, " ");
        while (token) {
            char binary[33]; // 32 bits + null terminator
            if (token[strlen(token) - 1] == 'h') {
                // Handle hexadecimal value
                token[strlen(token) - 1] = '\0'; // Remove the trailing 'h'
                convertHexToBinary(token, binary);
            } else {
                // Handle decimal value
                int intValue = atoi(token);
                convertIntToBinary(intValue, binary);
            }

            addToDataSegment(dataSegment, binary);

            token = strtok(NULL, " ");
        }
    }
}
