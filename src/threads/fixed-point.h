/* fixed-point.h -- header file for floating point implementation */
#ifndef PINTOS_7_FIXED_POINT_H
#define PINTOS_7_FIXED_POINT_H

#include "lib/stdint.h"
#include "lib/inttypes.h"

#define FLOATING_Q 14                     /* decimal part for real numbers */
#define FLOATING_F (1 << FLOATING_Q)      /* integer part for real numbers */
#define LAST_BIT 31                       /* last bit for real numbers     */
#define SIGN_BIT (1 << LAST_BIT)          /* signed bit for real numbers   */
#define TIMER_FREQ 100                    /* timer interrupts occurrence   */

/* Using int represent floating points */
typedef int32_t fp;

/* Negativity for real numbers         */
#define IS_NEGATIVE(rn) ((rn & SIGN_BIT) != 0)
#define NEGATIVE(rn) (rn ^ SIGN_BIT)

/* Absolute value for real numbers     */
#define ABS(rn) (IS_NEGATIVE(rn) ? NEGATIVE(rn) : rn)

/* Calculate */
#define CONVERT_FREQ(rn) (rn * TIMER_FREQ)

/* Basic function for floating points  */
static inline fp
fp_fraction_construct (int numerator, int denominator) {
  return ((int64_t) numerator * FLOATING_F) / denominator;
}

static inline fp
fp_int_construct (int int_number) {
  return int_number * FLOATING_F;
}

/* Mind that the return value for down and near is in int not fp. */
static inline int
fp_rounding_down (fp real_number) {
  return real_number / FLOATING_F;
  // return value - (IS_NEGATIVE(real_number) ? 1 : 0);
}

static inline int
fp_rounding_near (fp real_number) {
  return (real_number >= 0) ? (real_number + FLOATING_F / 2) / FLOATING_F :
         (real_number - FLOATING_F / 2) / FLOATING_F;
}

/* operator for floating point numbers addition and subtraction */
static inline fp
fp_add (fp real_number, int int_number) {
  return real_number + int_number * FLOATING_F;
}

static inline fp
fp_subtract (fp real_number, int int_number) {
  return real_number - int_number * FLOATING_F;
}

/* operator for floating point multiplication and division      */
static inline fp
fp_multiply (fp real1, fp real2) {
  return ((int64_t) real1) * real2 / FLOATING_F;
}

static inline fp
fp_divide (fp real1, fp real2) {
  return ((int64_t) real1) * FLOATING_F / real2;
}

#endif //PINTOS_7_FIXED_POINT_H
