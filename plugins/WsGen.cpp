// WsGen: synthesizes a stream of wavesets. Each waveset is a positive and a negative
// half-cycle whose length, peak, shape and zero-crossing position are read once per
// waveset, so demand-rate inputs (Dbrown, Dseq, ...) advance one value per waveset.

#include "ws.hpp"

static InterfaceTable* ft;

enum { kFreq, kAmp, kShape, kSkew, kRepeats, kGlide, kBalance, kEvery };

struct WsGen : public Unit {
    double phase; // 0..1 across the current waveset
    // waveset parameters: p* where the waveset started from, c* where it is heading
    double pFreq, pAmp, pShape, pSkew;
    double cFreq, cAmp, cShape, cSkew;
    float glide, balance;
    WsClock clock;
    int repLeft;
    bool started;
};

static void WsGen_next(WsGen* unit, int inNumSamples);
static void WsGen_Ctor(WsGen* unit);

void WsGen_Ctor(WsGen* unit) {
    SETCALC(WsGen_next);
    unit->phase = 1.0; // forces a draw on the first sample
    unit->pFreq = unit->cFreq = 0.0;
    unit->pAmp = unit->cAmp = 0.0;
    unit->pShape = unit->cShape = 0.0;
    unit->pSkew = unit->cSkew = 0.5;
    unit->glide = 0.f;
    unit->balance = 0.f;
    unit->clock = WsClock();
    if (unit->mNumOutputs > 1)
        OUT0(1) = 0.f;
    unit->repLeft = 0;
    unit->started = false;
    OUT0(0) = 0.f;
}

void WsGen_next(WsGen* unit, int inNumSamples) {
    float* out = OUT(0);
    float* trig = unit->mNumOutputs > 1 ? OUT(1) : nullptr; // only with arTrig
    double sr = SAMPLERATE;
    double phase = unit->phase;

    for (int i = 0; i < inNumSamples; ++i) {
        float tr = 0.f;
        if (phase >= 1.0) {
            phase -= 1.0;
            tr = clockTick(unit, unit->clock, kEvery, i + 1);
            unit->pFreq = unit->cFreq;
            unit->pAmp = unit->cAmp;
            unit->pShape = unit->cShape;
            unit->pSkew = unit->cSkew;
            if (unit->repLeft <= 0) {
                unit->cFreq = pull(unit, kFreq, i + 1, (float)unit->cFreq);
                unit->cAmp = pull(unit, kAmp, i + 1, (float)unit->cAmp);
                unit->cShape = pull(unit, kShape, i + 1, (float)unit->cShape);
                unit->cSkew = pull(unit, kSkew, i + 1, (float)unit->cSkew);
                unit->glide = pull(unit, kGlide, i + 1, unit->glide);
                unit->balance = pull(unit, kBalance, i + 1, unit->balance);
                float reps = pull(unit, kRepeats, i + 1, 1.f);
                unit->repLeft = reps < 1.f ? 1 : (int)reps;
                if (!unit->started) {
                    unit->pFreq = unit->cFreq;
                    unit->pAmp = unit->cAmp;
                    unit->pShape = unit->cShape;
                    unit->pSkew = unit->cSkew;
                    unit->started = true;
                }
            }
            unit->repLeft--;
            if (phase >= 1.0)
                phase = 0.0;
        }

        // glide 0 jumps to the new values; glide 1 reaches them at the end of the waveset
        double g = unit->glide;
        double t = g <= 0.0 ? 1.0 : (phase >= g ? 1.0 : phase / g);
        double freq = unit->pFreq + t * (unit->cFreq - unit->pFreq);
        double amp = unit->pAmp + t * (unit->cAmp - unit->pAmp);
        double shape = unit->pShape + t * (unit->cShape - unit->pShape);
        double s = clampd(unit->pSkew + t * (unit->cSkew - unit->pSkew), 0.01, 0.99);

        // both halves share the shape, so their areas are proportional to their lengths
        double gPos, gNeg;
        balanceGains(s, 1.0 - s, unit->balance, gPos, gNeg);
        double y = phase < s ? gPos * halfCycle(shape, phase / s) : -gNeg * halfCycle(shape, (phase - s) / (1.0 - s));
        out[i] = zapgremlins((float)(y * amp));
        if (trig)
            trig[i] = tr;
        phase += incFor(freq, sr);
    }

    unit->phase = phase;
}

PluginLoad(WsGen) {
    ft = inTable;
    DefineSimpleUnit(WsGen);
}
