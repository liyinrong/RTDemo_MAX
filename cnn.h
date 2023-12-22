/**************************************************************************************************
* Copyright (C) 2019-2021 Maxim Integrated Products, Inc. All Rights Reserved.
*
* Maxim Integrated Products, Inc. Default Copyright Notice:
* https://www.maximintegrated.com/en/aboutus/legal/copyrights.html
**************************************************************************************************/

/*
 * This header file was automatically @generated for the RTDemo_MAX_alpha network from a template.
 * Please do not edit; instead, edit the template and regenerate.
 */

#ifndef __CNN_H__
#define __CNN_H__

#include <stdint.h>
typedef int32_t q31_t;
typedef int16_t q15_t;

/* Return codes */
#define CNN_FAIL 0
#define CNN_OK 1

/*
  SUMMARY OF OPS
  Hardware: 746,496 ops (720,384 macc; 19,968 comp; 6,144 add; 0 mul; 0 bitwise)
    Layer 0 (conv1): 55,296 ops (55,296 macc; 0 comp; 0 add; 0 mul; 0 bitwise)
    Layer 1 (pool1): 3,072 ops (0 macc; 3,072 comp; 0 add; 0 mul; 0 bitwise)
    Layer 2 (convblk1_convr): 99,840 ops (98,304 macc; 1,536 comp; 0 add; 0 mul; 0 bitwise)
    Layer 3 (convblk1_conv1): 24,960 ops (24,576 macc; 384 comp; 0 add; 0 mul; 0 bitwise)
    Layer 4 (convblk1_conv2): 18,816 ops (18,432 macc; 384 comp; 0 add; 0 mul; 0 bitwise)
    Layer 5 (convblk1_conv3): 26,112 ops (24,576 macc; 1,536 comp; 0 add; 0 mul; 0 bitwise)
    Layer 6 (convblk1_resid): 1,536 ops (0 macc; 0 comp; 1,536 add; 0 mul; 0 bitwise)
    Layer 7 (convblk2_convr): 99,840 ops (98,304 macc; 1,536 comp; 0 add; 0 mul; 0 bitwise)
    Layer 8 (convblk2_conv1): 24,960 ops (24,576 macc; 384 comp; 0 add; 0 mul; 0 bitwise)
    Layer 9 (convblk2_conv2): 18,816 ops (18,432 macc; 384 comp; 0 add; 0 mul; 0 bitwise)
    Layer 10 (convblk2_conv3): 26,112 ops (24,576 macc; 1,536 comp; 0 add; 0 mul; 0 bitwise)
    Layer 11 (convblk2_resid): 1,536 ops (0 macc; 0 comp; 1,536 add; 0 mul; 0 bitwise)
    Layer 12 (convblk3_convr): 99,840 ops (98,304 macc; 1,536 comp; 0 add; 0 mul; 0 bitwise)
    Layer 13 (convblk3_conv1): 24,960 ops (24,576 macc; 384 comp; 0 add; 0 mul; 0 bitwise)
    Layer 14 (convblk3_conv2): 18,816 ops (18,432 macc; 384 comp; 0 add; 0 mul; 0 bitwise)
    Layer 15 (convblk3_conv3): 26,112 ops (24,576 macc; 1,536 comp; 0 add; 0 mul; 0 bitwise)
    Layer 16 (convblk3_resid): 1,536 ops (0 macc; 0 comp; 1,536 add; 0 mul; 0 bitwise)
    Layer 17 (convblk4_convr): 99,840 ops (98,304 macc; 1,536 comp; 0 add; 0 mul; 0 bitwise)
    Layer 18 (convblk4_conv1): 24,960 ops (24,576 macc; 384 comp; 0 add; 0 mul; 0 bitwise)
    Layer 19 (convblk4_conv2): 18,816 ops (18,432 macc; 384 comp; 0 add; 0 mul; 0 bitwise)
    Layer 20 (convblk4_conv3): 26,112 ops (24,576 macc; 1,536 comp; 0 add; 0 mul; 0 bitwise)
    Layer 21 (convblk4_resid): 1,536 ops (0 macc; 0 comp; 1,536 add; 0 mul; 0 bitwise)
    Layer 22 (pool2): 1,536 ops (0 macc; 1,536 comp; 0 add; 0 mul; 0 bitwise)
    Layer 23 (fc): 1,536 ops (1,536 macc; 0 comp; 0 add; 0 mul; 0 bitwise)

  RESOURCE USAGE
  Weight memory: 30,336 bytes out of 442,368 bytes total (6.9%)
  Bias memory:   688 bytes out of 2,048 bytes total (33.6%)
*/

/* Number of outputs for this network */
#define CNN_NUM_OUTPUTS 2

/* Use this timer to time the inference */
#define CNN_INFERENCE_TIMER MXC_TMR0

/* Port pin actions used to signal that processing is active */

#define CNN_START LED_On(1)
#define CNN_COMPLETE LED_Off(1)
#define SYS_START LED_On(0)
#define SYS_COMPLETE LED_Off(0)

/* Run software SoftMax on unloaded data */
void softmax_q17p14_q15(const q31_t * vec_in, const uint16_t dim_vec, q15_t * p_out);
/* Shift the input, then calculate SoftMax */
void softmax_shift_q17p14_q15(q31_t * vec_in, const uint16_t dim_vec, uint8_t in_shift, q15_t * p_out);

/* Stopwatch - holds the runtime when accelerator finishes */
extern volatile uint32_t cnn_time;

/* Custom memcopy routines used for weights and data */
void memcpy32(uint32_t *dst, const uint32_t *src, int n);
void memcpy32_const(uint32_t *dst, int n);

/* Enable clocks and power to accelerator, enable interrupt */
int cnn_enable(uint32_t clock_source, uint32_t clock_divider);

/* Disable clocks and power to accelerator */
int cnn_disable(void);

/* Perform minimum accelerator initialization so it can be configured */
int cnn_init(void);

/* Configure accelerator for the given network */
int cnn_configure(void);

/* Load accelerator weights */
int cnn_load_weights(void);

/* Verify accelerator weights (debug only) */
int cnn_verify_weights(void);

/* Load accelerator bias values (if needed) */
int cnn_load_bias(void);

/* Start accelerator processing */
int cnn_start(void);

/* Force stop accelerator */
int cnn_stop(void);

/* Continue accelerator after stop */
int cnn_continue(void);

/* Unload results from accelerator */
int cnn_unload(uint32_t *out_buf);

/* Turn on the boost circuit */
int cnn_boost_enable(mxc_gpio_regs_t *port, uint32_t pin);

/* Turn off the boost circuit */
int cnn_boost_disable(mxc_gpio_regs_t *port, uint32_t pin);

#endif // __CNN_H__
