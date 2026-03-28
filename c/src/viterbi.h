/**
 * \file viterbi.h
 *
 * \brief Public interface for the C implementation of a Viterbi decoder.
 */

#ifndef VITERBI_H_
#define VITERBI_H_

/* =======================================================================
 * [INCLUDES]
 * =======================================================================
 */

#include <stdbool.h>
#include <stdint.h>


/* =======================================================================
 * [MACROS]
 * =======================================================================
 */

#define VSD_IN_BYTES          30
#define VSD_OUT_BYTES         15
#define VSD_IN_SYMBOLS        240
#define VSD_OUT_BITS          120
#define VSD_CONSTRAINT_LENGTH 7
#define VSD_POLY_G1           0117  // Equivalente a 0171 en convención MSB más reciente
#define VSD_POLY_G2           0155  // Equivalente a 0133 en convención MSB más reciente
#define VSD_INVERT_G1         0
#define VSD_INVERT_G2         1

#define VGBD_MAX_DECODERS         3
#define VGBD_MAX_SYMBOLS_PER_STEP 8


/* =======================================================================
 * [TYPEDEF]
 * =======================================================================
 */

typedef struct vgd_generator {
    uint32_t poly;    //< Generator polynomial (LSB is the most recent bit)
    bool isInverted;  //< Indicates if the output branch is inverted
} vgd_generator_t;

typedef void* vgbd_ctx_t;

/* =======================================================================
 * [EXTERNAL DATA DECLARATION]
 * =======================================================================
 */

/* Ideally, this section should never be used on a public interface */

/* =======================================================================
 * [PUBLIC INTERFACE PROTOTYPES]
 * =======================================================================
 */

uint32_t viterbiStaticDecoder(const uint8_t in[VSD_IN_BYTES], uint8_t out[VSD_OUT_BYTES]);

int32_t viterbiGenericBlockDecoderInit(
        vgbd_ctx_t* vgbdCtx,
        const uint8_t symbolsPerInput,
        const uint8_t bitsPerStep,
        const uint8_t symbolsPerStep,
        const uint8_t constraintLength,
        const vgd_generator_t* symbolGenerators,
        const uint8_t* workingBuffer,
        const uint32_t bufferSize);

int32_t viterbiGenericBlockDecoder(const vgbd_ctx_t vgbdCtx, const uint8_t* in, uint8_t* out);

#endif
