// WsGroup: a waveset made of any number of half-cycles. Signs alternate within a group and
// restart positive at the next, so odd counts give runs of equal sign across the boundary.
// tilt spreads the half-cycle lengths, decay lowers each successive half-cycle and flip
// inverts signs at random. All inputs are read once per group.

#include "ws.hpp"

static InterfaceTable* ft;

enum { kFreq, kAmp, kShape, kHalves, kTilt, kDecay, kFlip, kBalance, kEvery };

static const int kMaxHalves = 64;

struct WsGroup : public Unit {
    double phase, inc;
    float freq, amp, shape, halves, tilt, decay, flip, balance;
    int n, j; // halves in the group, current half
    double edge[kMaxHalves + 1]; // half-cycle boundaries over the group's phase 0..1
    double gain[kMaxHalves]; // signed peak of each half
    WsClock clock;
};

static void WsGroup_next(WsGroup* unit, int inNumSamples);
static void WsGroup_Ctor(WsGroup* unit);

void WsGroup_Ctor(WsGroup* unit) {
    SETCALC(WsGroup_next);
    unit->phase = 1.0;
    unit->inc = 0.0;
    unit->freq = 110.f;
    unit->amp = 0.f;
    unit->shape = 0.f;
    unit->halves = 3.f;
    unit->tilt = 0.f;
    unit->decay = 0.f;
    unit->flip = 0.f;
    unit->balance = 0.f;
    unit->n = 1;
    unit->j = 0;
    unit->edge[0] = 0.0;
    unit->edge[1] = 1.0;
    unit->gain[0] = 0.0;
    unit->clock = WsClock();
    OUT0(0) = 0.f;
    if (unit->mNumOutputs > 1)
        OUT0(1) = 0.f;
}

static void newGroup(WsGroup* unit, int offset, double sr) {
    unit->freq = pull(unit, kFreq, offset, unit->freq);
    unit->amp = pull(unit, kAmp, offset, unit->amp);
    unit->shape = pull(unit, kShape, offset, unit->shape);
    unit->halves = pull(unit, kHalves, offset, unit->halves);
    unit->tilt = pull(unit, kTilt, offset, unit->tilt);
    unit->decay = pull(unit, kDecay, offset, unit->decay);
    unit->flip = pull(unit, kFlip, offset, unit->flip);
    unit->balance = pull(unit, kBalance, offset, unit->balance);

    RGen& rgen = *unit->mParent->mRGen;
    int n = (int)clampd(unit->halves, 1.0, kMaxHalves);
    double fade = 1.0 - clampd(unit->decay, 0.0, 1.0), flip = clampd(unit->flip, 0.0, 1.0);
    // half j has length 2^(tilt * (2j/(n-1) - 1)): tilt 1 makes the last half four times the first
    double total = 0.0, level = 1.0, aPos = 0.0, aNeg = 0.0;
    double w[kMaxHalves];
    for (int j = 0; j < n; ++j) {
        w[j] = n > 1 ? std::exp2(unit->tilt * (2.0 * j / (n - 1) - 1.0)) : 1.0;
        total += w[j];
        double sign = (j % 2 == 0) ? 1.0 : -1.0;
        if (flip > 0.0 && rgen.frand() < flip)
            sign = -sign;
        unit->gain[j] = sign * level;
        // equal shapes, so each half's area is proportional to its length times its level
        (sign > 0.0 ? aPos : aNeg) += w[j] * level;
        level *= fade;
    }
    unit->edge[0] = 0.0;
    for (int j = 0; j < n; ++j)
        unit->edge[j + 1] = unit->edge[j] + w[j] / total;
    unit->edge[n] = 1.0;

    double gPos, gNeg;
    balanceGains(aPos, aNeg, unit->balance, gPos, gNeg);
    for (int j = 0; j < n; ++j)
        unit->gain[j] *= unit->gain[j] > 0.0 ? gPos : gNeg;

    unit->n = n;
    unit->j = 0;
    unit->inc = incFor(unit->freq, sr);
}

void WsGroup_next(WsGroup* unit, int inNumSamples) {
    float* out = OUT(0);
    float* trig = unit->mNumOutputs > 1 ? OUT(1) : nullptr; // only with arTrig
    double sr = SAMPLERATE;
    double phase = unit->phase;

    for (int i = 0; i < inNumSamples; ++i) {
        float tr = 0.f;
        if (phase >= 1.0) {
            phase -= 1.0;
            tr = clockTick(unit, unit->clock, kEvery, i + 1);
            newGroup(unit, i + 1, sr);
            if (phase >= 1.0)
                phase = 0.0;
        }
        while (unit->j < unit->n - 1 && phase >= unit->edge[unit->j + 1])
            unit->j++;

        int j = unit->j;
        double a = unit->edge[j], b = unit->edge[j + 1];
        double v = unit->gain[j] * halfCycle(unit->shape, (phase - a) / (b - a));
        out[i] = zapgremlins((float)(v * unit->amp));
        if (trig)
            trig[i] = tr;
        phase += unit->inc;
    }

    unit->phase = phase;
}

PluginLoad(WsGroup) {
    ft = inTable;
    DefineSimpleUnit(WsGroup);
}
