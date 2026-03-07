#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/viterbi.h"

const uint8_t inputSymbols[VSD_IN_BYTES] = { 0x8C, 0x1A, 0xAA, 0x73, 0x31, 0x5A, 0x6F, 0x59, 0x78, 0x95,
                                             0x55, 0x8C, 0xCE, 0xA5, 0x90, 0xA6, 0x84, 0x02, 0x02, 0x1B,
                                             0x99, 0xEB, 0x5C, 0x18, 0xBB, 0xFD, 0xFD, 0xE4, 0x53, 0x22 };

const uint8_t expectedOutputBits[VSD_OUT_BYTES] = { 0xFF, 0xF0, 0xCC, 0xAA, 0x00, 0x0F, 0x33, 0x55,
                                                    0xE3, 0xEC, 0xDF, 0x8A, 0x1C, 0x13, 0x40 };

int calculateBitErrors(uint8_t* output)
{
    int diffBits = 0;

    for (int i = 0; i < VSD_OUT_BYTES; ++i) {
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

void printBitsComparison(uint8_t* output)
{
    printf("Expected : ");

    for (int i = 0; i < VSD_OUT_BYTES; ++i) {
        printf("%02X ", expectedOutputBits[i]);
    }

    printf("\nDecoded  : ");

    for (int i = 0; i < VSD_OUT_BYTES; ++i) {
        printf("%02X ", output[i]);
    }

    printf("\n");
}

void printResults(int diffBits)
{
    if (diffBits == 0) {
        printf("Result: OK — Output bit sequence matches expected sequence.\n");
    } else {
        printf("Result: FAIL — %d different bits.\n", diffBits);
    }
}

void test1(void)
{
    printf("Test 1: Input without errors.\n");

    uint8_t input[VSD_IN_BYTES];
    uint8_t output[VSD_OUT_BYTES];

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    viterbiStaticDecoder(input, output);

    printBitsComparison(output);

    int diffBits = calculateBitErrors(output);

    printResults(diffBits);
}

void test2(void)
{
    printf("Test 2: Input with 1 bit error.\n");

    uint8_t input[VSD_IN_BYTES];
    uint8_t output[VSD_OUT_BYTES];

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    /* Inserting 1 bit error */
    input[3] ^= 0b100;

    viterbiStaticDecoder(input, output);

    printBitsComparison(output);

    int diffBits = calculateBitErrors(output);

    printResults(diffBits);
}

void test3(void)
{
    printf("Test 3: Input with 1 bit error per byte.\n");

    uint8_t input[VSD_IN_BYTES];
    uint8_t output[VSD_OUT_BYTES];
    int i;

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    /* Inserting 1 bit error at each byte */
    for (i = 0; i < VSD_IN_BYTES; i++) {
        input[i] ^= 0b100;
    }

    viterbiStaticDecoder(input, output);

    printBitsComparison(output);

    int diffBits = calculateBitErrors(output);

    printResults(diffBits);
}

void test4(void)
{
    printf("Test 4: Input with 2 bit errors per byte.\n");

    uint8_t input[VSD_IN_BYTES];
    uint8_t output[VSD_OUT_BYTES];
    int i;

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    /* Inserting 2 bit error at each byte */
    for (i = 0; i < VSD_IN_BYTES; i++) {
        input[i] ^= 0b0101;
    }

    viterbiStaticDecoder(input, output);

    printBitsComparison(output);

    int diffBits = calculateBitErrors(output);

    printResults(diffBits);
}

void test5(void)
{
    printf("Test 5: Input with a burst of 5 bit errors.\n");

    uint8_t input[VSD_IN_BYTES];
    uint8_t output[VSD_OUT_BYTES];

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    /* Inserting 5 bit error */
    input[VSD_IN_BYTES / 2] ^= 0b11111;

    viterbiStaticDecoder(input, output);

    printBitsComparison(output);

    int diffBits = calculateBitErrors(output);

    printResults(diffBits);
}

int main(void)
{
    test1();
    test2();
    test3();
    test4();
    test5();
}
