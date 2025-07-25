/*******************************************************************************
 * Math operations implemented in software (not hardware)
 *******************************************************************************/

#ifndef __MATH_SOFT_H__
#define __MATH_SOFT_H__

#define M_PI 3.14159265358979323846
#define M_TWO_PI (2.0 * M_PI)
#define M_PI_2 (M_PI / 2.0)
#define M_PI_4 (M_PI / 4.0)
#define M_3PI_4 (3.0 * M_PI / 4.0)

#define M_PIf 3.1415927f
#define M_PI_2f (M_PIf / 2.0f)

double trunc(double x);

float truncf(float x);

double ceil(double x);

float ceilf(float x);

double sqrt(double x);

float sqrtf(float x);

double round(double x);

float roundf(float x);

double fabs(double x);

float fabsf(float x);

long lround(double x);

long lroundf(float x);

double floor(double x);

float floorf(float x);

double sin(double x);

float sinf(float x);

double cos(double x);

double cos_approx(double x);

double atan(double z);

double atan2(double y, double x);

float atan2f(float y, float x);

#endif  // __MATH_SOFT_H__