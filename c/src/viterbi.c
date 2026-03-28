/**
 * \file Viterbi decoder implementation.
 */

/* =======================================================================
 * [INCLUDES]
 * =======================================================================
 */

#include "viterbi.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>


/* =======================================================================
 * [MACROS]
 * =======================================================================
 */

#define VSD_NSTATES             (1 << (VSD_CONSTRAINT_LENGTH - 1))
#define VSD_SHIFT_REGISTER_MASK ((1 << VSD_CONSTRAINT_LENGTH) - 1)
#define VSD_STATE_MASK          ((1 << (VSD_CONSTRAINT_LENGTH - 1)) - 1)


/* =======================================================================
 * [TYPEDEF]
 * =======================================================================
 */

typedef struct vgbd_ctrl {
    uint8_t symbolsPerInput;                                      //< Symbols count at input buffer (N)
    uint8_t bitsPerStep;                                          //< Bits input to the encoder at each step (k)
    uint8_t symbolsPerStep;                                       //< Symbols output by the encoder at each step (n)
    uint8_t constraintLength;                                     //< Constraint length (K)
    vgd_generator_t symbolGenerators[VGBD_MAX_SYMBOLS_PER_STEP];  //< Generators for each symbol
    uint32_t* survivors;        //< Buffer for surviving paths. Must be of size 4*N*(k/n)*(2^((K-1)*k))
    uint32_t* pathMetricsCurr;  //< Buffer for current metrics. Must be of size 4*(2^((K-1)*k))
    uint32_t* pathMetricsNext;  //< Buffer for next step metrics. Must be of size 4*(2^((K-1)*k))
} vgbd_ctrl_t;

/* =======================================================================
 * [FUNCTION  PROTOTYPES]
 * =======================================================================
 */

/* =======================================================================
 * [INTERNAL DATA DEFINITION]
 * =======================================================================
 */

uint8_t vgbdCount = 0;
vgbd_ctrl_t vgbdTable[VGBD_MAX_DECODERS];


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

static inline uint8_t getInputSymbols(const uint8_t* in, const uint32_t step, const uint32_t symbolsPerStep)
{
    uint32_t i;
    uint32_t symbol = 0;
    uint8_t symbols = 0;

    for (i = 0; i < symbolsPerStep; i++) {

        symbol = symbolsPerStep * step + i;

        const uint32_t byte   = symbol >> 3;
        const uint32_t bitpos = 7 - (symbol & 7);

        symbols |= ((in[byte] >> bitpos) & 1) << i;
    }

    return symbols;
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

static inline void setOutBits(uint8_t* out, const uint32_t step, const uint8_t bitsPerStep, const uint8_t decodedBits)
{
    uint8_t i;
    uint8_t bit;

    for (i = 0; i < bitsPerStep; i++) {

        bit = bitsPerStep * step + i;

        const uint32_t byte   = bit >> 3;
        const uint32_t bitpos = 7 - (bit & 7);

        out[byte] |= (((decodedBits >> i) & 1) << bitpos);
    }
}

static inline uint8_t encodeBit(uint32_t state, uint8_t bit)
{
    const uint32_t reg = ((state << 1) | bit) & VSD_SHIFT_REGISTER_MASK;
    const uint8_t g1   = parity8(reg & VSD_POLY_G1) ^ VSD_INVERT_G1;
    const uint8_t g2   = parity8(reg & VSD_POLY_G2) ^ VSD_INVERT_G2;

    return g1 | (g2 << 1);
}


static inline uint8_t encodeBits(
        uint32_t state,
        const uint8_t bits,
        const uint8_t bitsPerStep,
        const uint8_t symbolsPerStep,
        const uint8_t constraintLength,
        const vgd_generator_t* symbolGen)
{
    const uint32_t shiftRegisterMask = ((1 << constraintLength) - 1);
    const uint32_t reg               = ((state << bitsPerStep) | bits) & shiftRegisterMask;

    uint8_t symbols = 0;
    uint8_t i;

    for (i = 0; i < symbolsPerStep; i++) {
        const uint8_t symbol = parity8(reg & symbolGen[i].poly) ^ symbolGen[i].isInverted;
        symbols |= symbol << i;
    }

    return symbols;
}

static inline uint8_t calculateError(const uint8_t symbols, const uint8_t outSymbols)
{
    uint8_t bitErrors = symbols ^ outSymbols;
    uint8_t errors    = 0;

    while (bitErrors) {
        bitErrors &= (bitErrors - 1);
        errors++;
    }

    return errors;
}

static inline uint32_t getNextState(const uint32_t state, const uint8_t newBit)
{
    return ((state << 1) | newBit) & VSD_STATE_MASK;
}

static inline uint8_t getLastBitsFromState(const uint32_t state, const uint8_t bitsPerStep)
{
    const uint32_t lastBitsMask = ((1 << bitsPerStep) - 1);
    return state & lastBitsMask;
}


/* =======================================================================
 * [PUBLIC INTERFACE FUNCTIONS DEFINITION]
 * =======================================================================
 */


uint32_t viterbiStaticDecoder(const uint8_t in[VSD_IN_BYTES], uint8_t out[VSD_OUT_BYTES])
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
        const uint32_t decodedBit = state & 1;
        setOutBit(out, outBit, decodedBit);
        state = (int32_t)survivors[outBit][state];
    }

    return bestMetric;
}


int32_t viterbiGenericBlockDecoderInit(
        vgbd_ctx_t* vgbdCtx,
        const uint8_t symbolsPerInput,
        const uint8_t bitsPerStep,
        const uint8_t symbolsPerStep,
        const uint8_t constraintLength,
        const vgd_generator_t* symbolGenerators,
        const uint8_t* workingBuffer,
        const uint32_t bufferSize)
{
    *vgbdCtx = NULL;

    if (++vgbdCount >= VGBD_MAX_DECODERS) {
        fprintf(stderr, "Viterbi generic decoders limit reached (%u maximum)\n", VGBD_MAX_DECODERS);
        return -1;
    }

    if (symbolsPerStep > VGBD_MAX_SYMBOLS_PER_STEP) {
        fprintf(stderr, "Too many output symbols generators (%u maximum)\n", VGBD_MAX_SYMBOLS_PER_STEP);
        return -2;
    }

    const uint32_t stateCount            = 1 << ((constraintLength - 1) * bitsPerStep);
    const uint32_t survivorsBufferSize   = 4 * symbolsPerInput * bitsPerStep * stateCount / symbolsPerStep;
    const uint32_t pathMetricsBufferSize = 4 * stateCount;
    const uint32_t totalBufferSize       = survivorsBufferSize + 2 * pathMetricsBufferSize;

    if (bufferSize < totalBufferSize) {
        fprintf(stderr, "Not enough workspace memory (%u bytes needed, %u provided)\n", totalBufferSize, bufferSize);
        return -3;
    }

    vgbd_ctrl_t* vgdbCtrl = &vgbdTable[vgbdCount];
    uint8_t i;

    vgdbCtrl->symbolsPerInput  = symbolsPerInput;
    vgdbCtrl->bitsPerStep      = bitsPerStep;
    vgdbCtrl->symbolsPerStep   = symbolsPerStep;
    vgdbCtrl->constraintLength = constraintLength;

    for (i = 0; i < symbolsPerStep; i++) {
        vgdbCtrl->symbolGenerators[i] = symbolGenerators[i];
    }

    vgdbCtrl->survivors       = (uint32_t*)workingBuffer;
    vgdbCtrl->pathMetricsCurr = (uint32_t*)(workingBuffer + survivorsBufferSize);
    vgdbCtrl->pathMetricsNext = (uint32_t*)(workingBuffer + survivorsBufferSize + pathMetricsBufferSize);

    *vgbdCtx = vgdbCtrl;

    return 0;
}


int32_t viterbiGenericBlockDecoder(const vgbd_ctx_t vgbdCtx, const uint8_t* in, uint8_t* out)
{
    if (vgbdCtx == NULL || in == NULL || out == NULL) {
        return -1;
    }

    vgbd_ctrl_t* vgbdCtrl = (vgbd_ctrl_t*)vgbdCtx;

    const uint32_t infinity    = UINT32_MAX;
    const int32_t stateCount   = 1 << ((vgbdCtrl->constraintLength - 1) * vgbdCtrl->bitsPerStep);
    const int32_t stepsCount   = vgbdCtrl->symbolsPerInput / vgbdCtrl->symbolsPerStep;
    const uint32_t outBytes    = (vgbdCtrl->bitsPerStep * stepsCount + 7) / 8;
    const uint8_t maxBitsValue = (1 << vgbdCtrl->bitsPerStep) - 1;

    uint32_t(*survivors)[stateCount] = (uint32_t(*)[stateCount])vgbdCtrl->survivors;
    uint32_t* pathMetricsCurr        = vgbdCtrl->pathMetricsCurr;
    uint32_t* pathMetricsNext        = vgbdCtrl->pathMetricsNext;

    int32_t state;
    int32_t step;
    uint8_t bits;


    memset(pathMetricsCurr, (int)infinity, sizeof(uint32_t) * stateCount);
    pathMetricsCurr[0] = 0;

    for (step = 0; step < stepsCount; step++) {
        memset(pathMetricsNext, (int)infinity, sizeof(uint32_t) * stateCount);

        const uint8_t symbols = getInputSymbols(in, step, vgbdCtrl->symbolsPerStep);

        for (state = 0; state < stateCount; state++) {
            uint32_t currMetric = pathMetricsCurr[state];

            if (currMetric == infinity) {
                continue;
            }

            for (bits = 0; bits <= maxBitsValue; bits++) {

                const uint32_t nextState = getNextState(state, bits);
                const uint8_t outSymbols = encodeBits(
                        state,
                        bits,
                        vgbdCtrl->bitsPerStep,
                        vgbdCtrl->symbolsPerStep,
                        vgbdCtrl->constraintLength,
                        vgbdCtrl->symbolGenerators);
                const uint32_t errors = calculateError(symbols, outSymbols);

                const uint32_t newMetric = currMetric + errors;

                if (newMetric < pathMetricsNext[nextState]) {
                    pathMetricsNext[nextState] = newMetric;
                    survivors[step][nextState] = state;
                }
            }
        }

        for (state = 0; state < stateCount; state++) {
            pathMetricsCurr[state] = pathMetricsNext[state];
        }
    }

    uint32_t bestMetric = infinity;
    uint32_t bestState  = 0;

    for (state = 0; state < stateCount; state++) {
        if (pathMetricsCurr[state] < bestMetric) {
            bestMetric = pathMetricsCurr[state];
            bestState  = state;
        }
    }

    state = (int32_t)bestState;

    memset(out, 0, outBytes);

    for (step = stepsCount - 1; step >= 0; step--) {
        const uint32_t decodedBits = getLastBitsFromState(state, vgbdCtrl->bitsPerStep);
        setOutBits(out, step, vgbdCtrl->bitsPerStep, decodedBits);
        state = (int32_t)survivors[step][state];
    }

    return (int32_t)bestMetric;
}
