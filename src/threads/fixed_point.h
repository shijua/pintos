/* fixed-point.h -- header file for floating point implementation */
#ifndef PINTOS_7_FIXED_POINT_H
#define PINTOS_7_FIXED_POINT_H

#include "lib/stdint.h"
#include "lib/inttypes.h"

#define FLOATING_Q 14                     /* decimal part for real numbers. */
#define FLOATING_F (1 << FLOATING_Q)      /* integer part for real numbers. */
#define LAST_BIT 31                       /* last bit for real numbers.     */
#define SIGN_BIT (1 << LAST_BIT)          /* signed bit for real numbers.   */

/* using int to represent floating points. */
typedef int32_t fp;

/* Negative sign for real numbers. */
#define IS_NEGATIVE(RN) ((RN & SIGN_BIT) != 0)
#define NEGATIVE(RN) (RN ^ SIGN_BIT)

/* Absolute value for real numbers. */
#define ABS(RN) (IS_NEGATIVE(RN) ? NEGATIVE(RN) : RN)

/* Calculate timer_freq numbers. */
#define CONVERT_FREQ(RN) (RN * TIMER_FREQ)

/* Constructor for int. */
#define FP_INT_CONSTRUCT(RN) (fp_construct (RN, 1))

/* Constructs a fixed-point representation from a fraction */
static inline fp fp_construct (int numerator, int denominator) {
  return (int64_t) numerator * FLOATING_F / denominator;
}

/* Converts a fixed-point value to an integer, rounding down */
static inline int fp_rounding_down (fp real_number) {
  return real_number / FLOATING_F;
}

/* Converts a fixed-point value to an integer, rounding to nearest */
static inline int fp_rounding_near (fp real_number) {
  return (real_number >= 0) ? (real_number + FLOATING_F / 2) / FLOATING_F :
  (real_number - FLOATING_F / 2) / FLOATING_F;
}

/* Addition operations */
static inline fp fp_add (fp real1, fp real2) {
  return real1 + real2;
}

static inline fp fp_int_add (fp real_number, int int_number) {
  return real_number + int_number * FLOATING_F;
}

/* Subtraction operations */
static inline fp fp_subtract (fp real1, fp real2) {
  return real1 - real2;
}

static inline fp fp_int_subtract (fp real_number, int int_number) {
  return real_number - int_number * FLOATING_F;
}

/* Multiplication operations */
static inline fp fp_multiply (fp real1, fp real2) {
  return ((int64_t) real1 * real2) / FLOATING_F;
}


/* Division operations */
static inline fp fp_divide (fp real1, fp real2) {
  return ((int64_t) real1 * FLOATING_F) / real2;
}


#endif //PINTOS_7_FIXED_POINT_H
