#ifndef AXIS_MATH_H
#define AXIS_MATH_H

//#define MIN(MINA0, MINA1)               (((MINA0)<(MINA1))?(MINA0):(MINA1))
//#define MAX(MAXA0, MAXA1)               (((MAXA0)>(MAXA1))?(MAXA0):(MAXA1))
//#define ABS(ABSA0)                      (((ABSA0)<0)?(-(ABSA0)):ABSA0)
#define TWEEN_PERCENT(MIN, MAX, PER)    ((MIN) * (PER) + (MAX) * (1.0f - (PER)))

#define PI                              (3.14159265358979323846)
#define TAU                             (PI * 2.0)
#define HPI                             (PI * 0.5)

#define DEG2RAD                         (0.0174532925199433)
#define RAD2DEG                         (57.29577951308231)
#define S2RAD                           (0.0000958737992429)
#define S2DEG                           (0.0054931640625)
#define RAD2S                           (10430.37835047045)
#define DEG2S                           (182.0444444444444)

// My math could be wrong here
#define C2SEC                           (1.06666666e-8)
#define C2MSEC                          (0.00001066666)

#define STOR(RHS)                       (S2RAD * RHS)
#define STOD(RHS)                       (S2DEG * RHS)
#define RTOS(RHS)                       (RAD2S * RHS)
#define DTOS(RHS)                       (DEG2S * RHS)
#define DTOR(RHS)                       (DEG2RAD * RHS)
#define RTOD(RHS)                       (RAD2DEG * RHS)

// If you ever need to explicitly truncate these
#define STORF(RHS)                      ((f32)S2RAD * RHS)
#define STODF(RHS)                      ((f32)S2DEG * RHS)
#define RTOSF(RHS)                      ((f32)RAD2S * RHS)
#define DTOSF(RHS)                      ((f32)DEG2S * RHS)
#define DTORF(RHS)                      ((f32)DEG2RAD * RHS)
#define RTODF(RHS)                      ((f32)RAD2DEG * RHS)

#define OS_CYCLES_TO_SEC(LHS)           (((f64)(LHS)) * C2SEC)
#define OS_CYCLES_TO_MSEC(LHS)          (((f64)(LHS)) * C2MSEC)


static const f64 MATH_PI                = PI;
static const f64 MATH_TAU               = TAU;
static const f64 MATH_HPI               = HPI;
static const f32 MATHF_PI               = (f32)PI;
static const f32 MATHF_TAU              = (f32)TAU;
static const f32 MATHF_HPI              = (f32)HPI;

static const f64 MATH_DTOR              = DEG2RAD;
static const f64 MATH_RTOD              = RAD2DEG;
static const f64 MATH_STOR              = S2RAD;
static const f64 MATH_STOD              = S2DEG;
static const f64 MATH_RTOS              = RAD2S;
static const f64 MATH_DTOS              = DEG2S;
static const f32 MATHF_DTOR             = (f32)DEG2RAD;
static const f32 MATHF_RTOD             = (f32)RAD2DEG;
static const f32 MATHF_STOR             = (f32)S2RAD;
static const f32 MATHF_STOD             = (f32)S2DEG;
static const f32 MATHF_RTOS             = (f32)RAD2S;
static const f32 MATHF_DTOS             = (f32)DEG2S;

static const f64 MATH_C2SEC             = C2SEC;
static const f64 MATH_C2MSEC            = C2MSEC;
static const f32 MATHF_C2SEC            = (f32)C2SEC;
static const f32 MATHF_C2MSEC           = (f32)C2MSEC;

#define acosf                           Math_FAcosF

#endif
