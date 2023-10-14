/* fixed-point.h -- header file for floating point implementation */
#ifndef PINTOS_7_FIXED_POINT_H
#define PINTOS_7_FIXED_POINT_H

#import "lib/stdint.h"
#import "lib/inttypes.h"

#define FLOATING_Q 14                     /* decimal part for real numbers */
#define FLOATING_F (1 << FLOATING_Q)      /* integer part for real numbers */

/* using int represent floating points */
typedef int32_t fp;

/* basic function for floating points */
static fp fp_construct (int numerator, int denominator) {
  return (numerator * FLOATING_F) / denominator;
}

static int fp_rounding_down (fp real_number) {
  return real_number / FLOATING_F;
}

static int fp_rounding_near (fp real_number) {
  return (real_number >= 0) ? (real_number + FLOATING_F / 2) / FLOATING_F :
         (real_number - FLOATING_F / 2) / FLOATING_F;
}

/* operator for floating point numbers addition and subtraction */
static fp fp_add (fp real_number, int int_number) {
  return real_number + int_number * FLOATING_F;
}

static fp fp_subtract (fp real_number, int int_number) {
  return real_number - int_number * FLOATING_F;
}

/* operator for floating point multiplication and division      */
static fp fp_multiply (fp real1, fp real2) {
  return ((int64_t) real1) * real2 / FLOATING_F;
}

static fp fp_divide (fp real1, fp real2) {
  return ((int64_t) real1) * FLOATING_F / real2;
}

#endif //PINTOS_7_FIXED_POINT_H
