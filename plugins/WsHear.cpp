// WsHear: WsMap that listens. Each waveset of the input is measured as it ends and its length
// is read back as a map state on the same lo..hi scale. At the next generated waveset, couple
// mixes that heard state into x before the map steps. Fed its own processed output, the orbit
// is bent by whatever moved the zero crossings on the way back.

#include "ws.hpp"

static InterfaceTable* ft;

enum { kIn, kR, kLo, kHi, kAmp, kShape, kMap, kCouple, kThresh, kSkewDepth, kAmpDepth, kInit, kBalance, kHyst, kEvery };

struct WsHear : public Unit {
    WsDetect det;
    double heardLen; // length in samples of the last input waveset; 0 when none is new
    double phase, inc;
    double x, y, prev;
    double rEff; // r after hysteresis; negative until the first waveset
    float r, lo, hi, amp, shape, map, couple, skewDepth, ampDepth, balance, hyst;
    WsClock clock;
    double peak, skew, gPos, gNeg;
};

static void WsHear_next(WsHear* unit, int inNumSamples);
static void WsHear_Ctor(WsHear* unit);

void WsHear_Ctor(WsHear* unit) {
    SETCALC(WsHear_next);
    unit->det = WsDetect();
    unit->heardLen = 0.0;
    unit->phase = 1.0;
    unit->inc = 0.0;
    mapInit(IN0(kInit), unit->x, unit->y);
    unit->prev = unit->x;
    unit->rEff = -1.0;
    unit->r = 0.9f;
    unit->lo = 60.f;
    unit->hi = 1200.f;
    unit->amp = 0.f;
    unit->shape = 0.f;
    unit->map = 0.f;
    unit->couple = 0.f;
    unit->skewDepth = 0.f;
    unit->ampDepth = 0.f;
    unit->balance = 0.f;
    unit->hyst = 0.f;
    unit->clock = WsClock();
    unit->gPos = unit->gNeg = 1.0;
    unit->peak = 0.0;
    unit->skew = 0.5;
    OUT0(0) = 0.f;
    if (unit->mNumOutputs > 1)
        OUT0(1) = 0.f;
}

void WsHear_next(WsHear* unit, int inNumSamples) {
    const float* in = IN(kIn);
    float* out = OUT(0);
    float* trig = unit->mNumOutputs > 1 ? OUT(1) : nullptr; // only with arTrig
    float thresh = IN0(kThresh);
    double sr = SAMPLERATE;
    double phase = unit->phase;
    double inc = unit->inc;

    for (int i = 0; i < inNumSamples; ++i) {
        if (detect(unit->det, in[i], thresh))
            unit->heardLen = unit->det.len;

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
            unit->couple = pull(unit, kCouple, i + 1, unit->couple);
            unit->skewDepth = pull(unit, kSkewDepth, i + 1, unit->skewDepth);
            unit->ampDepth = pull(unit, kAmpDepth, i + 1, unit->ampDepth);
            unit->balance = pull(unit, kBalance, i + 1, unit->balance);
            unit->hyst = pull(unit, kHyst, i + 1, unit->hyst);

            double lo = std::fabs(unit->lo) < 0.01 ? 0.01 : std::fabs(unit->lo);
            double hi = std::fabs(unit->hi) < 0.01 ? 0.01 : std::fabs(unit->hi);
            double span = std::log(hi / lo);
            // the inverse of x -> lo * (hi/lo)^x; each heard waveset is used once
            if (unit->heardLen > 0.0 && std::fabs(span) > 1e-9) {
                double xh = clampd(std::log(sr / unit->heardLen / lo) / span, 1e-9, 1.0 - 1e-9);
                double c = clampd(unit->couple, 0.0, 1.0);
                unit->x += c * (xh - unit->x);
                unit->heardLen = 0.0;
            }

            unit->prev = unit->x;
            unit->rEff = unit->rEff < 0.0 ? unit->r : backlash(unit->rEff, unit->r, unit->hyst);
            int m = (int)clampd(unit->map, 0.0, kNumMaps - 1);
            stepMap(m, unit->rEff, unit->x, unit->y);

            inc = incFor(lo * std::pow(hi / lo, unit->x), sr);
            unit->skew = clampd(0.5 + unit->skewDepth * mapSecond(m, unit->x, unit->y, unit->prev), 0.01, 0.99);
            unit->peak = unit->amp * (1.0 - clampd(unit->ampDepth, 0.0, 1.0) * unit->x);
            balanceGains(unit->skew, 1.0 - unit->skew, unit->balance, unit->gPos, unit->gNeg);
            if (phase >= 1.0)
                phase = 0.0;
        }

        double s = unit->skew;
        double v = phase < s ? unit->gPos * halfCycle(unit->shape, phase / s)
                             : -unit->gNeg * halfCycle(unit->shape, (phase - s) / (1.0 - s));
        out[i] = zapgremlins((float)(v * unit->peak));
        if (trig)
            trig[i] = tr;
        phase += inc;
    }

    unit->phase = phase;
    unit->inc = inc;
}

PluginLoad(WsHear) {
    ft = inTable;
    DefineSimpleUnit(WsHear);
}
