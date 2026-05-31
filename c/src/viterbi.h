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
#define VBD_MAX_DECODERS 2


/* =======================================================================
 * [TYPEDEF]
 * =======================================================================
 */

/**
 * @brief Control structure for a Viterbi block decoder instance.
 */
typedef struct vbd_ctrl vbd_ctrl_t;

/**
 * @brief Data type to store the state of the Viterbi decoder.
 *
 * The state corresponds to the content of the shift register not including the most recent bits. The user can select
 * the data type used in order to reduce the memory needed for the survivors path storage. The maximum number of states
 * is 2^((K-1)*k), where K is the constraint length and k is the number of bits input to the encoder at each step. E.g.
 * for K=7 and k=1, the maximum number of states is 64, so a uint8_t can be used to store the state. For K=7 and k=2,
 * the maximum number of states is 4096, so a uint16_t can be used.
 *
 */
typedef uint8_t v_state_t;

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
 * @return  0 = success.
 *         -1 = invalid pointer arguments.
 *         -2 = maximum decoders instances limit reached.
 *         -3 = too many symbols per step.
 *         -4 = not enough workspace memory.
 *         -5 = too many states for the selected data type for v_state_t.
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
 * @brief Frees a Viterbi block decoder instance.
 *
 * @param vbdCtrl Pointer to the control structure for the decoder instance to be freed.
 * @return 0  = success.
 *         -1 = invalid argument.
 */
int32_t viterbiBlockDecoderFree(vbd_ctrl_t* vbdCtrl);

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
