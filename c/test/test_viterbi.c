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

#define RED(x)   "\033[31m" x "\033[0m"
#define GREEN(x) "\033[32m" x "\033[0m"

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
        printf("Result: " GREEN("OK") "\n");
        printf(" >> Output bit sequence matches expected sequence\n");
        printf(" >> %u bit errors detected\n", errorsDetected);
    } else {
        printf("Result: " RED("FAIL") "\n");
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


void vbdFailInitTest1(void)
{
    const char* title = "VBD Init Test 1: Invalid pointer arguments.";

    uint8_t input[TEST_VBD_IN_BYTES];
    int32_t ret;
    int status = 0;

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    vbd_ctrl_t* vbdCtrl;
    v_generator_t symbolGen[9] = { [0] = { .poly = TEST_VBD_POLY_G1, .isInverted = TEST_VBD_INVERT_G1 },
                                   [8] = { .poly = TEST_VBD_POLY_G2, .isInverted = TEST_VBD_INVERT_G2 } };
    uint8_t buffer[31232]      = { 0 };

    ret = viterbiBlockDecoderInit(NULL, TEST_VBD_IN_SYMBOLS, 1, 9, 7, symbolGen, buffer, 31232);

    if (ret != -1) {
        status = 1;
    }

    viterbiBlockDecoderFree(vbdCtrl);

    ret = viterbiBlockDecoderInit(&vbdCtrl, TEST_VBD_IN_SYMBOLS, 1, 9, 7, NULL, buffer, 31232);

    if (ret != -1) {
        status = 1;
    }

    viterbiBlockDecoderFree(vbdCtrl);

    ret = viterbiBlockDecoderInit(&vbdCtrl, TEST_VBD_IN_SYMBOLS, 1, 9, 7, symbolGen, NULL, 31232);

    if (ret != -1) {
        status = 1;
    }

    viterbiBlockDecoderFree(vbdCtrl);

    printf("--------------------------------------------------------------------------------\n");
    printf("%s\n", title);
    if (status) {
        printf("Result: " RED("FAIL") "\n");
        printf(" >> Expected error code -1, got %d\n", ret);
    } else {
        printf("Result: " GREEN("OK") "\n");
        printf(" >> Correct error code -1 received when passing invalid pointer arguments.\n");
    }
    printf("--------------------------------------------------------------------------------\n");
}

void vbdFailInitTest2(void)
{
    const char* title = "VBD Init Test 2: Too many instances.";

    uint8_t input[TEST_VBD_IN_BYTES];
    int32_t ret;
    int i;

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    vbd_ctrl_t* vbdCtrl[VBD_MAX_DECODERS + 1];
    v_generator_t symbolGen[2] = { [0] = { .poly = TEST_VBD_POLY_G1, .isInverted = TEST_VBD_INVERT_G1 },
                                   [1] = { .poly = TEST_VBD_POLY_G2, .isInverted = TEST_VBD_INVERT_G2 } };
    uint8_t buffer[31232]      = { 0 };

    for (i = 0; i < VBD_MAX_DECODERS; i++) {
        ret = viterbiBlockDecoderInit(&vbdCtrl[i], TEST_VBD_IN_SYMBOLS, 1, 2, 7, symbolGen, buffer, 31232);

        if (ret < 0) {
            printf("VBD test error at initialization\n");
            return;
        }
    }

    ret = viterbiBlockDecoderInit(&vbdCtrl[i], TEST_VBD_IN_SYMBOLS, 1, 2, 7, symbolGen, buffer, 31232);

    for (i = 0; i < VBD_MAX_DECODERS; i++) {
        viterbiBlockDecoderFree(vbdCtrl[i]);
    }

    printf("--------------------------------------------------------------------------------\n");
    printf("%s\n", title);
    if (ret != -2) {
        printf("Result: " RED("FAIL") "\n");
        printf(" >> Expected error code -2, got %d\n", ret);
    } else {
        printf("Result: " GREEN("OK") "\n");
        printf(" >> Correct error code -2 received when exceeding max instances.\n");
    }
    printf("--------------------------------------------------------------------------------\n");
}

void vbdFailInitTest3(void)
{
    const char* title = "VBD Init Test 3: Too many symbols per step.";

    uint8_t input[TEST_VBD_IN_BYTES];
    int32_t ret;

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    vbd_ctrl_t* vbdCtrl;
    v_generator_t symbolGen[9] = { [0] = { .poly = TEST_VBD_POLY_G1, .isInverted = TEST_VBD_INVERT_G1 },
                                   [8] = { .poly = TEST_VBD_POLY_G2, .isInverted = TEST_VBD_INVERT_G2 } };
    uint8_t buffer[31232]      = { 0 };

    ret = viterbiBlockDecoderInit(&vbdCtrl, TEST_VBD_IN_SYMBOLS, 1, 9, 7, symbolGen, buffer, 31232);

    viterbiBlockDecoderFree(vbdCtrl);

    printf("--------------------------------------------------------------------------------\n");
    printf("%s\n", title);
    if (ret != -3) {
        printf("Result: " RED("FAIL") "\n");
        printf(" >> Expected error code -3, got %d\n", ret);
    } else {
        printf("Result: " GREEN("OK") "\n");
        printf(" >> Correct error code -3 received when exceeding max symbols per step.\n");
    }
    printf("--------------------------------------------------------------------------------\n");
}

void vbdFailInitTest4(void)
{
    const char* title = "VBD Init Test 4: Working buffer too small.";

    uint8_t input[TEST_VBD_IN_BYTES];
    int32_t ret;

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    vbd_ctrl_t* vbdCtrl;
    v_generator_t symbolGen[9] = { [0] = { .poly = TEST_VBD_POLY_G1, .isInverted = TEST_VBD_INVERT_G1 },
                                   [8] = { .poly = TEST_VBD_POLY_G2, .isInverted = TEST_VBD_INVERT_G2 } };

    const uint32_t N              = TEST_VBD_IN_SYMBOLS;
    const uint32_t k              = 1;
    const uint32_t n              = 2;
    const uint32_t K              = TEST_VBD_CONSTRAINT_LENGTH;
    const size_t bufferSizeNeeded = sizeof(v_state_t) * N * k * (1 << ((K - 1) * k)) / n
            + 2 * sizeof(uint32_t) * (1 << ((K - 1) * k));
    uint8_t buffer[bufferSizeNeeded];

    memset(buffer, 0, bufferSizeNeeded);

    ret = viterbiBlockDecoderInit(&vbdCtrl, N, k, n, K, symbolGen, buffer, bufferSizeNeeded - 1);

    viterbiBlockDecoderFree(vbdCtrl);

    printf("--------------------------------------------------------------------------------\n");
    printf("%s\n", title);
    if (ret != -4) {
        printf("Result: " RED("FAIL") "\n");
        printf(" >> Expected error code -4, got %d\n", ret);
    } else {
        printf("Result: " GREEN("OK") "\n");
        printf(" >> Correct error code -4 received when the working buffer is not enough.\n");
    }
    printf("--------------------------------------------------------------------------------\n");
}

void vbdFailInitTest5(void)
{
    const char* title = "VBD Init Test 5: Data type for v_state_t is not wide enough.";

    uint8_t input[TEST_VBD_IN_BYTES];
    int32_t ret;

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    vbd_ctrl_t* vbdCtrl;
    v_generator_t symbolGen[9] = { [0] = { .poly = TEST_VBD_POLY_G1, .isInverted = TEST_VBD_INVERT_G1 },
                                   [8] = { .poly = TEST_VBD_POLY_G2, .isInverted = TEST_VBD_INVERT_G2 } };

    const uint32_t N = 10;
    const uint32_t k = 3;
    const uint32_t n = 8;
    const uint32_t K = TEST_VBD_CONSTRAINT_LENGTH;

    const size_t bufferSizeNeeded = sizeof(v_state_t) * N * k * (1 << ((K - 1) * k)) / n
            + 2 * sizeof(uint32_t) * (1 << ((K - 1) * k));

    // Fake buffer to avoid huge memory allocation for the test
    uint8_t buffer = 0;


    ret = viterbiBlockDecoderInit(&vbdCtrl, N, k, n, K, symbolGen, &buffer, bufferSizeNeeded);

    viterbiBlockDecoderFree(vbdCtrl);

    printf("--------------------------------------------------------------------------------\n");
    printf("%s\n", title);
    if (ret != -5) {
        printf("Result: " RED("FAIL") "\n");
        printf(" >> Expected error code -5, got %d\n", ret);
    } else {
        printf("Result: " GREEN("OK") "\n");
        printf(" >> Correct error code -5 received when the data type for v_state_t is not wide enough.\n");
    }
    printf("--------------------------------------------------------------------------------\n");
}

void vbdTest1(void)
{
    const char* title = "VBD Test1: Input with 0 errors.";

    uint8_t input[TEST_VBD_IN_BYTES];
    uint8_t output[TEST_VBD_OUT_BYTES];
    int32_t ret;

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    vbd_ctrl_t* vbdCtrl;
    v_generator_t symbolGen[2] = { [0] = { .poly = TEST_VBD_POLY_G1, .isInverted = TEST_VBD_INVERT_G1 },
                                   [1] = { .poly = TEST_VBD_POLY_G2, .isInverted = TEST_VBD_INVERT_G2 } };
    uint8_t buffer[31232]      = { 0 };

    ret = viterbiBlockDecoderInit(&vbdCtrl, TEST_VBD_IN_SYMBOLS, 1, 2, 7, symbolGen, buffer, 31232);

    if (ret < 0) {
        printf("VBD test error at initialization\n");
        return;
    }

    uint32_t bitErrors = viterbiBlockDecoder(vbdCtrl, input, output);

    ret = viterbiBlockDecoderFree(vbdCtrl);

    if (ret < 0) {
        printf("VBD test error at freeing decoder instance\n");
        return;
    }

    processTestResults(title, output, bitErrors);
}

void vbdTest2(void)
{
    const char* title = "VBD Test2: Input with 1 bit error.";

    uint8_t input[TEST_VBD_IN_BYTES];
    uint8_t output[TEST_VBD_OUT_BYTES];
    int32_t ret;

    memcpy(input, inputSymbols, sizeof(inputSymbols));

    /* Inserting 1 bit error */
    input[3] ^= 0b100;

    vbd_ctrl_t* vbdCtrl;
    v_generator_t symbolGen[2] = { [0] = { .poly = TEST_VBD_POLY_G1, .isInverted = TEST_VBD_INVERT_G1 },
                                   [1] = { .poly = TEST_VBD_POLY_G2, .isInverted = TEST_VBD_INVERT_G2 } };
    uint8_t buffer[31232]      = { 0 };

    ret = viterbiBlockDecoderInit(&vbdCtrl, TEST_VBD_IN_SYMBOLS, 1, 2, 7, symbolGen, buffer, 31232);

    if (ret < 0) {
        printf("VBD test error at initialization\n");
        return;
    }

    uint32_t bitErrors = viterbiBlockDecoder(vbdCtrl, input, output);

    ret = viterbiBlockDecoderFree(vbdCtrl);

    if (ret < 0) {
        printf("VBD test error at freeing decoder instance\n");
        return;
    }

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
    v_generator_t symbolGen[2] = { [0] = { .poly = TEST_VBD_POLY_G1, .isInverted = TEST_VBD_INVERT_G1 },
                                   [1] = { .poly = TEST_VBD_POLY_G2, .isInverted = TEST_VBD_INVERT_G2 } };
    uint8_t buffer[31232]      = { 0 };

    ret = viterbiBlockDecoderInit(&vbdCtrl, TEST_VBD_IN_SYMBOLS, 1, 2, 7, symbolGen, buffer, 31232);

    if (ret < 0) {
        printf("VBD test error at initialization\n");
        return;
    }

    uint32_t bitErrors = viterbiBlockDecoder(vbdCtrl, input, output);

    ret = viterbiBlockDecoderFree(vbdCtrl);

    if (ret < 0) {
        printf("VBD test error at freeing decoder instance\n");
        return;
    }

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
    v_generator_t symbolGen[2] = { [0] = { .poly = TEST_VBD_POLY_G1, .isInverted = TEST_VBD_INVERT_G1 },
                                   [1] = { .poly = TEST_VBD_POLY_G2, .isInverted = TEST_VBD_INVERT_G2 } };
    uint8_t buffer[31232]      = { 0 };

    ret = viterbiBlockDecoderInit(&vbdCtrl, TEST_VBD_IN_SYMBOLS, 1, 2, 7, symbolGen, buffer, 31232);

    if (ret < 0) {
        printf("VBD test error at initialization\n");
        return;
    }

    uint32_t bitErrors = viterbiBlockDecoder(vbdCtrl, input, output);

    ret = viterbiBlockDecoderFree(vbdCtrl);

    if (ret < 0) {
        printf("VBD test error at freeing decoder instance\n");
        return;
    }

    processTestResults(title, output, bitErrors);
}

int main(void)
{
    vbdFailInitTest1();
    vbdFailInitTest2();
    vbdFailInitTest3();
    vbdFailInitTest4();
    vbdFailInitTest5();

    vbdTest1();
    vbdTest2();
    vbdTest3();
    vbdTest4();
}
