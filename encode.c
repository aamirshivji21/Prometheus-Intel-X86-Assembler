#include "encode.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Registers
Register registers[] = {
    {"al", 0b00000000, 8}, {"cl", 0b00000001, 8}, {"dl", 0b00000010, 8}, {"bl", 0b00000011, 8},
    {"ah", 0b00000100, 8}, {"ch", 0b00000101, 8}, {"dh", 0b00000110, 8}, {"bh", 0b00000111, 8},
    {"ax", 0b00000000, 16}, {"cx", 0b00000001, 16}, {"dx", 0b00000010, 16}, {"bx", 0b00000011, 16},
    {"sp", 0b00000100, 16}, {"bp", 0b00000101, 16}, {"si", 0b00000110, 16}, {"di", 0b00000111, 16},
    {"eax", 0b00000000, 32}, {"ecx", 0b00000001, 32}, {"edx", 0b00000010, 32}, {"ebx", 0b00000011, 32},
    {"esp", 0b00000100, 32}, {"ebp", 0b00000101, 32}, {"esi", 0b00000110, 32}, {"edi", 0b00000111, 32}
};

Instruction pushOpcode[] = {
        {"es",0b0000001000000000}, {"ss", 0b0000001000010000}, {"cs", 0b0000001000100000}, {"ds", 0b0000001000110000},
        {"fs", 0b0000001001000000}, {"gs", 0b0000001001010000}, {"eax", 0b0000001001100000}, {"ax", 0b0000001001100001},
        {"ecx", 0b0000001001100010}, {"cx", 0b0000001001100011}, {"edx", 0b0000001001100100}, {"dx", 0b0000001001100101},
        {"ebx", 0b0000001001100110}, {"bx", 0b0000001001100111}, {"esp", 0b0000001001101000}, {"sp", 0b0000001001101001},
        {"ebp", 0b0000001001101010}, {"bp", 0b0000001001101011}, {"esi", 0b0000001001101100}, {"si", 0b0000001001101101},
        {"edi", 0b0000001001101110}, {"di", 0b0000001001101111}
};

Instruction popOpcode[] = {
        {"es", 0b0000001100000000}, {"ss", 0b0000001100010000}, {"ds", 0b0000001100100000}, {"fs", 0b0000001100110000},
        {"gs", 0b0000001101000000}, {"eax",0b0000001101010000}, {"ax", 0b0000001101010001}, {"ecx",0b0000001101010010},
        {"cx",0b0000001101010011}, {"edx",0b0000001101010100}, {"dx", 0b0000001101010101}, {"ebx", 0b0000001101010110},
        {"bx", 0b0000001101010111}, {"esp", 0b0000001101011000}, {"sp", 0b0000001101011001}, {"ebp", 0b0000001101011010},
        {"bp", 0b0000001101011011}, {"esi", 0b0000001101011100}, {"si", 0b0000001101011101}, {"edi", 0b0000001101011110}, {"di", 0b0000001101011111}
};

Instruction incOpcode[] = {
        {"eax", 0b0000011100000000}, {"ax", 0b0000011100000001}, {"ecx", 0b0000011100000010}, {"cx", 0b0000011100000011},
        {"edx", 0b0000011100000100}, {"dx", 0b0000011100000101}, {"ebx", 0b0000011100000110}, {"bx", 0b0000011100000111},
        {"esp", 0b0000011100001000}, {"sp", 0b0000011100001001}, {"ebp", 0b0000011100001010}, {"bp", 0b0000011100001011},
        {"esi", 0b0000011100001100}, {"si", 0b0000011100001101}, {"edi", 0b0000011100001110}, {"di", 0b0000011100001111}
};//there are three more registers for byte, word and double word which i think take care of memory addressing????

Instruction decOpcode[] = {
        {"eax", 0b0000100000000000}, {"ax", 0b0000100000000001}, {"ecx", 0b0000100000000010}, {"cx", 0b0000100000000011},
        {"edx", 0b0000100000000100}, {"dx", 0b0000100000000101}, {"ebx", 0b0000100000000110}, {"bx", 0b0000100000000111},
        {"esp", 0b0000100000001000}, {"sp", 0b0000100000001001}, {"ebp", 0b0000100000001010}, {"bp", 0b0000100000001011},
        {"esi", 0b0000100000001100}, {"si", 0b0000100000001101}, {"edi", 0b0000100000001110}, {"di", 0b0000100000001111}
};//same issue

// Segment Registers
Register segment_registers[] = {
    {"es", 0b00000000}, {"cs", 0b00000001}, {"ss", 0b00000010}, {"ds", 0b00000011}, {"fs", 0b00000100}, {"gs", 0b00000101}
};

const char *instruction_names[] = {"mov","add","push","pop","inc","dec","sub","mul","div","and","or","xor","int","jmp"};


int get_register_code(const char* reg) {
    for (int i = 0; i < sizeof(registers) / sizeof(Register); i++) {
        if (strcmp(registers[i].name, reg) == 0) {
            return registers[i].code;
        }
    }
    return -1;
}

// Function to get the register size
int get_register_size(const char* reg) {
    for (int i = 0; i < sizeof(registers) / sizeof(Register); i++) {
        if (strcmp(registers[i].name, reg) == 0) {
            return registers[i].size;
        }
    }
    return -1;
}


// Function to get the segment register code
int get_segment_register_code(const char* reg) {
    for (int i = 0; i < sizeof(segment_registers) / sizeof(Register); i++) {
        if (strcmp(segment_registers[i].name, reg) == 0) {
            return segment_registers[i].code;
        }
    }
    return -1;
}


// Convert an integer to a binary string of specified length
void intToBinaryString(int value, int length, char *output) {
    for (int i = length - 1; i >= 0; i--) {
        output[length - 1 - i] = (value & (1 << i)) ? '1' : '0';
    }
    output[length] = '\0';
}
bool is_simple_register(const char* operand) {
    const char* general_purpose_registers[] = {
        "al", "cl", "dl", "bl",
        "ah", "ch", "dh", "bh",
        "ax", "cx", "dx", "bx",
        "sp", "bp", "si", "di",
        "eax", "ecx", "edx", "ebx",
        "esp", "ebp", "esi", "edi"
    };

    const char* segment_registers[] = {
        "es", "cs", "ss", "ds", "fs", "gs"
    };

    int general_purpose_size = sizeof(general_purpose_registers) / sizeof(general_purpose_registers[0]);
    int segment_size = sizeof(segment_registers) / sizeof(segment_registers[0]);

    for (int i = 0; i < general_purpose_size; i++) {
        if (strcmp(operand, general_purpose_registers[i]) == 0) {
            return true;
        }
    }

    for (int i = 0; i < segment_size; i++) {
        if (strcmp(operand, segment_registers[i]) == 0) {
            return true;
        }
    }

    return false;
}
bool is_complex_operand(const char* operand) {
    // Check for segment override (e.g., es:)
    const char* colon = strchr(operand, ':');
    if (colon) {
        // Ensure that the part before the colon is a valid segment register
        char segment[10];
        strncpy(segment, operand, colon - operand);
        segment[colon - operand] = '\0';

        if (!is_simple_register(segment)) {
            return false;
        }

        // Ensure that the part after the colon is a memory operand
        const char* mem = colon + 1;
        if (mem[0] == '[' && mem[strlen(mem) - 1] == ']') {
            return true;
        }
    }

    // Check for simple memory operand without segment override (e.g., [bx+6H])
    if (operand[0] == '[' && operand[strlen(operand) - 1] == ']') {
        return true;
    }

    return false;
}

int extract_base_register(const char* mem) {
    char clean_reg[10];
    char* start = strchr(mem, '[') + 1;
    char* end = strchr(mem, '+');
    if (end == NULL) {
        end = strchr(mem, ']');
    }
    strncpy(clean_reg, start, end - start);
    clean_reg[end - start] = '\0';
    return get_register_code(clean_reg);
}
int extract_register_size(const char* mem) {
    char clean_reg[10];
    char* start = strchr(mem, '[') + 1;
    char* end = strchr(mem, '+');
    if (end == NULL) {
        end = strchr(mem, ']');
    }
    strncpy(clean_reg, start, end - start);
    clean_reg[end - start] = '\0';
    return get_register_size(clean_reg);
}
unsigned int encodeInstruction(const char *instruction, const char *operand1, const char *operand2) {
        int reg1_code, reg2_code, reg1_size, reg2_size;
        unsigned short opcode;
        unsigned int machineCode;
        for (int i = 0; i < (sizeof(instruction_names)/sizeof(instruction_names[0])); i++){
                if (strcmp(instruction, instruction_names[i]) == 0) {
                        switch (i) {
                                case 0:
                                        if (is_simple_register(operand1) && is_simple_register(operand2)){
                                                reg1_code = get_register_code(operand1);
                                                reg2_code = get_register_code(operand2);
                                                reg1_size = get_register_size(operand1);
                                                reg2_size = get_register_size(operand2);

                                                opcode = 0;
                                                if (reg2_code == -1 && reg2_size == -1){
                                                        reg2_code = get_segment_register_code(operand2);
                                                        opcode = 0b0000010001000000;
                                                }
                                                else if (reg1_code == -1 && reg1_size == -1){
                                                        reg1_code = get_segment_register_code(operand1);
                                                        opcode = 0b0000010001010000;
                                                }
                                                else{
                                                        switch (reg1_size) {
                                                                case 8:
                                                                        opcode = 0b0000010000000000;
                                                                        break;
                                                                case 16:
                                                                        opcode = 0b0000010000010000;
                                                                        break;
                                                                case 32:
                                                                        opcode = 0b0000010000011000;
                                                                        break;
                                                        }
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        } else if (is_simple_register(operand1) && is_complex_operand(operand2)){
                                                reg1_code = get_register_code(operand1);
                                                reg2_code = extract_base_register(operand2);
                                                reg1_size = get_register_size(operand1);
                                                reg2_size = extract_register_size(operand2);

                                                opcode = 0;
                                                switch (reg1_size) {
                                                        case 8:
                                                                opcode = 0b0000010000000000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000010000010000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000010000011000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        } else if (is_complex_operand(operand1) && is_simple_register(operand2)){
                                                reg1_code = extract_base_register(operand1);
                                                reg2_code = get_register_code(operand2);
                                                reg1_size = extract_register_size(operand1);
                                                reg2_code = get_register_size(operand2);

                                                opcode = 0;
                                                switch (reg1_size) {
                                                        case 8:
                                                                opcode = 0b0000010000100000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000010000110000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000010000111000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        break;
                                case 1:
                                        if (is_simple_register(operand1) && is_simple_register(operand2)) {
                                                reg1_code = get_register_code(operand1);
                                                reg2_code = get_register_code(operand2);
                                                reg1_size = get_register_size(operand1);
                                                reg2_size = get_register_size(operand2);

                                                opcode = 0;
                                                switch (reg1_size) {
                                                        case 8:
                                                                opcode = 0b0000000100000000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000000100010000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000000100011000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        else if (is_simple_register(operand1) && is_complex_operand(operand2)){
                                                reg1_code = get_register_code(operand1);
                                                reg2_code = extract_base_register(operand2);
                                                reg1_size = get_register_size(operand1);
                                                reg2_size = extract_register_size(operand2);

                                                opcode = 0;
                                                switch (reg1_size) {
                                                        case 8:
                                                                opcode = 0b0000000100000000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000000100010000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000000100011000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        else if (is_complex_operand(operand1) && is_simple_register(operand2)){
                                                reg1_code = extract_base_register(operand1);
                                                reg2_code = get_register_code(operand2);
                                                reg1_size = extract_register_size(operand1);
                                                reg2_size = get_register_size(operand2);

                                                opcode = 0;
                                                switch (reg1_size) {
                                                        case 8:
                                                                opcode = 0b0000000100100000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000000100110000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000000100111000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        break;
                                case 2:
                                        reg1_code = 0b00000000;
                                        reg2_code = get_register_code(operand1);
                                        opcode = 0;
                                        for (int i = 0; i < sizeof(pushOpcode) / sizeof(pushOpcode[0]); i++) {
                                                if (strcmp(operand1, pushOpcode[i].name) == 0) {
                                                        opcode = pushOpcode[i].opcode;
                                                }
                                        /*      else{
                                                        printf("Error: Unsupported register %s for PUSH instruction\n", operand1);
                                                }*/
                                        }
                                        machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                        return machineCode;
                                        break;
                                case 3:
                                        reg1_code = 0b00000000;
                                        reg2_code = get_register_code(operand1);
                                        opcode = 0;
                                        for (int i = 0; i < sizeof(popOpcode) / sizeof(popOpcode[0]); i++) {
                                                if (strcmp(operand1, popOpcode[i].name) == 0) {
                                                        opcode = popOpcode[i].opcode;
                                                }
                                                else{
                                                        printf("Error: Unsupported register %s for POP instruction\n", operand1);
                                                }
                                        }
                                        machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                        return machineCode;
                                        break;
                                case 4:
                                        reg1_code = 0b00000000;
                                        reg2_code = get_register_code(operand1);
                                        opcode = 0;
                                        for (int i = 0; i < sizeof(incOpcode) / sizeof(incOpcode[0]); i++) {
                                                if (strcmp(operand1, incOpcode[i].name) == 0) {
                                                        opcode = incOpcode[i].opcode;
                                                }
                                                else{
                                                        printf("Error: Unsupported register %s for INC instruction\n", operand1);
                                                }
                                        }
                                        machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                        return machineCode;
                                        break;
                                case 5:
                                        reg1_code = 0b00000000;
                                        reg2_code = get_register_code(operand1);
                                        opcode = 0;
                                        for (int i = 0; i < sizeof(decOpcode) / sizeof(decOpcode[0]); i++) {
                                                if (strcmp(operand1, decOpcode[i].name) == 0) {
                                                        opcode = decOpcode[i].opcode;
                                                }
                                                else{
                                                        printf("Error: Unsupported register %s for INC instruction\n", operand1);
                                                }
                                        }
                                        machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                        return machineCode;
                                        break;
                                case 6:
                                        if (is_simple_register(operand1) && is_simple_register(operand2)){
                                                reg1_code = get_register_code(operand1);
                                                reg2_code = get_register_code(operand2);
                                                reg1_size = get_register_size(operand1);
                                                reg2_size = get_register_size(operand2);

                                                opcode = 0;
                                                switch(reg1_size) {
                                                        case 8:
                                                                opcode = 0b0000011000000000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000011000010000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000011000011000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        else if(is_simple_register(operand1) && is_complex_operand(operand2)){
                                                reg1_code = get_register_code(operand1);
                                                reg2_code = extract_base_register(operand2);
                                                reg1_size = get_register_size(operand1);
                                                reg2_size = extract_register_size(operand2);

                                                opcode = 0;
                                                switch (reg1_size){
                                                        case 8:
                                                                opcode = 0b0000011000000000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000011000010000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000011000011000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;                                     }
                                        else if(is_complex_operand(operand1) && is_simple_register(operand2)){
                                                reg1_code = extract_base_register(operand1);
                                                reg2_code = get_register_code(operand2);
                                                reg1_size = extract_register_size(operand1);
                                                reg2_size = get_register_size(operand2);

                                                opcode = 0;
                                                switch(reg2_size){
                                                        case 8:
                                                                opcode = 0b0000011000100000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000011000110000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000011000111000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        break;
                                case 7:
                                        reg1_code = 0b00000000;
                                        if (is_simple_register(operand1)){
                                                reg2_code = get_register_code(operand1);
                                                reg2_size = get_register_size(operand1);

                                                opcode = 0;
                                                switch (reg2_size){
                                                        case 8:
                                                                opcode = 0b0000101000000000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000101000010000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000101000011000;
                                                                break;
                                                }
                                        machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                        return machineCode;
                                        }
                                        //need to handle memory addressing using ptrs
                                        break;
                                case 8:
                                        reg1_code = 0b00000000;
                                        if (is_simple_register(operand1)){
                                                reg2_code = get_register_code(operand1);
                                                reg2_size = get_register_size(operand1);

                                                opcode = 0;
                                                switch (reg2_size){
                                                        case 8:
                                                                opcode = 0b0000110000000000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000110000010000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000110000011000;
                                                                break;
                                                }
                                        machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                        return machineCode;
                                        }
                                        //memory adressing handled similarly to mul
                                        break;
                                case 9:
                                        if (is_simple_register(operand1) && is_simple_register(operand2)) {
                                                reg1_code = get_register_code(operand1);
                                                reg2_code = get_register_code(operand2);
                                                reg1_size = get_register_size(operand1);
                                                reg2_size = get_register_size(operand2);

                                                opcode = 0;
                                                switch (reg1_size) {
                                                        case 8:
                                                                opcode = 0b0000110100000000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000110100010000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000110100011000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        else if(is_simple_register(operand1) && is_complex_operand(operand2)){
                                                reg1_code = get_register_code(operand1);
                                                reg2_code = extract_base_register(operand2);
                                                reg1_size = get_register_size(operand1);
                                                reg2_size = extract_register_size(operand2);

                                                opcode = 0;
                                                switch (reg1_size) {
                                                        case 8:
                                                                opcode = 0b0000110100000000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000110100010000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000110100011000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        else if(is_complex_operand(operand1) && is_simple_register(operand2)){
                                                reg1_code = extract_base_register(operand1);
                                                reg2_code = get_register_code(operand2);
                                                reg1_size = extract_register_size(operand1);
                                                reg2_size = get_register_size(operand2);

                                                opcode = 0;
                                                switch (reg2_size) {
                                                        case 8:
                                                                opcode = 0b0000110100100000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000110100110000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000110100111000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        break;
                                case 10:
                                        if (is_simple_register(operand1) && is_simple_register(operand2)){
                                                reg1_code = get_register_code(operand1);
                                                reg2_code = get_register_code(operand2);
                                                reg1_size = get_register_size(operand1);
                                                reg2_size = get_register_size(operand2);

                                                opcode = 0;
                                                switch (reg1_size) {
                                                        case 8:
                                                                opcode = 0b0000111000000000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000111000010000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000111000011000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        else if (is_simple_register(operand1) && is_complex_operand(operand2)){
                                                reg1_code = get_register_code(operand1);
                                                reg2_code = extract_base_register(operand2);
                                                reg1_size = get_register_size(operand1);
                                                reg2_size = extract_register_size(operand2);

                                                opcode = 0;
                                                switch (reg1_size) {
                                                        case 8:
                                                                opcode = 0b0000111000000000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000111000010000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000111000011000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        else if (is_complex_operand(operand1) && is_simple_register(operand2)){
                                                reg1_code = extract_base_register(operand1);
                                                reg2_code = get_register_code(operand2);
                                                reg1_size = extract_register_size(operand1);
                                                reg2_size = get_register_size(operand2);

                                                opcode = 0;
                                                switch (reg2_size) {
                                                        case 8:
                                                                opcode = 0b0000111000100000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000111000110000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000111000111000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        break;
                                case 11:
                                        if (is_simple_register(operand1) && is_simple_register(operand2)){
                                                reg1_code = get_register_code(operand1);
                                                reg2_code = get_register_code(operand2);
                                                reg1_size = get_register_size(operand1);
                                                reg2_size = get_register_size(operand2);

                                                opcode = 0;
                                                switch (reg1_size){
                                                        case 8:
                                                                opcode = 0b0000111100000000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000111100010000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000111100011000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        else if (is_simple_register(operand1) && is_complex_operand(operand2)){
                                                reg2_code = extract_base_register(operand2);
                                                reg1_code = get_register_code(operand1);
                                                reg2_size = extract_register_size(operand2);
                                                reg1_size = get_register_size(operand1);

                                                opcode = 0;
                                                switch (reg1_size) {
                                                        case 8:
                                                                opcode = 0b0000111100000000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000111100010000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000111100011000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        else if (is_complex_operand(operand1) && is_simple_register(operand2)){
                                                reg1_code = extract_base_register(operand1);
                                                reg2_code = get_register_code(operand2);
                                                reg1_size = get_register_size(operand1);
                                                reg2_size = extract_register_size(operand2);

                                                opcode = 0;
                                                switch (reg2_size){
                                                        case 8:
                                                                opcode = 0b0000111100100000;
                                                                break;
                                                        case 16:
                                                                opcode = 0b0000111100110000;
                                                                break;
                                                        case 32:
                                                                opcode = 0b0000111100111000;
                                                                break;
                                                }
                                                machineCode = (opcode << 16) | (reg1_code << 8) | reg2_code;
                                                return machineCode;
                                        }
                                        break;
                                default:
                                        printf("Error: Unsupported instruction %s\n", instruction);
                                        return 0;
                                }
                        }
                }
                printf("Error: Instruction not found %s\n", instruction);
                return 0;
}
