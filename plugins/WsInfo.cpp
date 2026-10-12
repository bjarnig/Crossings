// WsInfo: measures each waveset of a live input. Outputs, updated when a waveset ends:
// frequency (1 / length), peak, skew (fraction of the waveset before the falling crossing)
// and a trigger.

#include "ws.hpp"

static InterfaceTable* ft;

enum { kIn, kThresh };

struct WsInfo : public Unit {
    WsDetect det;
    float freq, peak, skew;
};

static void WsInfo_next(WsInfo* unit, int inNumSamples);
static void WsInfo_Ctor(WsInfo* unit);

void WsInfo_Ctor(WsInfo* unit) {
    SETCALC(WsInfo_next);
    unit->det = WsDetect();
    unit->freq = 0.f;
    unit->peak = 0.f;
    unit->skew = 0.5f;
    for (int k = 0; k < 4; ++k)
        OUT0(k) = k == 2 ? 0.5f : 0.f;
}

void WsInfo_next(WsInfo* unit, int inNumSamples) {
    const float* in = IN(kIn);
    float thresh = IN0(kThresh);
    double sr = FULLRATE; // the input's sample rate, also when this unit runs at control rate
    bool audio = unit->mCalcRate == calc_FullRate;
    // at control rate, scan the whole audio block of the input and report its last waveset
    int n = audio ? inNumSamples : (INRATE(kIn) == calc_FullRate ? FULLBUFLENGTH : 1);
    float anyTrig = 0.f;

    for (int i = 0; i < n; ++i) {
        float tr = 0.f;
        if (detect(unit->det, in[i], thresh)) {
            unit->freq = (float)(sr / unit->det.len);
            unit->peak = (float)unit->det.lastPeak;
            unit->skew = (float)unit->det.skew;
            tr = anyTrig = 1.f;
        }
        if (audio) {
            OUT(0)[i] = unit->freq;
            OUT(1)[i] = unit->peak;
            OUT(2)[i] = unit->skew;
            OUT(3)[i] = tr;
        }
    }
    if (!audio) {
        OUT0(0) = unit->freq;
        OUT0(1) = unit->peak;
        OUT0(2) = unit->skew;
        OUT0(3) = anyTrig;
    }
}

PluginLoad(WsInfo) {
    ft = inTable;
    DefineSimpleUnit(WsInfo);
}
