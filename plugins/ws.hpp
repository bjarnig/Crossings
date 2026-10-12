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

// Counts wavesets for the trigger output: fires on the first waveset and then on every
// `every`-th one; `every` is read only when it fires, so demand inputs step per trigger.
struct WsClock {
    int count = 0;
    float every = 1.f;
};

static inline float clockTick(Unit* unit, WsClock& c, int everyIndex, int offset) {
    if (--c.count > 0)
        return 0.f;
    c.every = pull(unit, everyIndex, offset, c.every);
    c.count = c.every < 1.f ? 1 : (int)c.every;
    return 1.f;
}

// Iterated maps shared by WsMap and Dmap. x and y are kept normalised to [0,1] so the map
// can change between steps; 2D maps (standard, Ikeda) convert to their own coordinates.
// r in [0,1] is scaled to each map's useful range, so r near 1 is chaotic for every map.
enum { kLogistic, kTent, kSine, kCircle, kStandard, kIkeda, kNumMaps };

static const double kTau = 2.0 * kPi;
static const double kIkX0 = -0.5, kIkX1 = 1.9, kIkY0 = -2.4, kIkY1 = 1.0; // measured Ikeda bounds for u <= 0.9

static inline bool is2D(int map) { return map >= kStandard; }

static inline double wrap01(double v) { return v - std::floor(v); }

static inline void mapInit(double init, double& x, double& y) {
    x = clampd(init, 0.0, 1.0);
    y = (0.0 - kIkY0) / (kIkY1 - kIkY0); // raw y = 0 keeps Ikeda in its bounded basin
}

static inline void stepMap(int map, double r, double& x, double& y) {
    r = clampd(r, 0.0, 1.0);
    switch (map) {
    case kLogistic:
        x = 4.0 * r * x * (1.0 - x);
        break;
    case kTent: // r = 1 collapses in floating point
        x = 2.0 * (r > 0.9995 ? 0.9995 : r) * (x < 0.5 ? x : 1.0 - x);
        break;
    case kSine:
        x = r * std::sin(kPi * x);
        break;
    case kCircle: // K = 8r
        x = wrap01(x + 0.618 - (8.0 * r / kTau) * std::sin(kTau * x));
        break;
    case kStandard: { // x angle, y momentum, both over one turn; K = 6r
        double p = wrap01(y + (6.0 * r / kTau) * std::sin(kTau * x));
        x = wrap01(x + p);
        y = p;
        break;
    }
    default: { // Ikeda, u = 0.9r; an orbit that escapes to the far attractor restarts at the origin
        double a = kIkX0 + x * (kIkX1 - kIkX0), b = kIkY0 + y * (kIkY1 - kIkY0);
        double u = 0.9 * r, t = 0.4 - 6.0 / (1.0 + a * a + b * b);
        double a2 = 1.0 + u * (a * std::cos(t) - b * std::sin(t));
        double b2 = u * (a * std::sin(t) + b * std::cos(t));
        if (a2 < kIkX0 || a2 > kIkX1 || b2 < kIkY0 || b2 > kIkY1)
            a2 = b2 = 0.0;
        x = (a2 - kIkX0) / (kIkX1 - kIkX0);
        y = (b2 - kIkY0) / (kIkY1 - kIkY0);
        break;
    }
    }
    // nudged off the edges so a map that lands on 0 or 1 does not freeze there
    x = clampd(x, 1e-9, 1.0 - 1e-9);
    y = clampd(y, 0.0, 1.0);
}

// Second stream in [-1,1]: y for 2D maps, the change in x for 1D maps.
static inline double mapSecond(int map, double x, double y, double prevX) {
    return is2D(map) ? 2.0 * y - 1.0 : x - prevX;
}

// Backlash hysteresis: the effective r only moves when the input pushes past a dead band of
// half-width h, so sweeping up and back down leaves it in different places.
static inline double backlash(double rEff, double r, double h) {
    h = std::fabs(h);
    return clampd(rEff, r - h, r + h);
}
