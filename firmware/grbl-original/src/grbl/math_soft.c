/*******************************************************************************
 * Math operations implemented in software (not hardware)
 *******************************************************************************/

#include "math_soft.h"

double trunc(double x) {
  return (x > 0) ? (double)(int)x : (x < 0) ? (double)(int)x
                                            : 0.0;
}

float truncf(float x) {
  return (x > 0) ? (float)(int)x : (x < 0) ? (float)(int)x
                                           : 0.0;
}

double ceil(double x) {
  int xi = (int)x;
  if (x > 0 && x != (double)xi)
    return (double)(xi + 1);
  return (double)xi;
}

float ceilf(float x) {
  int xi = (int)x;
  if (x > 0.0f && x != (float)xi)
    return (float)(xi + 1);
  return (float)xi;
}

double sqrt(double x) {
  if (x <= 0.0)
    return 0.0;

  double guess = x * 0.5;
  double epsilon = 1e-10;  // Tolerance for precision
  double diff;

  do {
    double new_guess = 0.5 * (guess + x / guess);
    diff = new_guess - guess;
    guess = new_guess;
  } while (diff > epsilon || diff < -epsilon);

  return guess;
}

float sqrtf(float x) {
  if (x <= 0.0f)
    return 0.0f;

  float guess = x * 0.5f;
  float epsilon = 1e-5f;  // Less strict for float
  float diff;

  do {
    float new_guess = 0.5f * (guess + x / guess);
    diff = new_guess - guess;
    guess = new_guess;
  } while (diff > epsilon || diff < -epsilon);

  return guess;
}

double round(double x) {
  if (x >= 0.0)
    return (double)((int)(x + 0.5));
  else
    return (double)((int)(x - 0.5));
}

float roundf(float x) {
  if (x >= 0.0f)
    return (float)((int)(x + 0.5f));
  else
    return (float)((int)(x - 0.5f));
}

double fabs(double x) {
  return (x < 0.0) ? -x : x;
}

float fabsf(float x) {
  return (x < 0.0f) ? -x : x;
}

long lround(double x) {
  if (x >= 0.0)
    return (long)(x + 0.5);
  else
    return (long)(x - 0.5);
}

long lroundf(float x) {
  if (x >= 0.0f)
    return (long)(x + 0.5f);
  else
    return (long)(x - 0.5f);
}

double floor(double x) {
  long i = (long)x;
  return (x < 0.0 && x != (double)i) ? (double)(i - 1) : (double)i;
}

float floorf(float x) {
  long i = (long)x;
  return (x < 0.0f && x != (float)i) ? (float)(i - 1) : (float)i;
}

static double wrap_angle(double x) {
  while (x > M_PI) x -= M_TWO_PI;
  while (x < -M_PI) x += M_TWO_PI;
  return x;
}

double sin(double x) {
  x = wrap_angle(x);
  double x2 = x * x;
  return x * (1.0 - x2 / 6.0 + x2 * x2 / 120.0 - x2 * x2 * x2 / 5040.0);
}

float sinf(float x) {
  x = (float)wrap_angle(x);
  float x2 = x * x;
  return x * (1.0f - x2 / 6.0f + x2 * x2 / 120.0f - x2 * x2 * x2 / 5040.0f);
}

double cos(double x) {
  return sin(x + M_PI_2);
}

double cos_approx(double x) {
  x = wrap_angle(x);
  double x2 = x * x;
  return 1.0 - x2 / 2.0 + x2 * x2 / 24.0 - x2 * x2 * x2 / 720.0;
}

double atan(double z) {
  // Polynomial approximation on [-1, 1]
  const double a = 0.9998660;
  const double b = -0.3302995;
  const double c = 0.1801410;
  const double d = -0.0851330;
  const double e = 0.0208351;

  double z2 = z * z;
  return (((((e * z2 + d) * z2 + c) * z2 + b) * z2 + a) * z);
}

double atan2(double y, double x) {
  if (x > 0.0)
    return atan(y / x);
  else if (x < 0.0) {
    if (y >= 0.0)
      return atan(y / x) + M_PI;
    else
      return atan(y / x) - M_PI;
  } else {  // x == 0
    if (y > 0.0)
      return M_PI_2;
    else if (y < 0.0)
      return -M_PI_2;
    else
      return 0.0;  // undefined, return 0
  }
}

float atanf(float z) {
  // Polynomial approximation on [-1, 1]
  const float a = 0.999866f;
  const float b = -0.3302995f;
  const float c = 0.1801410f;
  const float d = -0.0851330f;
  const float e = 0.0208351f;

  float z2 = z * z;
  return (((((e * z2 + d) * z2 + c) * z2 + b) * z2 + a) * z);
}

float atan2f(float y, float x) {
  if (x > 0.0f)
    return atanf(y / x);
  else if (x < 0.0f) {
    if (y >= 0.0f)
      return atanf(y / x) + M_PIf;
    else
      return atanf(y / x) - M_PIf;
  } else {  // x == 0
    if (y > 0.0f)
      return M_PI_2f;
    else if (y < 0.0f)
      return -M_PI_2f;
    else
      return 0.0f;  // undefined
  }
}
