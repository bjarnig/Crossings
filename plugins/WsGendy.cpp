// WsGendy: dynamic stochastic synthesis bounded by zero crossings. Each half-cycle is a
// breakpoint curve whose points random-walk once per waveset; the length walks in octaves.

#include "ws.hpp"

static InterfaceTable* ft;

enum { kMinFreq, kMaxFreq, kFreqStep, kAmpStep, kKnum, kAmp, kSkew, kInterp };

static const int kMaxPts = 16;

struct WsGendy : public Unit {
    double phase, inc;
    double oct; // log2 of the current frequency
    float minFreq, maxFreq, freqStep, ampStep, knum, amp, skew, interp;
    double pos[kMaxPts], neg[kMaxPts];
    int n;
};

static void WsGendy_next(WsGendy* unit, int inNumSamples);
static void WsGendy_Ctor(WsGendy* unit);

// Curve through 0, pts[0..n-1], 0 at even spacing; interp 0 linear, 1 cosine.
static inline double curve(const double* pts, int n, double p, double interp) {
    double q = p * (n + 1);
    int j = (int)q;
    if (j > n)
        j = n;
    double f = q - j;
    double a = j == 0 ? 0.0 : pts[j - 1];
    double b = j == n ? 0.0 : pts[j];
    double w = f + interp * (0.5 - 0.5 * std::cos(kPi * f) - f);
    return a + w * (b - a);
}

void WsGendy_Ctor(WsGendy* unit) {
    SETCALC(WsGendy_next);
    RGen& rgen = *unit->mParent->mRGen;
    for (int k = 0; k < kMaxPts; ++k) {
        unit->pos[k] = rgen.frand();
        unit->neg[k] = rgen.frand();
    }
    unit->phase = 1.0;
    unit->inc = 0.0;
    unit->oct = std::log2(clampd(std::sqrt(std::fabs(IN0(kMinFreq) * IN0(kMaxFreq))), 0.01, 20000.0));
    unit->minFreq = 80.f;
    unit->maxFreq = 800.f;
    unit->freqStep = 0.f;
    unit->ampStep = 0.f;
    unit->knum = 8.f;
    unit->amp = 0.f;
    unit->skew = 0.5f;
    unit->interp = 0.f;
    unit->n = 8;
    OUT0(0) = 0.f;
}

void WsGendy_next(WsGendy* unit, int inNumSamples) {
    float* out = OUT(0);
    double sr = SAMPLERATE;
    double phase = unit->phase;
    double inc = unit->inc;
    RGen& rgen = *unit->mParent->mRGen;

    for (int i = 0; i < inNumSamples; ++i) {
        if (phase >= 1.0) {
            phase -= 1.0;
            unit->minFreq = pull(unit, kMinFreq, i + 1, unit->minFreq);
            unit->maxFreq = pull(unit, kMaxFreq, i + 1, unit->maxFreq);
            unit->freqStep = pull(unit, kFreqStep, i + 1, unit->freqStep);
            unit->ampStep = pull(unit, kAmpStep, i + 1, unit->ampStep);
            unit->knum = pull(unit, kKnum, i + 1, unit->knum);
            unit->amp = pull(unit, kAmp, i + 1, unit->amp);
            unit->skew = pull(unit, kSkew, i + 1, unit->skew);
            unit->interp = pull(unit, kInterp, i + 1, unit->interp);

            unit->n = (int)clampd(unit->knum, 1.0, kMaxPts);
            double st = std::fabs(unit->ampStep);
            for (int k = 0; k < unit->n; ++k) {
                unit->pos[k] = mirror(unit->pos[k] + st * rgen.frand2(), 0.0, 1.0);
                unit->neg[k] = mirror(unit->neg[k] + st * rgen.frand2(), 0.0, 1.0);
            }
            double lo = std::log2(clampd(std::fabs(unit->minFreq), 0.01, sr * 0.5));
            double hi = std::log2(clampd(std::fabs(unit->maxFreq), 0.01, sr * 0.5));
            if (lo > hi)
                std::swap(lo, hi);
            unit->oct = mirror(unit->oct + std::fabs(unit->freqStep) * rgen.frand2(), lo, hi);
            inc = incFor(std::exp2(unit->oct), sr);
            if (phase >= 1.0)
                phase = 0.0;
        }

        double s = clampd(unit->skew, 0.01, 0.99);
        double it = clampd(unit->interp, 0.0, 1.0);
        double y = phase < s ? curve(unit->pos, unit->n, phase / s, it)
                             : -curve(unit->neg, unit->n, (phase - s) / (1.0 - s), it);
        out[i] = zapgremlins((float)(y * unit->amp));
        phase += inc;
    }

    unit->phase = phase;
    unit->inc = inc;
}

PluginLoad(WsGendy) {
    ft = inTable;
    DefineSimpleUnit(WsGendy);
}
