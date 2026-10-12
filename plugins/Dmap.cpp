// Dmap: demand-rate iterated map. Each pull steps the map once and returns x in [0,1]
// (axis 0) or the second stream mapped to [0,1] (axis 1), the same values WsMap uses.

#include "ws.hpp"

static InterfaceTable* ft;

enum { kLength, kR, kMap, kInit, kAxis, kHyst };

struct Dmap : public Unit {
    double x, y, prev, rEff;
    float r, map, axis, hyst;
    double repeats;
    int count;
};

static void Dmap_next(Dmap* unit, int inNumSamples);
static void Dmap_Ctor(Dmap* unit);

void Dmap_next(Dmap* unit, int inNumSamples) {
    if (!inNumSamples) { // reset, also resetting demand-rate inputs
        RESETINPUT(kLength);
        RESETINPUT(kR);
        RESETINPUT(kMap);
        RESETINPUT(kAxis);
        RESETINPUT(kHyst);
        mapInit(IN0(kInit), unit->x, unit->y);
        unit->prev = unit->x;
        unit->rEff = -1.0;
        unit->repeats = -1.0;
        unit->count = 0;
        return;
    }
    if (unit->repeats < 0.0) {
        float n = DEMANDINPUT_A(kLength, inNumSamples);
        unit->repeats = std::isnan(n) ? 0.0 : std::floor(n + 0.5);
    }
    if (unit->count >= unit->repeats) {
        OUT0(0) = NAN;
        return;
    }
    unit->count++;
    unit->r = pull(unit, kR, inNumSamples, unit->r);
    unit->map = pull(unit, kMap, inNumSamples, unit->map);
    unit->axis = pull(unit, kAxis, inNumSamples, unit->axis);
    unit->hyst = pull(unit, kHyst, inNumSamples, unit->hyst);

    unit->prev = unit->x;
    unit->rEff = unit->rEff < 0.0 ? unit->r : backlash(unit->rEff, unit->r, unit->hyst);
    int m = (int)clampd(unit->map, 0.0, kNumMaps - 1);
    stepMap(m, unit->rEff, unit->x, unit->y);
    OUT0(0) = unit->axis >= 0.5f ? (float)(0.5 + 0.5 * clampd(mapSecond(m, unit->x, unit->y, unit->prev), -1.0, 1.0))
                                 : (float)unit->x;
}

void Dmap_Ctor(Dmap* unit) {
    SETCALC(Dmap_next);
    unit->r = 0.9f;
    unit->map = 0.f;
    unit->axis = 0.f;
    unit->hyst = 0.f;
    Dmap_next(unit, 0);
    OUT0(0) = 0.f;
}

PluginLoad(Dmap) {
    ft = inTable;
    DefineSimpleUnit(Dmap);
}
