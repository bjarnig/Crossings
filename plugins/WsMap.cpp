// WsMap: each waveset is computed from the previous one through an iterated map.
// A state x in [0,1] steps once per waveset; x sets the length, the change in x
// sets the skew, and x scales the peak.

#include "ws.hpp"

static InterfaceTable* ft;

enum { kR, kLo, kHi, kAmp, kShape, kMap, kSkewDepth, kAmpDepth, kInit, kBalance, kEvery };

struct WsMap : public Unit {
    double phase, inc;
    double x, prev;
    float r, lo, hi, amp, shape, map, skewDepth, ampDepth, balance;
    WsClock clock;
    double peak, skew, gPos, gNeg;
};

static void WsMap_next(WsMap* unit, int inNumSamples);
static void WsMap_Ctor(WsMap* unit);

// r in [0,1] is scaled to each map's full range, so r near 1 is chaotic for every map
static inline double iterate(int map, double r, double x) {
    switch (map) {
    case 0:
        return 4.0 * r * x * (1.0 - x); // logistic
    case 1:
        return 2.0 * (r > 0.9995 ? 0.9995 : r) * (x < 0.5 ? x : 1.0 - x); // tent; r = 1 collapses in floating point
    case 2:
        return r * std::sin(kPi * x); // sine
    default: {
        double y = x + 0.618 - (8.0 * r / (2.0 * kPi)) * std::sin(2.0 * kPi * x); // circle, K = 8r
        return y - std::floor(y);
    }
    }
}

void WsMap_Ctor(WsMap* unit) {
    SETCALC(WsMap_next);
    unit->phase = 1.0;
    unit->inc = 0.0;
    unit->x = clampd(IN0(kInit), 0.0, 1.0);
    unit->prev = unit->x;
    unit->r = 0.9f;
    unit->lo = 60.f;
    unit->hi = 1200.f;
    unit->amp = 0.f;
    unit->shape = 0.f;
    unit->map = 0.f;
    unit->skewDepth = 0.f;
    unit->ampDepth = 0.f;
    unit->balance = 0.f;
    unit->clock = WsClock();
    if (unit->mNumOutputs > 1)
        OUT0(1) = 0.f;
    unit->gPos = unit->gNeg = 1.0;
    unit->peak = 0.0;
    unit->skew = 0.5;
    OUT0(0) = 0.f;
}

void WsMap_next(WsMap* unit, int inNumSamples) {
    float* out = OUT(0);
    float* trig = unit->mNumOutputs > 1 ? OUT(1) : nullptr; // only with arTrig
    double sr = SAMPLERATE;
    double phase = unit->phase;
    double inc = unit->inc;

    for (int i = 0; i < inNumSamples; ++i) {
        float tr = 0.f;
        if (phase >= 1.0) {
            phase -= 1.0;
            tr = clockTick(unit, unit->clock, kEvery, i + 1);
            unit->r = pull(unit, kR, i + 1, unit->r);
            unit->lo = pull(unit, kLo, i + 1, unit->lo);
            unit->hi = pull(unit, kHi, i + 1, unit->hi);
            unit->amp = pull(unit, kAmp, i + 1, unit->amp);
            unit->shape = pull(unit, kShape, i + 1, unit->shape);
            unit->map = pull(unit, kMap, i + 1, unit->map);
            unit->skewDepth = pull(unit, kSkewDepth, i + 1, unit->skewDepth);
            unit->ampDepth = pull(unit, kAmpDepth, i + 1, unit->ampDepth);
            unit->balance = pull(unit, kBalance, i + 1, unit->balance);

            unit->prev = unit->x;
            int m = (int)clampd(unit->map, 0.0, 3.0);
            // nudged off the edges so a map that lands on 0 or 1 does not freeze there
            unit->x = clampd(iterate(m, clampd(unit->r, 0.0, 1.0), unit->x), 1e-9, 1.0 - 1e-9);

            double lo = std::fabs(unit->lo) < 0.01 ? 0.01 : std::fabs(unit->lo);
            double hi = std::fabs(unit->hi) < 0.01 ? 0.01 : std::fabs(unit->hi);
            inc = incFor(lo * std::pow(hi / lo, unit->x), sr);
            unit->skew = clampd(0.5 + unit->skewDepth * (unit->x - unit->prev), 0.01, 0.99);
            unit->peak = unit->amp * (1.0 - clampd(unit->ampDepth, 0.0, 1.0) * unit->x);
            balanceGains(unit->skew, 1.0 - unit->skew, unit->balance, unit->gPos, unit->gNeg);
            if (phase >= 1.0)
                phase = 0.0;
        }

        double s = unit->skew;
        double y = phase < s ? unit->gPos * halfCycle(unit->shape, phase / s)
                             : -unit->gNeg * halfCycle(unit->shape, (phase - s) / (1.0 - s));
        out[i] = zapgremlins((float)(y * unit->peak));
        if (trig)
            trig[i] = tr;
        phase += inc;
    }

    unit->phase = phase;
    unit->inc = inc;
}

PluginLoad(WsMap) {
    ft = inTable;
    DefineSimpleUnit(WsMap);
}
