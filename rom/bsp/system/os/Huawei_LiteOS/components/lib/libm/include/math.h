/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * from: @(#)fdlibm.h 5.1 93/09/24
 * $FreeBSD$
 */

#ifndef _MATH_H_
#define _MATH_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#ifndef CONFIG_HAVE_DOUBLE
#define CONFIG_HAVE_DOUBLE 1
#endif

#ifndef CONFIG_HAVE_LONG_DOUBLE
#define CONFIG_HAVE_LONG_DOUBLE 1
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* General Constants ********************************************************/

#define INFINITY    (1.0/0.0)
#define NAN         (0.0/0.0)
#define HUGE_VAL    INFINITY
#ifndef volatile
#define volatile    __volatile__
#endif

/* Symbolic constants to classify floating point numbers. */
#define FP_INFINITE     0x01
#define FP_NAN          0x02
#define FP_NORMAL       0x04
#define FP_SUBNORMAL    0x08
#define FP_ZERO         0x10


#define fpclassify(x) \
    ((sizeof (x) == sizeof (float)) ? __fpclassifyf(x) \
    : (sizeof (x) == sizeof (double)) ? __fpclassifyd(x) \
    : __fpclassifyl(x))

#define isfinite(x) \
    ((sizeof (x) == sizeof (float)) ? __isfinitef(x)\
    : (sizeof (x) == sizeof (double)) ? __isfinite(x)\
    : __isfinitel(x))

#define isinf(x) \
    ((sizeof (x) == sizeof (float)) ? __isinff(x)\
    : (sizeof (x) == sizeof (double)) ? isinf(x)\
    : __isinfl(x))

#define isnan(x) \
    ((sizeof (x) == sizeof (float)) ? __isnanf(x)\
    : (sizeof (x) == sizeof (double)) ? isnan(x)\
    : __isnanl(x))

#define isnormal(x) \
    ((sizeof (x) == sizeof (float)) ? __isnormalf(x)\
    : (sizeof (x) == sizeof (double)) ? __isnormal(x)\
    : __isnormall(x))

/* Exponential and Logarithmic constants ************************************/

#define M_E        2.7182818284590452353602874713526625
#define M_SQRT2    1.4142135623730950488016887242096981
#define M_SQRT1_2  0.7071067811865475244008443621048490
#define M_LOG2E    1.4426950408889634073599246810018921
#define M_LOG10E   0.4342944819032518276511289189166051
#define M_LN2      0.6931471805599453094172321214581765
#define M_LN10     2.3025850929940456840179914546843642

/* Trigonometric Constants **************************************************/

#define M_PI       3.1415926535897932384626433832795029
#define M_PI_2     1.5707963267948966192313216916397514
#define M_PI_4     0.7853981633974483096156608458198757
#define M_1_PI     0.3183098861837906715377675267450287
#define M_2_PI     0.6366197723675813430755350534900574
#define M_2_SQRTPI 1.1283791670955125738961589031215452

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* General Functions ********************************************************/

float       ceilf (float x);
#if CONFIG_HAVE_DOUBLE
double      ceil  (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double ceill (long double x);
#endif

float       floorf(float x);
#if CONFIG_HAVE_DOUBLE
double      floor (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double floorl(long double x);
#endif

float       roundf(float x);
#if CONFIG_HAVE_DOUBLE
double      round (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double roundl(long double x);
#endif

float       rintf(float x);      /* Not implemented */
#if CONFIG_HAVE_DOUBLE
double      rint(double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double rintl(long double x); /* Not implemented */
#endif
long long llrint(double x);

float       fabsf (float x);
#if CONFIG_HAVE_DOUBLE
double      fabs  (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double fabsl (long double x);
#endif

float       modff (float x, float *iptr);
#if CONFIG_HAVE_DOUBLE
double      modf  (double x, double *iptr);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double modfl (long double x, long double *iptr);
#endif

float       fmodf (float x, float div);
#if CONFIG_HAVE_DOUBLE
double      fmod  (double x, double div);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double fmodl (long double x, long double div);
#endif

/* Exponential and Logarithmic Functions ************************************/

float       powf  (float b, float e);
#if CONFIG_HAVE_DOUBLE
double      pow   (double b, double e);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double powl  (long double b, long double e);
#endif

float       expf  (float x);
double        exp2(double);
float expm1f(float x);
#if CONFIG_HAVE_DOUBLE
double      exp   (double x);
#define expm1(x) (exp(x) - 1.0)
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double expl  (long double x);
#define expm1l(x) (expl(x) - 1.0)
#endif

float       logf  (float x);
#if CONFIG_HAVE_DOUBLE
double      log   (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double logl  (long double x);
#endif

float       log10f(float x);
#if CONFIG_HAVE_DOUBLE
double      log10 (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double log10l(long double x);
#endif

float       log2f (float x);
#if CONFIG_HAVE_DOUBLE
double      log2  (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double log2l (long double x);
#endif

float       sqrtf (float x);
#if CONFIG_HAVE_DOUBLE
double      sqrt  (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double sqrtl (long double x);
#endif

float       ldexpf(float x, int n);
#if CONFIG_HAVE_DOUBLE
double      ldexp (double x, int n);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double ldexpl(long double x, int n);
#endif

float       frexpf(float x, int *exp);
#if CONFIG_HAVE_DOUBLE
double      frexp (double x, int *exp);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double frexpl(long double x, int *exp);
#endif

/* Trigonometric Functions **************************************************/

float       sinf  (float x);
#if CONFIG_HAVE_DOUBLE
double      sin   (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double sinl  (long double x);
#endif

float       cosf  (float x);
#if CONFIG_HAVE_DOUBLE
double      cos   (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double cosl  (long double x);
#endif

float       tanf  (float x);
#if CONFIG_HAVE_DOUBLE
double      tan   (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double tanl  (long double x);
#endif

float       asinf (float x);
#if CONFIG_HAVE_DOUBLE
double      asin  (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double asinl (long double x);
#endif

float       acosf (float x);
#if CONFIG_HAVE_DOUBLE
double      acos  (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double acosl (long double x);
#endif

float       atanf (float x);
#if CONFIG_HAVE_DOUBLE
double      atan  (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double atanl (long double x);
#endif

float       atan2f(float y, float x);
#if CONFIG_HAVE_DOUBLE
double      atan2 (double y, double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double atan2l(long double y, long double x);
#endif

float       sinhf (float x);
#if CONFIG_HAVE_DOUBLE
double      sinh  (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double sinhl (long double x);
#endif

float       coshf (float x);
#if CONFIG_HAVE_DOUBLE
double      cosh  (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double coshl (long double x);
#endif

float       tanhf (float x);
#if CONFIG_HAVE_DOUBLE
double      tanh  (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double tanhl (long double x);
#endif

float       asinhf (float x);
#if CONFIG_HAVE_DOUBLE
double      asinh  (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double asinhl (long double x);
#endif

float       acoshf (float x);
#if CONFIG_HAVE_DOUBLE
double      acosh  (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double acoshl (long double x);
#endif

float       atanhf (float x);
#if CONFIG_HAVE_DOUBLE
double      atanh  (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double atanhl (long double x);
#endif

float       erff (float x);
//#define     erfcf(x) (1 - erff(x))
float   erfcf(float x);
#if CONFIG_HAVE_DOUBLE
double      erf  (double x);
//#define     erfc(x) (1 - erf(x))
double  erfc(double );
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double erfl (long double x);
//#define     erfcl(x) (1 - erfl(x))
long double erfcl(long double x);

#endif

float       copysignf (float x, float y);
#if CONFIG_HAVE_DOUBLE
double      copysign  (double x, double y);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double copysignl (long double x, long double y);
#endif

float       truncf (float x);
#if CONFIG_HAVE_DOUBLE
double      trunc (double x);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
long double truncl (long double x);
#endif

#define nanf(x) ((float)(NAN))
#ifdef CONFIG_HAVE_DOUBLE
#define nan(x) ((double)(NAN))
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
#define nanl(x) ((long double)(NAN))
#endif

/*
 * Most of these functions depend on the rounding mode and have the side
 * effect of raising floating-point exceptions, so they are not declared
 * as __pure2.  In C99, FENV_ACCESS affects the purity of these functions.
 */

/*
 * ANSI/POSIX
 */
int __fpclassifyd(double);
int __fpclassifyf(float);
int __fpclassifyl(long double);
int __isfinitef(float);
int __isfinite(double);
int __isfinitel(long double);
int __isinf(double);
int __isinff(float);
int __isinfl(long double);
int __isnanf(float);
int __isnanl(long double);
int __isnormalf(float);
int __isnormal(double);
int __isnormall(long double);

int (isinf)(double);
int (isnan)(double);
int finite(double x);

double hypot(double x, double y);
double scalbn(double x, int n);
float   scalbnf (float x, int n);

double cbrt(double x);
float cbrtf(float x);
long double cbrtl(long double x);

double  log1p(double x);
float log1pf(float x);

//#if defined(__USE_GNU)
void sincos(double, double*, double*);
void sincosf(float, float*, float*);
void sincosl(long double, long double*, long double*);
//#endif /* __USE_GNU */

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

//#endif /* CONFIG_LIBM */
#endif /* __INCLUDE_NUTTX_MATH_H */
