![Crossings](docs/banner.svg)

# Crossings

SuperCollider server plugins (UGens) for waveset synthesis. A waveset is the span of signal between alternate zero crossings: one positive and one negative half-cycle. It is a companion to [WavesetUGens](https://github.com/bjarnig/WavesetUGens), which ports CDP's processes for cutting recorded sound into wavesets. Crossings synthesizes the wavesets themselves.

Every parameter is read once per waveset, at the zero crossing. Demand-rate inputs (`Dseq`, `Dbrown`, `Dwhite`, ...) therefore advance one value per waveset, and each waveset can have its own length, shape, peak and skew. Pitch, timbre and rhythm come from the same sequence of decisions.

## UGens

- **`WsGen`**: wavesets with per-waveset length, peak, shape (sine, triangle, square, spike, morphable), skew, repeats and glide.
- **`WsMap`**: each waveset computed from the previous one through an iterated map (logistic, tent, sine, circle, standard, Ikeda). Fixed points give a steady pitch, periodic orbits repeating patterns, chaos rough streams. Optional hysteresis on the map parameter.
- **`WsGendy`**: Xenakis' dynamic stochastic synthesis bounded by zero crossings. The breakpoints of each half-cycle random-walk once per waveset.
- **`Dmap`**: the WsMap maps at demand rate, so one orbit can drive waveset parameters, `TDuty` rhythms or `DemandEnvGen` breakpoints, or a slower map can steer WsMap.

`WsGen`, `WsMap` and `WsGendy` share two extras:

- **`arTrig`** returns `[sig, trig]`, with a trigger at the start of every `every`-th waveset, so the waveset clock can drive other UGens.
- **`balance`** scales the larger half-cycle so both halves enclose the same area, removing the DC offset that skewed wavesets produce.

## Build from source

```sh
./build.sh [path-to-supercollider-source]
```

Builds and installs into your user Extensions folder (`.../SuperCollider/Extensions/Crossings/`). Then recompile the class library (Cmd-Shift-L) and boot the server.

```supercollider
{ WsMap.ar(MouseX.kr(0.7, 1), 60, 1200, 0.3) ! 2 }.play;
```

Requires CMake 3.12 or later and a C++17 compiler. Builds against a SuperCollider source checkout matching the scsynth you run (plugin API v3 for SuperCollider 3.12 to 3.14).

## Credits & license

The waveset follows Trevor Wishart's definition, and `WsGendy` follows Iannis Xenakis' GENDYN. Licensed under **GPL-3.0** (see [`LICENSE`](LICENSE)).
