#include <stdint.h>
#include <stdio.h>

#include "../src/viterbi.h"

int main(void)
{
    const uint8_t inputSymbols[VSD_IN_BYTES] = { 0x8C, 0x1A, 0xAA, 0x73, 0x31, 0x5A, 0x6F, 0x59, 0x78, 0x95,
                                                 0x55, 0x8C, 0xCE, 0xA5, 0x90, 0xA6, 0x84, 0x02, 0x02, 0x1B,
                                                 0x99, 0xEB, 0x5C, 0x18, 0xBB, 0xFD, 0xFD, 0xE4, 0x53, 0x22 };

    const uint8_t expectedOutputBits[VSD_OUT_BYTES] = { 0xFF, 0xF0, 0xCC, 0xAA, 0x00, 0x0F, 0x33, 0x55,
                                                        0xE3, 0xEC, 0xDF, 0x8A, 0x1C, 0x13, 0x40 };

    uint8_t outputBits[VSD_OUT_BYTES];

    viterbiStaticDecoder(inputSymbols, outputBits);

    printf("Expected : ");

    for (int i = 0; i < VSD_OUT_BYTES; ++i) {
        printf("%02X ", expectedOutputBits[i]);
    }

    printf("\nDecoded  : ");

    for (int i = 0; i < VSD_OUT_BYTES; ++i) {
        printf("%02X ", outputBits[i]);
    }

    printf("\n");

    int diffBytes = 0, diffBits = 0;

    for (int i = 0; i < VSD_OUT_BYTES; ++i) {
        if (outputBits[i] != expectedOutputBits[i]) {
            diffBytes++;
            uint8_t x = outputBits[i] ^ expectedOutputBits[i];
            for (int b = 0; b < 8; ++b)
                if (x & (1u << b))
                    diffBits++;
        }
    }

    if (diffBytes == 0) {
        printf("Result: OK — Output bit sequence matches expected sequence.\n");
        return 0;
    } else {
        printf("Result: FAIL — %d different bytes, %d different bits.\n", diffBytes, diffBits);
        return 1;
    }
}
