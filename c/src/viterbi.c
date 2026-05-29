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

/**
 * @brief Maximum number of output symbols per step (n) supported by the decoder. This limit is needed only to determine
 * the memory asigned to store the output generators.
 */
#define VBD_MAX_SYMBOLS_PER_STEP 8


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

struct vbd_ctrl {
    uint8_t symbolsPerInput;                                   //< Symbols count at input buffer (N)
    uint8_t bitsPerStep;                                       //< Bits input to the encoder at each step (k)
    uint8_t symbolsPerStep;                                    //< Symbols output by the encoder at each step (n)
    uint8_t constraintLength;                                  //< Constraint length (K)
    v_generator_t symbolGenerators[VBD_MAX_SYMBOLS_PER_STEP];  //< Generators for each symbol
    v_state_t* survivors;   //< Buffer for surviving paths. Must be of size 4*N*(k/n)*(2^((K-1)*k))
    uint32_t* currMetrics;  //< Buffer for current metrics. Must be of size 4*(2^((K-1)*k))
    uint32_t* nextMetrics;  //< Buffer for next step metrics. Must be of size 4*(2^((K-1)*k))
};

uint8_t vbdCount = 0;
vbd_ctrl_t vbdTable[VBD_MAX_DECODERS];


/* =======================================================================
 * [EXTERNAL DATA DEFINITION]
 * =======================================================================
 */

/* =======================================================================
 * [PRIVATE FUNCTIONS DEFINITION]
 * =======================================================================
 */

static inline uint8_t parity32(uint32_t x)
{
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;

    return x & 1;
}

static inline uint8_t getInputSymbols(const uint8_t* in, const uint32_t step, const uint8_t symbolsPerStep)
{
    const uint32_t symbolStart = symbolsPerStep * step;
    uint8_t symbols            = 0;
    uint32_t i;

    for (i = 0; i < symbolsPerStep; i++) {

        const uint32_t symbol = symbolStart + i;
        const uint32_t byte   = symbol >> 3;
        const uint32_t bitpos = 7 - (symbol & 7);

        symbols |= ((in[byte] >> bitpos) & 1) << i;
    }

    return symbols;
}

static inline void setOutBits(uint8_t* out, const uint32_t step, const uint8_t bitsPerStep, const uint8_t decodedBits)
{
    const uint32_t bitStart = bitsPerStep * step;
    uint8_t i;

    for (i = 0; i < bitsPerStep; i++) {

        const uint32_t bit    = bitStart + i;
        const uint32_t byte   = bit >> 3;
        const uint32_t bitpos = 7 - (bit & 7);

        out[byte] |= (((decodedBits >> i) & 1) << bitpos);
    }
}

static inline uint8_t encodeBits(
        v_state_t state,
        const uint8_t bits,
        const uint8_t bitsPerStep,
        const uint8_t symbolsPerStep,
        const v_generator_t* symbolGen)
{
    const uint32_t reg = (((uint32_t)state << bitsPerStep) | bits);

    uint8_t symbols = 0;
    uint8_t i;

    for (i = 0; i < symbolsPerStep; i++) {
        const uint8_t symbol = parity32(reg & symbolGen[i].poly) ^ symbolGen[i].isInverted;
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

static inline v_state_t
getNextState(const v_state_t state, const uint8_t bits, const uint8_t bitsPerStep, const uint8_t constraintLength)
{
    const uint32_t stateMask = ((1 << ((constraintLength - 1) * bitsPerStep)) - 1);

    return (v_state_t)((((uint32_t)state << bitsPerStep) | bits) & stateMask);
}

static inline uint8_t getLastBitsFromState(const v_state_t state, const uint8_t bitsPerStep)
{
    const uint32_t lastBitsMask = ((1 << bitsPerStep) - 1);

    return (uint8_t)((uint32_t)state & lastBitsMask);
}


/* =======================================================================
 * [PUBLIC INTERFACE FUNCTIONS DEFINITION]
 * =======================================================================
 */

int32_t viterbiBlockDecoderInit(
        vbd_ctrl_t** vbdCtrl,
        const uint8_t symbolsPerInput,
        const uint8_t bitsPerStep,
        const uint8_t symbolsPerStep,
        const uint8_t constraintLength,
        const v_generator_t* symbolGenerators,
        const uint8_t* workingBuffer,
        const uint32_t bufferSize)
{
    if (++vbdCount >= VBD_MAX_DECODERS) {
        fprintf(stderr, "Viterbi generic decoders limit reached (%u maximum)\n", VBD_MAX_DECODERS);
        vbdCount--;
        *vbdCtrl = NULL;
        return -1;
    }

    if (symbolsPerStep > VBD_MAX_SYMBOLS_PER_STEP) {
        fprintf(stderr, "Too many output symbols generators (%u maximum)\n", VBD_MAX_SYMBOLS_PER_STEP);
        return -2;
    }

    const uint32_t stateCount          = 1 << ((constraintLength - 1) * bitsPerStep);
    const uint32_t survivorsBufferSize = sizeof(v_state_t) * symbolsPerInput * bitsPerStep * stateCount
            / symbolsPerStep;
    const uint32_t pathMetricsBufferSize = sizeof(uint32_t) * stateCount;
    const uint32_t totalBufferSize       = survivorsBufferSize + 2 * pathMetricsBufferSize;

    if (bufferSize < totalBufferSize) {
        fprintf(stderr, "Not enough workspace memory (%u bytes needed, %u provided)\n", totalBufferSize, bufferSize);
        return -3;
    }

    uint8_t i;

    *vbdCtrl = &vbdTable[vbdCount];

    (*vbdCtrl)->symbolsPerInput  = symbolsPerInput;
    (*vbdCtrl)->bitsPerStep      = bitsPerStep;
    (*vbdCtrl)->symbolsPerStep   = symbolsPerStep;
    (*vbdCtrl)->constraintLength = constraintLength;

    for (i = 0; i < symbolsPerStep; i++) {
        (*vbdCtrl)->symbolGenerators[i] = symbolGenerators[i];
    }

    (*vbdCtrl)->survivors   = (v_state_t*)workingBuffer;
    (*vbdCtrl)->currMetrics = (uint32_t*)(workingBuffer + survivorsBufferSize);
    (*vbdCtrl)->nextMetrics = (uint32_t*)(workingBuffer + survivorsBufferSize + pathMetricsBufferSize);

    return 0;
}


int32_t viterbiBlockDecoder(const vbd_ctrl_t* vbdCtrl, const uint8_t* in, uint8_t* out)
{
    if (vbdCtrl == NULL || in == NULL || out == NULL) {
        return -1;
    }

    const uint32_t infinity    = UINT32_MAX;
    const uint32_t stateCount  = 1 << ((vbdCtrl->constraintLength - 1) * vbdCtrl->bitsPerStep);
    const int32_t stepsCount   = vbdCtrl->symbolsPerInput / vbdCtrl->symbolsPerStep;
    const uint32_t outBytes    = (vbdCtrl->bitsPerStep * stepsCount + 7) / 8;
    const uint8_t maxBitsValue = (1 << vbdCtrl->bitsPerStep) - 1;

    v_state_t(*survivors)[stateCount] = (v_state_t(*)[stateCount])vbdCtrl->survivors;
    uint32_t* currMetrics             = vbdCtrl->currMetrics;
    uint32_t* nextMetrics             = vbdCtrl->nextMetrics;

    v_state_t state;
    int32_t step;
    uint8_t bits;

    /* Clean output stream and set the metrics to the maximum except for the first state as the initial state of the
     * algorithm  */
    memset(out, 0, outBytes);
    memset(currMetrics, (int)infinity, sizeof(uint32_t) * stateCount);
    currMetrics[0] = 0;

    /* Forward processing of each step symbols */
    for (step = 0; step < stepsCount; step++) {
        memset(nextMetrics, (int)infinity, sizeof(uint32_t) * stateCount);

        const uint8_t symbols = getInputSymbols(in, step, vbdCtrl->symbolsPerStep);

        for (state = 0; state < stateCount; state++) {
            uint32_t currMetric = currMetrics[state];

            if (currMetric == infinity) {
                continue;
            }

            for (bits = 0; bits <= maxBitsValue; bits++) {

                const v_state_t nextState = getNextState(state, bits, vbdCtrl->bitsPerStep, vbdCtrl->constraintLength);
                const uint8_t outSymbols  = encodeBits(
                        state,
                        bits,
                        vbdCtrl->bitsPerStep,
                        vbdCtrl->symbolsPerStep,
                        vbdCtrl->symbolGenerators);
                const uint32_t errors = calculateError(symbols, outSymbols);

                const uint32_t newMetric = currMetric + errors;

                if (newMetric < nextMetrics[nextState]) {
                    nextMetrics[nextState]     = newMetric;
                    survivors[step][nextState] = state;
                }
            }
        }

        for (state = 0; state < stateCount; state++) {
            currMetrics[state] = nextMetrics[state];
        }
    }

    /* Search the most probable state and path at block end */
    uint32_t bestMetric = infinity;
    uint32_t bestState  = 0;

    for (state = 0; state < stateCount; state++) {
        if (currMetrics[state] < bestMetric) {
            bestMetric = currMetrics[state];
            bestState  = state;
        }
    }

    /* Traceback */
    for (step = stepsCount - 1, state = bestState; step >= 0; step--) {
        const uint32_t decodedBits = getLastBitsFromState(state, vbdCtrl->bitsPerStep);
        setOutBits(out, step, vbdCtrl->bitsPerStep, decodedBits);
        state = survivors[step][state];
    }

    return (int32_t)bestMetric;
}
