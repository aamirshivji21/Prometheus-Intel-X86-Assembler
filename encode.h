#ifndef ENCODE_H
#define ENCODE_H
#include <stddef.h>
typedef struct {
    const char *name;
    unsigned char code;
    int size;
} Register;

typedef struct {
    const char *name;
    unsigned short opcode;
} Instruction;


unsigned int encodeInstruction(const char *instruction, const char *operand1, const char *operand2);
void intToBinaryString(int value, int length, char *output);



#endif // ENCODE_H
