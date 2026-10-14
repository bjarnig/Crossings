// WsNest: wavesets made of wavesets. Each half of a waveset holds `branch` smaller wavesets,
// down to `depth` levels, and only the bottom level is drawn. A node's children continue the
// map's orbit from the node's own state, so one rule sets the proportions at every scale.
// Signs multiply down the tree, so a parent's sign change can merge two child half-cycles
// and remove the zero crossing between them.

#include "ws.hpp"

static InterfaceTable* ft;

enum { kR, kLo, kHi, kAmp, kShape, kMap, kDepth, kBranch, kSpread, kContour, kSkewDepth, kInit, kBalance, kEvery };

static const int kMaxHalves = 4096;
static const int kMaxBranch = 8;
static const double kMinHalf = 4.0; // samples

struct WsNest : public Unit {
    double phase, inc;
    double x, y;
    float r, lo, hi, amp, shape, map, depth, branch, spread, contour, skewDepth, balance;
    int n, j; // half-cycles in the tree, current half
    double edge[kMaxHalves + 1]; // half-cycle boundaries over the top waveset's phase 0..1
    double gain[kMaxHalves]; // signed peak of each half
    WsClock clock;
};

struct NestBuild {
    WsNest* unit;
    int map, depth, branch;
    double r, spread, contour, shape, skewDepth;
    double aPos, aNeg;
};

static void WsNest_next(WsNest* unit, int inNumSamples);
static void WsNest_Ctor(WsNest* unit);

void WsNest_Ctor(WsNest* unit) {
    SETCALC(WsNest_next);
    unit->phase = 1.0;
    unit->inc = 0.0;
    mapInit(IN0(kInit), unit->x, unit->y);
    unit->r = 0.9f;
    unit->lo = 2.f;
    unit->hi = 20.f;
    unit->amp = 0.f;
    unit->shape = 0.f;
    unit->map = 0.f;
    unit->depth = 2.f;
    unit->branch = 3.f;
    unit->spread = 1.f;
    unit->contour = 0.f;
    unit->skewDepth = 0.f;
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

// One waveset node from start to start + len. Its 2 * branch children are the next steps
// of the orbit from (x, y), first through the positive half and on through the negative.
static void node(NestBuild& b, double x, double y, double prev, int level, double start, double len, double sign,
                 double env) {
    double s = clampd(0.5 + b.skewDepth * mapSecond(b.map, x, y, prev), 0.01, 0.99);
    double hStart[2] = { start, start + len * s }, hLen[2] = { len * s, len * (1.0 - s) };
    double hSign[2] = { sign, -sign };
    double cx = x, cy = y;
    for (int h = 0; h < 2; ++h) {
        if (level == b.depth) {
            WsNest* unit = b.unit;
            unit->edge[unit->n] = hStart[h];
            unit->gain[unit->n] = hSign[h] * env;
            (hSign[h] > 0.0 ? b.aPos : b.aNeg) += hLen[h] * env;
            unit->n++;
            continue;
        }
        double sx[kMaxBranch], sy[kMaxBranch], sp[kMaxBranch], w[kMaxBranch], total = 0.0;
        for (int c = 0; c < b.branch; ++c) {
            sp[c] = cx;
            stepMap(b.map, b.r, cx, cy);
            sx[c] = cx;
            sy[c] = cy;
            w[c] = std::exp2(b.spread * (1.0 - 2.0 * cx)); // high x, short child, as in WsMap
            total += w[c];
        }
        double pos = hStart[h];
        for (int c = 0; c < b.branch; ++c) {
            double clen = hLen[h] * w[c] / total;
            double centre = (pos + 0.5 * clen - hStart[h]) / hLen[h];
            double cenv = env * (1.0 - b.contour + b.contour * halfCycle(b.shape, centre));
            node(b, sx[c], sy[c], sp[c], level + 1, pos, clen, hSign[h], cenv);
            pos += clen;
        }
    }
}

static void newTree(WsNest* unit, int offset, double sr) {
    unit->r = pull(unit, kR, offset, unit->r);
    unit->lo = pull(unit, kLo, offset, unit->lo);
    unit->hi = pull(unit, kHi, offset, unit->hi);
    unit->amp = pull(unit, kAmp, offset, unit->amp);
    unit->shape = pull(unit, kShape, offset, unit->shape);
    unit->map = pull(unit, kMap, offset, unit->map);
    unit->depth = pull(unit, kDepth, offset, unit->depth);
    unit->branch = pull(unit, kBranch, offset, unit->branch);
    unit->spread = pull(unit, kSpread, offset, unit->spread);
    unit->contour = pull(unit, kContour, offset, unit->contour);
    unit->skewDepth = pull(unit, kSkewDepth, offset, unit->skewDepth);
    unit->balance = pull(unit, kBalance, offset, unit->balance);

    NestBuild b;
    b.unit = unit;
    b.map = (int)clampd(unit->map, 0.0, kNumMaps - 1);
    b.branch = (int)clampd(unit->branch, 1.0, kMaxBranch);
    // the tree has 2 * (2 * branch)^depth half-cycles; depth is lowered until that fits
    int depth = (int)clampd(unit->depth, 0.0, 11.0), halves = 2;
    for (int d = 0; d < depth; ++d) {
        if (halves * 2 * b.branch > kMaxHalves) {
            depth = d;
            break;
        }
        halves *= 2 * b.branch;
    }
    b.depth = depth;
    b.r = unit->r;
    b.spread = unit->spread;
    b.contour = clampd(unit->contour, 0.0, 1.0);
    b.shape = unit->shape;
    b.skewDepth = unit->skewDepth;

    double prev = unit->x;
    stepMap(b.map, b.r, unit->x, unit->y);
    double lo = std::fabs(unit->lo) < 0.01 ? 0.01 : std::fabs(unit->lo);
    double hi = std::fabs(unit->hi) < 0.01 ? 0.01 : std::fabs(unit->hi);
    unit->inc = incFor(lo * std::pow(hi / lo, unit->x), sr);

    // a level whose shortest half-cycle falls under kMinHalf samples would alias; drop it
    for (;; --b.depth) {
        b.aPos = b.aNeg = 0.0;
        unit->n = 0;
        node(b, unit->x, unit->y, prev, 0, 0.0, 1.0, 1.0, 1.0);
        unit->edge[unit->n] = 1.0;
        double shortest = 1.0;
        for (int j = 0; j < unit->n; ++j)
            shortest = std::fmin(shortest, unit->edge[j + 1] - unit->edge[j]);
        if (b.depth == 0 || shortest >= kMinHalf * unit->inc)
            break;
    }

    double gPos, gNeg;
    balanceGains(b.aPos, b.aNeg, unit->balance, gPos, gNeg);
    for (int j = 0; j < unit->n; ++j)
        unit->gain[j] *= unit->gain[j] > 0.0 ? gPos : gNeg;

    unit->j = 0;
}

void WsNest_next(WsNest* unit, int inNumSamples) {
    float* out = OUT(0);
    float* trig = unit->mNumOutputs > 1 ? OUT(1) : nullptr; // only with arTrig
    double sr = SAMPLERATE;
    double phase = unit->phase;

    for (int i = 0; i < inNumSamples; ++i) {
        float tr = 0.f;
        if (phase >= 1.0) {
            phase -= 1.0;
            tr = clockTick(unit, unit->clock, kEvery, i + 1);
            newTree(unit, i + 1, sr);
            if (phase >= 1.0)
                phase = 0.0;
        }
        while (unit->j < unit->n - 1 && phase >= unit->edge[unit->j + 1])
            unit->j++;

        int j = unit->j;
        double a = unit->edge[j], b = unit->edge[j + 1];
        double v = b > a ? unit->gain[j] * halfCycle(unit->shape, (phase - a) / (b - a)) : 0.0;
        out[i] = zapgremlins((float)(v * unit->amp));
        if (trig)
            trig[i] = tr;
        phase += unit->inc;
    }

    unit->phase = phase;
}

PluginLoad(WsNest) {
    ft = inTable;
    DefineSimpleUnit(WsNest);
}
