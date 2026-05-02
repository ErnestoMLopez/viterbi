#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/viterbi.h"

// #define VERBOSE_OUTPUT

#define TEST_VBD_IN_BYTES          30
#define TEST_VBD_OUT_BYTES         15
#define TEST_VBD_IN_SYMBOLS        240
#define TEST_VBD_OUT_BITS          120
#define TEST_VBD_CONSTRAINT_LENGTH 7
#define TEST_VBD_POLY_G1           0117  // Equivalente a 0171 en convención MSB más reciente
#define TEST_VBD_POLY_G2           0155  // Equivalente a 0133 en convención MSB más reciente
#define TEST_VBD_INVERT_G1         0
#define TEST_VBD_INVERT_G2         1


const uint8_t inputSymbols[TEST_VBD_IN_BYTES] = { 0x8C, 0x1A, 0xAA, 0x73, 0x31, 0x5A, 0x6F, 0x59, 0x78, 0x95,
                                                  0x55, 0x8C, 0xCE, 0xA5, 0x90, 0xA6, 0x84, 0x02, 0x02, 0x1B,
                                                  0x99, 0xEB, 0x5C, 0x18, 0xBB, 0xFD, 0xFD, 0xE4, 0x53, 0x22 };

const uint8_t expectedOutputBits[TEST_VBD_OUT_BYTES] = { 0xFF, 0xF0, 0xCC, 0xAA, 0x00, 0x0F, 0x33, 0x55,
                                                         0xE3, 0xEC, 0xDF, 0x8A, 0x1C, 0x13, 0x40 };

int calculateBitErrors(const uint8_t* output)
{
    int diffBits = 0;

    for (int i = 0; i < TEST_VBD_OUT_BYTES; ++i) {
        if (output[i] != expectedOutputBits[i]) {
            uint8_t x = output[i] ^ expectedOutputBits[i];
            for (int b = 0; b < 8; ++b)
                if (x & (1u << b)) {
                    diffBits++;
                }
        }
    }

    return diffBits;
}

void printBitsComparison(const uint8_t* output)
{
    printf("Expected : ");

    for (int i = 0; i < TEST_VBD_OUT_BYTES; ++i) {
        printf("%02X ", expectedOutputBits[i]);
    }

    printf("\nDecoded  : ");

    for (int i = 0; i < TEST_VBD_OUT_BYTES; ++i) {
        printf("%02X ", output[i]);
    }

    printf("\n");
}

void printResults(const char* title, int diffBits, unsigned int errorsDetected)
{
    printf("--------------------------------------------------------------------------------\n");
    printf("%s\n", title);

    if (diffBits == 0) {
        printf("Result: \033[32mOK\033[0m\n");
        printf(" >> Output bit sequence matches expected sequence\n");
        printf(" >> %u bit errors detected\n", errorsDetected);
    } else {
        printf("Result: \033[31mFAIL\033[0m\n");
        printf(" >> %d different bits.\n", diffBits);
        printf(" >> %u bit errors detected\n", errorsDetected);
    }
    printf("--------------------------------------------------------------------------------\n");
}

void processTestResults(const char* title, const uint8_t* output, uint32_t bitErrors)
{
#ifdef VERBOSE_OUTPUT
    printBitsComparison(output);
#endif

    int diffBits = calculateBitErrors(output);

    printResults(title, diffBits, bitErrors);
}


void vbdTest1(void)
{
    const char* title = "VGDB Test1: Input with 0 errors.";

    uint8_t input[TEST_VBD_IN_BYTES];
    uint8_t output[TEST_VBD_OUT_BYTES];
    int32_t ret;

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    vbd_ctrl_t* vbdCtrl;
    vgd_generator_t symbolGen[2] = { [0] = { .poly = TEST_VBD_POLY_G1, .isInverted = TEST_VBD_INVERT_G1 },
                                     [1] = { .poly = TEST_VBD_POLY_G2, .isInverted = TEST_VBD_INVERT_G2 } };
    uint8_t buffer[31232]        = { 0 };

    ret = viterbiBlockDecoderInit(&vbdCtrl, TEST_VBD_IN_SYMBOLS, 1, 2, 7, symbolGen, buffer, 31232);

    if (ret < 0) {
        printf("VGDB test error at initialization\n");
        return;
    }

    uint32_t bitErrors = viterbiBlockDecoder(vbdCtrl, input, output);

    processTestResults(title, output, bitErrors);
}

void vbdTest2(void)
{
    const char* title = "VGDB Test2: Input with 1 bit error.";

    uint8_t input[TEST_VBD_IN_BYTES];
    uint8_t output[TEST_VBD_OUT_BYTES];
    int32_t ret;

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    /* Inserting 1 bit error */
    input[3] ^= 0b100;

    vbd_ctrl_t* vbdCtrl;
    vgd_generator_t symbolGen[2] = { [0] = { .poly = TEST_VBD_POLY_G1, .isInverted = TEST_VBD_INVERT_G1 },
                                     [1] = { .poly = TEST_VBD_POLY_G2, .isInverted = TEST_VBD_INVERT_G2 } };
    uint8_t buffer[31232]        = { 0 };

    ret = viterbiBlockDecoderInit(&vbdCtrl, TEST_VBD_IN_SYMBOLS, 1, 2, 7, symbolGen, buffer, 31232);

    if (ret < 0) {
        printf("VGDB test error at initialization\n");
        return;
    }

    uint32_t bitErrors = viterbiBlockDecoder(vbdCtrl, input, output);

    processTestResults(title, output, bitErrors);
}

void vbdTest3(void)
{
    const char* title = "VBD Test3: Input with 30 bit errors (1 per byte).";

    uint8_t input[TEST_VBD_IN_BYTES];
    uint8_t output[TEST_VBD_OUT_BYTES];
    int32_t ret;
    int i;

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    /* Inserting 1 bit error at each byte */
    for (i = 0; i < TEST_VBD_IN_BYTES; i++) {
        input[i] ^= 0b100;
    }

    vbd_ctrl_t* vbdCtrl;
    vgd_generator_t symbolGen[2] = { [0] = { .poly = TEST_VBD_POLY_G1, .isInverted = TEST_VBD_INVERT_G1 },
                                     [1] = { .poly = TEST_VBD_POLY_G2, .isInverted = TEST_VBD_INVERT_G2 } };
    uint8_t buffer[31232]        = { 0 };

    ret = viterbiBlockDecoderInit(&vbdCtrl, TEST_VBD_IN_SYMBOLS, 1, 2, 7, symbolGen, buffer, 31232);

    if (ret < 0) {
        printf("VGDB test error at initialization\n");
        return;
    }

    uint32_t bitErrors = viterbiBlockDecoder(vbdCtrl, input, output);

    processTestResults(title, output, bitErrors);
}

void vbdTest4(void)
{
    const char* title = "VBD Test4: Input with 5 bit errors (burst of 5).";

    uint8_t input[TEST_VBD_IN_BYTES];
    uint8_t output[TEST_VBD_OUT_BYTES];
    int32_t ret;

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    /* Inserting 5 bit error */
    input[TEST_VBD_IN_BYTES / 2] ^= 0b11111;

    vbd_ctrl_t* vbdCtrl;
    vgd_generator_t symbolGen[2] = { [0] = { .poly = TEST_VBD_POLY_G1, .isInverted = TEST_VBD_INVERT_G1 },
                                     [1] = { .poly = TEST_VBD_POLY_G2, .isInverted = TEST_VBD_INVERT_G2 } };
    uint8_t buffer[31232]        = { 0 };

    ret = viterbiBlockDecoderInit(&vbdCtrl, TEST_VBD_IN_SYMBOLS, 1, 2, 7, symbolGen, buffer, 31232);

    if (ret < 0) {
        printf("VGDB test error at initialization\n");
        return;
    }

    uint32_t bitErrors = viterbiBlockDecoder(vbdCtrl, input, output);

    processTestResults(title, output, bitErrors);
}

int main(void)
{
    vbdTest1();
    vbdTest2();
    vbdTest3();
    vbdTest4();
}
