// Shared waveset helpers: half-cycle shapes and per-waveset input reads.
#pragma once

#include "SC_PlugIn.h"
#include "SC_DemandUnit.h"
#include <cmath>

static const double kPi = 3.141592653589793; // portable (MSVC lacks M_PI)

// Half-cycle shapes over p in [0,1], zero at both ends except the square:
// 0 sine, 1 triangle, 2 square, 3 spike. Fractional `shape` morphs between neighbours.
static inline double shapeAt(int k, double p) {
    switch (k) {
    case 0:
        return std::sin(kPi * p);
    case 1:
        return 1.0 - std::fabs(2.0 * p - 1.0);
    case 2:
        return 1.0;
    default: {
        double s = std::sin(kPi * p);
        s *= s;
        s *= s;
        return s * s;
    }
    }
}

static inline double halfCycle(double shape, double p) {
    double sh = shape < 0.0 ? 0.0 : (shape > 3.0 ? 3.0 : shape);
    int k = (int)sh;
    double frac = sh - k;
    double a = shapeAt(k, p);
    return frac > 0.0 ? a + frac * (shapeAt(k + 1, p) - a) : a;
}

// Reads one input for the next waveset; a finished demand sequence (NaN) keeps the old value.
static inline float pull(Unit* unit, int index, int offset, float prev) {
    float x = DEMANDINPUT_A(index, offset);
    return std::isnan(x) ? prev : x;
}

static inline double clampd(double x, double lo, double hi) { return x < lo ? lo : (x > hi ? hi : x); }

// Converts a frequency to a phase increment, floored so the phase keeps moving.
static inline double incFor(double freq, double sr) { return clampd(std::fabs(freq), 0.01, sr * 0.5) / sr; }

// Folds x back into [lo,hi] at the edges, so random walks bounce instead of sticking.
static inline double mirror(double x, double lo, double hi) {
    double r = hi - lo;
    if (r <= 0.0)
        return lo;
    double m = std::fmod(x - lo, 2.0 * r);
    if (m < 0.0)
        m += 2.0 * r;
    return lo + (m > r ? 2.0 * r - m : m);
}

// Gains for the two half-cycles so they enclose equal area (no DC). Only the larger half
// is scaled down, so peaks never exceed amp; balance blends from 0 (off) to 1 (equal area).
static inline void balanceGains(double areaPos, double areaNeg, double balance, double& gPos, double& gNeg) {
    gPos = gNeg = 1.0;
    double b = clampd(balance, 0.0, 1.0);
    if (b <= 0.0 || areaPos <= 0.0 || areaNeg <= 0.0)
        return;
    if (areaPos > areaNeg)
        gPos = 1.0 - b * (1.0 - areaNeg / areaPos);
    else
        gNeg = 1.0 - b * (1.0 - areaPos / areaNeg);
}
