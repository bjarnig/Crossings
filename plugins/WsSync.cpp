// WsSync: synthesizes wavesets on the zero crossings of a live input. Each input waveset's
// length (and optionally its skew and peak) is queued when it ends and replayed as `count`
// generated wavesets, so the output keeps the input's timing exactly, one waveset late.
// When the queue runs dry the output waits in silence, which grows the delay to fit.

#include "ws.hpp"

static InterfaceTable* ft;

enum { kIn, kAmp, kShape, kSkew, kCount, kSkewFollow, kAmpFollow, kThresh, kBalance, kEvery };

static const int kQueue = 64;

struct WsSync : public Unit {
    WsDetect det;
    double qLen[kQueue], qSkew[kQueue], qPeak[kQueue];
    int qHead, qSize;
    double phase, inc;
    bool playing;
    int subLeft;
    float amp, shape, skew, count, skewFollow, ampFollow, balance;
    double peak, s, gPos, gNeg;
    WsClock clock;
};

static void WsSync_next(WsSync* unit, int inNumSamples);
static void WsSync_Ctor(WsSync* unit);

void WsSync_Ctor(WsSync* unit) {
    SETCALC(WsSync_next);
    unit->det = WsDetect();
    unit->qHead = unit->qSize = 0;
    unit->phase = 0.0;
    unit->inc = 0.0;
    unit->playing = false;
    unit->subLeft = 0;
    unit->amp = 0.5f;
    unit->shape = 0.f;
    unit->skew = 0.5f;
    unit->count = 1.f;
    unit->skewFollow = 0.f;
    unit->ampFollow = 0.f;
    unit->balance = 0.f;
    unit->peak = 0.0;
    unit->s = 0.5;
    unit->gPos = unit->gNeg = 1.0;
    unit->clock = WsClock();
    OUT0(0) = 0.f;
    if (unit->mNumOutputs > 1)
        OUT0(1) = 0.f;
}

// Starts the next generated waveset; returns false when there is nothing to play yet.
static inline bool nextWaveset(WsSync* unit, int offset) {
    if (unit->subLeft > 0) {
        unit->subLeft--;
        return true;
    }
    if (unit->qSize == 0)
        return false;
    int k = unit->qHead;
    unit->qHead = (unit->qHead + 1) % kQueue;
    unit->qSize--;

    unit->amp = pull(unit, kAmp, offset, unit->amp);
    unit->shape = pull(unit, kShape, offset, unit->shape);
    unit->skew = pull(unit, kSkew, offset, unit->skew);
    unit->count = pull(unit, kCount, offset, unit->count);
    unit->skewFollow = pull(unit, kSkewFollow, offset, unit->skewFollow);
    unit->ampFollow = pull(unit, kAmpFollow, offset, unit->ampFollow);
    unit->balance = pull(unit, kBalance, offset, unit->balance);

    int c = unit->count < 1.f ? 1 : (int)unit->count;
    unit->subLeft = c - 1;
    unit->inc = c / unit->qLen[k];
    double sf = clampd(unit->skewFollow, 0.0, 1.0), af = clampd(unit->ampFollow, 0.0, 1.0);
    unit->s = clampd(unit->skew + sf * (unit->qSkew[k] - unit->skew), 0.01, 0.99);
    unit->peak = unit->amp * (1.0 + af * (unit->qPeak[k] - 1.0));
    balanceGains(unit->s, 1.0 - unit->s, unit->balance, unit->gPos, unit->gNeg);
    return true;
}

void WsSync_next(WsSync* unit, int inNumSamples) {
    const float* in = IN(kIn);
    float* out = OUT(0);
    float* trig = unit->mNumOutputs > 1 ? OUT(1) : nullptr; // only with arTrig
    float thresh = IN0(kThresh);
    double phase = unit->phase;

    for (int i = 0; i < inNumSamples; ++i) {
        if (detect(unit->det, in[i], thresh) && unit->qSize < kQueue) {
            int k = (unit->qHead + unit->qSize) % kQueue;
            unit->qLen[k] = unit->det.len < 2.0 ? 2.0 : unit->det.len;
            unit->qSkew[k] = unit->det.skew;
            unit->qPeak[k] = unit->det.lastPeak;
            unit->qSize++;
        }

        float tr = 0.f;
        if (!unit->playing || phase >= 1.0) {
            phase = unit->playing ? phase - 1.0 : 0.0;
            unit->playing = nextWaveset(unit, i + 1);
            if (unit->playing) {
                tr = clockTick(unit, unit->clock, kEvery, i + 1);
                if (phase >= 1.0)
                    phase = 0.0;
            }
        }

        double v = 0.0;
        if (unit->playing) {
            double s = unit->s;
            v = phase < s ? unit->gPos * halfCycle(unit->shape, phase / s)
                          : -unit->gNeg * halfCycle(unit->shape, (phase - s) / (1.0 - s));
            v *= unit->peak;
            phase += unit->inc;
        }
        out[i] = zapgremlins((float)v);
        if (trig)
            trig[i] = tr;
    }

    unit->phase = phase;
}

PluginLoad(WsSync) {
    ft = inTable;
    DefineSimpleUnit(WsSync);
}
