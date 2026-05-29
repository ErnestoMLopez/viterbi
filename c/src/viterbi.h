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

/**
 * @brief Maximum number of Viterbi decoder instances that can be initialized simultaneously.
 */
#define VBD_MAX_DECODERS 6

/**
 * @brief Maximum number of output symbols per step (n) supported by the decoder. This limit is needed only to determine
 * the memory asigned to store the output generators.
 */
#define VBD_MAX_SYMBOLS_PER_STEP 8


/* =======================================================================
 * [TYPEDEF]
 * =======================================================================
 */

/**
 * @brief Control structure for a Viterbi block decoder instance.
 */
typedef struct vbd_ctrl vbd_ctrl_t;

/**
 * @brief Data type for the generator polynomials used in the convolutional encoder.
 *
 * The generator polynomial is represented as a 32-bit unsigned integer, where the least significant bit (LSB)
 * corresponds to the most recent bit in the shift register. The `isInverted` flag indicates whether the branch
 * corresponding to this generator is inverted or not to generate the output symbol.
 */
typedef struct v_generator {
    uint32_t poly;
    bool isInverted;
} v_generator_t;


/* =======================================================================
 * [EXTERNAL DATA DECLARATION]
 * =======================================================================
 */

/* Ideally, this section should never be used on a public interface */

/* =======================================================================
 * [PUBLIC INTERFACE PROTOTYPES]
 * =======================================================================
 */

/**
 * @brief Initializes a Viterbi block decoder instance.
 *
 * @param vbdCtrl Pointer to a pointer that will hold the control structure for the decoder instance.
 * @param symbolsPerInput Total number of symbols that are input to the decoder for each block.
 * @param bitsPerStep Parameter k = Number of bits input to the encoder for each step.
 * @param symbolsPerStep Parameter n = Number of symbols output by the encoder for each step.
 * @param constraintLength Parameter K = Constraint length of the convolutional code.
 * @param symbolGenerators Pointer to an array of ::symbolsPerStep generator polynomials.
 * @param workingBuffer Buffer needed for the survivors path and metrics storage.
 * @param bufferSize Buffer size [bytes].
 * @return 0  = success.
 *         -1 = maximum decoders instances limit reached.
 *         -2 = too many symbols per step.
 *         -3 = not enough workspace memory.
 */
int32_t viterbiBlockDecoderInit(
        vbd_ctrl_t** vbdCtrl,
        const uint8_t symbolsPerInput,
        const uint8_t bitsPerStep,
        const uint8_t symbolsPerStep,
        const uint8_t constraintLength,
        const v_generator_t* symbolGenerators,
        const uint8_t* workingBuffer,
        const uint32_t bufferSize);

/**
 * @brief Decodes a block of symbols using the Viterbi algorithm.
 *
 * @param vbdCtrl Pointer to the control structure for the decoder instance.
 * @param in Input buffer containing the symbols to be decoded.
 * @param out Output buffer where the decoded bits will be stored.
 * @return -1 = invalid arguments.
 *        >=0 = number of bit errors detected and corrected.
 */
int32_t viterbiBlockDecoder(const vbd_ctrl_t* vbdCtrl, const uint8_t* in, uint8_t* out);

#endif
