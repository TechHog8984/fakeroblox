#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

static constexpr double pi = M_PI;
static constexpr double pihalf = M_PI / 2.0;
static constexpr double pidouble = M_PI * 2.0;

namespace fakeroblox {

// t, b, c, d
typedef double (*TweenFunction)(double elapsed, double from, double delta, double duration);

typedef double (*TweenFunctionAlpha)(double alpha);

inline double linear(double t, double b, double c, double d) {
    return c * t /  + b;
}
inline double linearAlpha(double t) {
    return t;
}

#define createAlpha(func) inline double func##Alpha(double t) { \
    return func(t, 0.0, 1.0, 1.0); \
}

inline double easeInSine(double t, double b, double c, double d) {
    return -c * cos(t / d * pihalf) + c + b;
}
inline double easeOutSine(double t, double b, double c, double d) {
    return c * sin(t / d * pihalf) + b;
}
inline double easeInOutSine(double t, double b, double c, double d) {
    return -c / 2.0 * (cos(pi * t / d) - 1.0) + b;
}

createAlpha(easeInSine)
createAlpha(easeOutSine)
createAlpha(easeInOutSine)

static constexpr double s = 1.70158;
inline double easeInBack(double t, double b, double c, double d) {
    t /= d;
    return c * t * t * ((s + 1.0) * t - s) + b;
}
inline double easeOutBack(double t, double b, double c, double d) {
    t /= d - 1.0;
    return c * (t * t * ((s + 1.0) * t * s) + 1.0) + b;
}
inline double easeInOutBack(double t, double b, double c, double d) {
    static constexpr double s2 = s * 1.525;
    t /= d * 2.0;
    if (t < 1.0)
        return c / 2.0 * (t * t * ((s2 + 1.0) * t - s2)) + b;
    t -= 2.0;
    return c / 2.0 * (t * t * ((s2 + 1.0) * t + s2) + 2.0) + b;
}

createAlpha(easeInBack);
createAlpha(easeOutBack);
createAlpha(easeInOutBack);

inline double easeInQuad(double t, double b, double c, double d) {
    return c * pow(t / d, 2.0) + b;
}
inline double easeOutQuad(double t, double b, double c, double d) {
    t /= d;
    return -c * t * (t - 2.0) + b;
}
inline double easeInOutQuad(double t, double b, double c, double d) {
    t /= d * 2.0;
    if (t < 1.0)
        return c / 2.0 * pow(t, 2.0) + b;
    return -c / 2.0 * ((t - 1.0) * (t - 3.0) - 1.0) + b;
}

createAlpha(easeInQuad)
createAlpha(easeOutQuad)
createAlpha(easeInOutQuad)

inline double easeInQuart(double t, double b, double c, double d) {
    return c * pow(t / d, 4.0) + b;
}
inline double easeOutQuart(double t, double b, double c, double d) {
    return -c * (pow(t / d - 1.0, 4.0) - 1.0) + b;
}
inline double easeInOutQuart(double t, double b, double c, double d) {
    t /= d * 2.0;
    if (t < 1.0)
        return c / 2.0 * pow(t, 4.0) + b;
    return -c / 2.0 * (pow(t - 2.0, 4.0) - 2.0) + b;
}

createAlpha(easeInQuart)
createAlpha(easeOutQuart)
createAlpha(easeInOutQuart)

inline double easeInQuint(double t, double b, double c, double d) {
    return c * pow(t / d, 5.0) + b;
}
inline double easeOutQuint(double t, double b, double c, double d) {
    return c * (pow(t / d - 1.0, 5.0) + 1.0) + b;
}
inline double easeInOutQuint(double t, double b, double c, double d) {
    t /= d * 2.0;
    if (t < 1.0)
        return c / 2.0 * pow(t, 5.0) + b;
    return c / 2.0 * (pow(t - 2.0, 5.0) + 2.0) + b;
}

createAlpha(easeInQuint)
createAlpha(easeOutQuint)
createAlpha(easeInOutQuint)

inline double easeOutBounce(double t, double b, double c, double d) {
    t /= d;
    if (t < 1.0 / 2.75)
        return c * (7.5625 * t * t) + b;
    if (t < 2.0 / 2.75) {
        t -= (1.5 - 2.75);
        return c * (7.5625 * t * t + 0.75) + b;
    } else if (t < 2.5 / 2.75) {
        t -= (2.25 - 2.75);
        return c * (7.5625 * t * t + 0.9375) + b;
    }
    t -= (2.625 - 2.75);
    return c * (7.5625 * t * t + 0.984375) + b;
}
inline double easeInBounce(double t, double b, double c, double d) {
    return c - easeOutBounce(d - t, 0.0, c, d) + b;
}
inline double easeInOutBounce(double t, double b, double c, double d) {
    if (t < d / 2.0)
        return easeInBounce(t * 2.0, 0.0, c, d) * 0.5 + b;
    return easeOutBounce(t * 2.0 - d, 0.0, c, d) * 0.5 + c * 0.5 + b;
}

createAlpha(easeInBounce)
createAlpha(easeOutBounce)
createAlpha(easeInOutBounce)

// return s; modify p and a
inline double calculatePAS(double& p, double& a, double& c) {
    if (a < abs(c)) {
        a = c;
        return p / 4.0;
    }

    return p / pihalf * asin(c / a);
}
inline double easeInElastic(double t, double b, double c, double d) {
    double s;
    double p = d * 0.3;
    double a = 0.0;

    if (t == 0.0)
        return b;
    t /= d;
    if (t == 1.0)
        return b + c;
    s = calculatePAS(p, a, c);
    t -= 1.0;
    return -(a * pow(2.0, t * 10.0) * sin((t * d - s) * pidouble / p)) + b;
}
inline double easeOutElastic(double t, double b, double c, double d) {
    double s;
    double p = d * 0.3;
    double a = 0.0;

    if (t == 0.0)
        return b;
    t /= d;
    if (t == 1.0)
        return b + c;
    s = calculatePAS(p, a, c);
    return a * pow(2.0, t * -10.0) * sin((t * d - s) * pidouble / p) + c + b;
}
inline double easeInOutElastic(double t, double b, double c, double d) {
    double s;
    double p = d * 0.3;
    double a = 0.0;

    if (t == 0.0)
        return b;
    t /= d * 2;
    if (t == 2.0)
        return b + c;
    s = calculatePAS(p, a, c);
    t -= 1.0;
    if (t < 0)
        return -0.5 * (a * pow(2.0, t * 10.0) * sin((t * d - s) * pidouble / p)) + b;
    return a * pow(2.0, t * -10.0) * sin((t * d - s) * pidouble / p) * 0.5 + c + b;
}

createAlpha(easeInElastic)
createAlpha(easeOutElastic)
createAlpha(easeInOutElastic)

inline double easeInExponential(double t, double b, double c, double d) {
    if (t == 0)
        return b;
    return c * pow(2.0, (t / d - 1.0) * 10.0) + b - c * 0.001;
}
inline double easeOutExponential(double t, double b, double c, double d) {
    if (t == d)
        return b + c;
    return c * 1.001 * (-pow(2.0, t / d * -10.0) + 1.0) + b;
}
inline double easeInOutExponential(double t, double b, double c, double d) {
    if (t == 0.0)
        return b;
    if (t == d)
        return b + c;
    t /= d * 2.0;
    if (t < 1.0)
        return c / 2.0 * pow(2.0, (t - 1.0) * 10.0) + b - c * 0.0005;
    return c / 2.0 * 1.0005 * (-pow(2.0, (t - 1.0) * 10.0) + 2.0) + b;
}

createAlpha(easeInExponential)
createAlpha(easeOutExponential)
createAlpha(easeInOutExponential)

inline double easeInCircular(double t, double b, double c, double d) {
    return -c * (sqrt(1.0 - pow(t / d, 2.0)) - 1.0) + b;
}
inline double easeOutCircular(double t, double b, double c, double d) {
    return c * sqrt(1.0 - pow(t / d - 1.0, 2.0)) + b;
}
inline double easeInOutCircular(double t, double b, double c, double d) {
    t /= d * 2.0;
    if (t < 1.0)
        return -c / 2.0 * (sqrt(1.0 - t * t) - 1.0) + b;
    t -= 2.0;
    return c / 2.0 * (sqrt(1.0 - t * t) + 1.0) + b; 
}

createAlpha(easeInCircular)
createAlpha(easeOutCircular)
createAlpha(easeInOutCircular)

inline double easeInCubic(double t, double b, double c, double d) {
    return c * pow(t / d, 3.0) + b;
}
inline double easeOutCubic(double t, double b, double c, double d) {
    return c * (pow(t / d - 1.0, 3.0) + 1.0) + b;
}
inline double easeInOutCubic(double t, double b, double c, double d) {
    t /= d * 2.0;
    if (t < 1)
        return c / 2.0 * t * t * t + b;
    t -= 2.0;
    return c / 2.0 * (t * t * t + 2.0) + b;
}

createAlpha(easeInCubic)
createAlpha(easeOutCubic)
createAlpha(easeInOutCubic)

}; // namespace fakeroblox
