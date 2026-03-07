/**
 * \file Viterbi decoder implementation.
 */

/* =======================================================================
 * [INCLUDES]
 * =======================================================================
 */

#include "viterbi.h"

#include <stdint.h>
#include <string.h>


/* =======================================================================
 * [MACROS]
 * =======================================================================
 */

#define K VSD_CONSTRAINT_LENGTH

/* =======================================================================
 * [TYPEDEF]
 * =======================================================================
 */

/* =======================================================================
 * [FUNCTION  PROTOTYPES]
 * =======================================================================
 */

/* =======================================================================
 * [INTERNAL DATA DEFINITION]
 * =======================================================================
 */

/* =======================================================================
 * [EXTERNAL DATA DEFINITION]
 * =======================================================================
 */

/* =======================================================================
 * [PRIVATE FUNCTIONS DEFINITION]
 * =======================================================================
 */

static inline uint8_t parity8(uint8_t x)
{
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;

    return x & 1;
}

static inline uint8_t getInputSymbol(const uint8_t in[VSD_IN_BYTES], const uint32_t symbol)
{
    const uint32_t byte   = symbol >> 3;
    const uint32_t bitpos = 7 - (symbol & 7);

    return (in[byte] >> bitpos) & 1;
}

static inline void setOutBit(uint8_t out[VSD_OUT_BYTES], const uint32_t bit, const uint8_t decodedBit)
{
    if (!decodedBit) {
        return;
    }

    const uint32_t byte   = bit >> 3;
    const uint32_t bitpos = 7 - (bit & 7);

    out[byte] |= (1 << bitpos);
}

static inline uint8_t encodeBit(uint8_t state, uint8_t bit)
{
    const uint8_t reg = (uint8_t)((bit << (K - 1)) | (state & ((1 << (K - 1)) - 1)));
    const uint8_t g1  = parity8(reg & (uint8_t)VSD_POLY_G1) ^ VSD_INVERT_G1;
    const uint8_t g2  = parity8(reg & (uint8_t)VSD_POLY_G2) ^ VSD_INVERT_G2;

    return g1 | (g2 << 1);
}

static inline uint32_t getNextState(const uint32_t state, const uint8_t newBit)
{
    uint32_t nextState = (uint8_t)(((uint8_t)state >> 1) | (newBit << (K - 2)));
    nextState &= (VSD_NSTATES - 1);

    return nextState;
}

/* =======================================================================
 * [PUBLIC INTERFACE FUNCTIONS DEFINITION]
 * =======================================================================
 */


void viterbiStaticDecoder(const uint8_t in[VSD_IN_BYTES], uint8_t out[VSD_OUT_BYTES])
{
    const uint32_t infinity = UINT32_MAX;

    uint32_t survivors[VSD_OUT_BITS][VSD_NSTATES];
    uint32_t pathMetricsCurr[VSD_NSTATES];
    uint32_t pathMetricsNext[VSD_NSTATES];
    int32_t state;
    int32_t outBit;
    int8_t bit;

    memset(pathMetricsCurr, (int)infinity, sizeof(pathMetricsCurr));
    pathMetricsCurr[0] = 0;

    for (outBit = 0; outBit < VSD_OUT_BITS; outBit++) {

        memset(pathMetricsNext, (int)infinity, sizeof(pathMetricsNext));

        const uint8_t symbol1 = getInputSymbol(in, 2 * outBit);
        const uint8_t symbol2 = getInputSymbol(in, 2 * outBit + 1);

        for (state = 0; state < VSD_NSTATES; state++) {
            uint32_t currMetric = pathMetricsCurr[state];

            if (currMetric == infinity) {
                continue;
            }

            for (bit = 0; bit <= 1; bit++) {
                const uint32_t nextState = getNextState(state, bit);
                const uint8_t outSymbols = encodeBit(state, bit);
                const uint8_t outSymbol1 = outSymbols & 1;
                const uint8_t outSymbol2 = (outSymbols >> 1) & 1;

                const uint32_t errors    = (outSymbol1 != symbol1) + (outSymbol2 != symbol2);
                const uint32_t newMetric = currMetric + errors;

                if (newMetric < pathMetricsNext[nextState]) {
                    pathMetricsNext[nextState]   = newMetric;
                    survivors[outBit][nextState] = state;
                }
            }
        }

        for (state = 0; state < VSD_NSTATES; state++) {
            pathMetricsCurr[state] = pathMetricsNext[state];
        }
    }

    uint32_t bestMetric = infinity;
    uint32_t bestState  = 0;

    for (state = 0; state < VSD_NSTATES; state++) {
        if (pathMetricsCurr[state] < bestMetric) {
            bestMetric = pathMetricsCurr[state];
            bestState  = state;
        }
    }

    state = (int32_t)bestState;

    memset(out, 0, VSD_OUT_BYTES);

    for (outBit = VSD_OUT_BITS - 1; outBit >= 0; outBit--) {
        const uint8_t decodedBit = (uint8_t)((state >> (K - 2)) & 1);
        setOutBit(out, outBit, decodedBit);
        state = (int32_t)survivors[outBit][state];
    }
}
