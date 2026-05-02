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

#define VGBD_MAX_DECODERS         6
#define VGBD_MAX_SYMBOLS_PER_STEP 8


/* =======================================================================
 * [TYPEDEF]
 * =======================================================================
 */

typedef struct vgd_generator {
    uint32_t poly;    //< Generator polynomial (LSB is the most recent bit)
    bool isInverted;  //< Indicates if the output branch is inverted
} vgd_generator_t;

typedef struct vgbd_ctrl vgbd_ctrl_t;


/* =======================================================================
 * [EXTERNAL DATA DECLARATION]
 * =======================================================================
 */

/* Ideally, this section should never be used on a public interface */

/* =======================================================================
 * [PUBLIC INTERFACE PROTOTYPES]
 * =======================================================================
 */

int32_t viterbiGenericBlockDecoderInit(
        vgbd_ctrl_t** vgbdCtrl,
        const uint8_t symbolsPerInput,
        const uint8_t bitsPerStep,
        const uint8_t symbolsPerStep,
        const uint8_t constraintLength,
        const vgd_generator_t* symbolGenerators,
        const uint8_t* workingBuffer,
        const uint32_t bufferSize);

int32_t viterbiGenericBlockDecoder(const vgbd_ctrl_t* vgbdCtrl, const uint8_t* in, uint8_t* out);

#endif
