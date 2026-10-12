// WsGen: synthesizes a stream of wavesets. Each waveset is a positive and a negative
// half-cycle whose length, peak, shape and zero-crossing position are read once per
// waveset, so demand-rate inputs (Dbrown, Dseq, ...) advance one value per waveset.

#include "SC_PlugIn.h"
#include "SC_DemandUnit.h"
#include <cmath>

static InterfaceTable* ft;

static const double kPi = 3.141592653589793; // portable (MSVC lacks M_PI)

enum { kFreq, kAmp, kShape, kSkew, kRepeats };

struct WsGen : public Unit {
    double phase; // 0..1 across the current waveset
    double inc;
    float amp, shape, skew;
    int repLeft;
};

static void WsGen_next(WsGen* unit, int inNumSamples);
static void WsGen_Ctor(WsGen* unit);

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

static inline double halfCycle(float shape, double p) {
    double sh = shape < 0.f ? 0.0 : (shape > 3.f ? 3.0 : (double)shape);
    int k = (int)sh;
    double frac = sh - k;
    double a = shapeAt(k, p);
    return frac > 0.0 ? a + frac * (shapeAt(k + 1, p) - a) : a;
}

// Reads one input for the next waveset; a finished demand sequence (NaN) keeps the old value.
static inline float pull(WsGen* unit, int index, int offset, float prev) {
    float x = DEMANDINPUT_A(index, offset);
    return std::isnan(x) ? prev : x;
}

void WsGen_Ctor(WsGen* unit) {
    SETCALC(WsGen_next);
    unit->phase = 1.0; // forces a draw on the first sample
    unit->inc = 0.0;
    unit->amp = 0.f;
    unit->shape = 0.f;
    unit->skew = 0.5f;
    unit->repLeft = 0;
    OUT0(0) = 0.f;
}

void WsGen_next(WsGen* unit, int inNumSamples) {
    float* out = OUT(0);
    double sr = SAMPLERATE;
    double phase = unit->phase;
    double inc = unit->inc;
    double freq = inc * sr;

    for (int i = 0; i < inNumSamples; ++i) {
        if (phase >= 1.0) {
            phase -= 1.0;
            if (unit->repLeft <= 0) {
                freq = pull(unit, kFreq, i + 1, (float)freq);
                unit->amp = pull(unit, kAmp, i + 1, unit->amp);
                unit->shape = pull(unit, kShape, i + 1, unit->shape);
                unit->skew = pull(unit, kSkew, i + 1, unit->skew);
                float reps = pull(unit, kRepeats, i + 1, 1.f);
                unit->repLeft = reps < 1.f ? 1 : (int)reps;
                // a waveset needs at least two samples; 0.01 Hz floor keeps the phase moving
                double f = std::fabs(freq);
                inc = (f < 0.01 ? 0.01 : (f > sr * 0.5 ? sr * 0.5 : f)) / sr;
            }
            unit->repLeft--;
            if (phase >= 1.0)
                phase = 0.0;
        }

        double s = unit->skew < 0.01f ? 0.01 : (unit->skew > 0.99f ? 0.99 : (double)unit->skew);
        double y = phase < s ? halfCycle(unit->shape, phase / s)
                             : -halfCycle(unit->shape, (phase - s) / (1.0 - s));
        out[i] = (float)(y * unit->amp);
        phase += inc;
    }

    unit->phase = phase;
    unit->inc = inc;
}

PluginLoad(WsGen) {
    ft = inTable;
    DefineSimpleUnit(WsGen);
}
