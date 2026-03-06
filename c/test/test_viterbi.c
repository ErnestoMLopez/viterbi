#include <stdio.h>
#include <stdint.h>

#include "../src/viterbi.h"

int main (void) {
    int i = 1;

    i = viterbi();

    printf("Viterbi return = %u", i);
}