# OpenPedals

A collection of 37 guitar effect pedal plugins for macOS, built as Audio Units for use in GarageBand, Logic Pro, and any other AU-compatible host.
Every plugin is free and open source. No subscriptions, no telemetry.

The code has been written mostly with help from OpenCode and Claude Opus 4.6. Let's see how far we can get!

## Available Effects

### Gain & Drive

| Plugin | Code | Parameters | Description |
|--------|------|-----------|-------------|
| **OP Overdrive** | `Ovdr` | Drive, Tone, Level | Soft-clip overdrive with 4x oversampling and tone shaping |
| **OP Distortion** | `Dist` | Drive, Tone, Level, Mode | Hard/tube/fuzz clipping modes with 4x oversampling |
| **OP Fuzz** | `Fuzz` | Fuzz, Tone, Level | Extreme asymmetric clipping with 4x oversampling |
| **OP Amp Sim** | `AmSm` | Gain, Bass, Mid, Treble, Volume | Preamp, tone stack, power amp, and cabinet simulation chain |

### Modulation

| Plugin | Code | Parameters | Description |
|--------|------|-----------|-------------|
| **OP Chorus** | `Chrs` | Rate, Depth, Mix | LFO-modulated delay line chorus |
| **OP Flanger** | `Flgr` | Rate, Depth, Feedback, Mix | Short modulated delay with resonant feedback |
| **OP Phaser** | `Phsr` | Rate, Depth, Feedback, Mix | 4-stage allpass filter chain |
| **OP Vibrato** | `Vibr` | Rate, Depth | 100% wet pitch modulation |
| **OP Tremolo** | `Trem` | Rate, Depth, Shape | LFO amplitude modulation with waveform control |
| **OP Rotary** | `Rtry` | Speed, Depth, Mix | Leslie speaker simulation with horn/drum |
| **OP Uni-Vibe** | `UVib` | Speed, Intensity, Volume | Vintage phase shifter with non-uniform allpass spacing |
| **OP Ring Mod** | `RngM` | Frequency, Mix, Shape | Ring modulation with sine/square carrier |

### Delay

| Plugin | Code | Parameters | Description |
|--------|------|-----------|-------------|
| **OP Delay** | `DDly` | Time, Feedback, Mix, Tone | Digital delay with filtered feedback (1--2000 ms) |
| **OP Analog Delay** | `ADly` | Time, Feedback, Tone, Mix | BBD emulation with filtering and modulation |
| **OP Tape Delay** | `TDly` | Time, Feedback, Flutter, Mix | Tape saturation, flutter, progressive filtering |
| **OP Reverse Delay** | `RDly` | Time, Feedback, Mix | Double-buffered reverse playback delay |

### Reverb

| Plugin | Code | Parameters | Description |
|--------|------|-----------|-------------|
| **OP Reverb** | `Rvrb` | Decay, Damping, Mix, Pre-Delay | Room reverb with adjustable pre-delay |
| **OP Plate Reverb** | `PRvb` | Decay, Damping, Mix, Pre-Delay | Bright, tight plate character |
| **OP Spring Reverb** | `SRvb` | Decay, Drip, Mix | Spring tank drip/resonance with allpass pre-filtering |
| **OP Shimmer Reverb** | `ShRv` | Decay, Shimmer, Mix, Damping | Octave-up pitch shift in reverb feedback loop |

### EQ & Filter

| Plugin | Code | Parameters | Description |
|--------|------|-----------|-------------|
| **OP Graphic EQ** | `GrEQ` | 7 frequency bands, Level | 7-band fixed-frequency peaking EQ |
| **OP Parametric EQ** | `PrEQ` | 3 bands (freq, gain, Q each) | 3-band fully parametric EQ |
| **OP Wah** | `Wahh` | Position, Q, Volume | Manual bandpass filter (position automatable) |
| **OP Auto-Wah** | `AWah` | Sensitivity, Frequency, Q, Mix | Envelope-controlled bandpass filter |

### Dynamics

| Plugin | Code | Parameters | Description |
|--------|------|-----------|-------------|
| **OP Compressor** | `Comp` | Threshold, Ratio, Attack, Release, Makeup | Dynamics compressor with makeup gain |
| **OP Noise Gate** | `NGte` | Threshold, Attack, Hold, Release | Gate with attack/hold/release envelope |
| **OP Limiter** | `Lmtr` | Threshold, Release | Brick-wall peak limiter |

### Pitch

| Plugin | Code | Parameters | Description |
|--------|------|-----------|-------------|
| **OP Octaver** | `Octv` | Sub Level, Up Level, Dry Level | Sub-octave and octave-up via zero-crossing detection |
| **OP Pitch Shifter** | `PtSh` | Semitones, Fine, Mix | Granular pitch shift, +/-12 semitones |
| **OP Harmonizer** | `Harm` | Interval, Mix, Detune | Pitch-shifted harmony blended with dry signal |
| **OP Detune** | `Dtne` | Detune, Mix | Stereo detuning for width and thickness |

### Utility & Special

| Plugin | Code | Parameters | Description |
|--------|------|-----------|-------------|
| **OP Tuner** | `Tunr` | Mute | Mute-switch utility for silent tuning |
| **OP Slow Gear** | `SlGr` | Sensitivity, Attack, Volume | Auto-swell volume effect |
| **OP Looper** | `Loop` | Freeze | Freeze/loop circular buffer |
| **OP Slicer** | `Slcr` | Rate, Depth, Shape | Rhythmic gate/chopper effect |
| **OP Acoustic Sim** | `AcSm` | Body, Top, Level | EQ shaping for acoustic guitar character |
| **OP Humanizer** | `Hmzr` | Rate, Depth, Vowel, Mix | Formant filter morphing through vowel sounds |

All 37 plugins pass Apple's `auval` validation suite and are built as Universal Binaries (Apple Silicon + Intel).

## Building from Source

### Requirements

- macOS 11.0 or later
- Xcode (with command-line tools)
- CMake 3.22+
- Git

### Clone

```bash
git clone --recurse-submodules https://github.com/troendheim/open-pedals.git
cd open-pedals
```

If you already cloned without `--recurse-submodules`:

```bash
git submodule update --init --recursive
```

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(sysctl -n hw.ncpu)
```

The build produces AU (Audio Unit v2) `.component` bundles. With `COPY_PLUGIN_AFTER_BUILD` enabled (the default), they are automatically installed to:

```
~/Library/Audio/Plug-Ins/Components/
```

### Verify Installation

List registered OpenPedals plugins:

```bash
auval -a | grep -i openpedals
```

Run full validation on a specific plugin (e.g., Overdrive):

```bash
auval -v aufx Ovdr OpPd
```

### Using in GarageBand

1. Build and install the plugins (see above)
2. Open GarageBand and create or open a project
3. Select a track, open the Smart Controls or plugin chain
4. Under **Audio Units > OpenPedals**, you will find all installed effects

> **Note:** GarageBand will show a warning about "lowering security settings" the first time you load an OpenPedals plugin. This is because the plugins are not signed with an Apple Developer certificate -- it has nothing to do with the safety of the code. Click "Lower Security" to proceed. GarageBand shows this for all third-party plugins built from source. It only needs to be accepted once per plugin.

## Project Structure

```
openpedals/
  CMakeLists.txt          # Root build configuration
  JUCE/                   # JUCE framework (git submodule)
  common/                 # Shared DSP library (header-only)
    BiquadFilter.h        #   IIR filter (Audio EQ Cookbook)
    CabinetSim.h          #   Speaker cabinet simulation
    DelayLine.h           #   Circular buffer with interpolation
    EnvelopeFollower.h    #   Amplitude envelope detector
    LFO.h                 #   Low-frequency oscillator
    Oversampling.h        #   JUCE oversampling wrapper
    ToneStack.h           #   3-band bass/mid/treble EQ
    Waveshaper.h          #   Nonlinear clipping functions
  plugins/
    Overdrive/            # Each plugin has its own CMakeLists.txt,
    Delay/                #   PluginProcessor.h, and
    Chorus/               #   PluginProcessor.cpp
    ...                   #   (37 plugin directories total)
```

The shared DSP library in `common/` provides reusable building blocks that all plugins draw from. Each plugin is a thin layer that wires together the appropriate DSP components with an APVTS parameter layout.

## Roadmap

### Future Export Targets

The project currently builds AU (Audio Unit v2) plugins for macOS. Planned format expansions:

- [ ] **VST3** -- broad DAW compatibility (Ableton Live, FL Studio, Reaper, Cubase, etc.)
- [ ] **AUv3** -- modern Audio Unit format for iOS / iPadOS (GarageBand mobile, AUM, Cubasis)
- [ ] **CLAP** -- open plugin standard gaining traction in Bitwig, Reaper, and others
- [ ] **Standalone** -- self-contained apps with built-in audio I/O for practice and live use
- [ ] **LV2** -- Linux plugin format for Ardour, Carla, and other Linux DAWs

### Future Effects

- [ ] Custom skeuomorphic pedal GUIs
- [ ] MIDI-controllable parameters
- [ ] Stereo widening and spatial effects
- [ ] Multi-effects chain plugin

## Technical Details

- **Framework:** JUCE 8.x
- **Language:** C++17
- **Architecture:** Universal Binary (arm64 + x86_64)
- **Minimum macOS:** 11.0 (Big Sur)
- **Plugin format:** AU v2 (`.component` bundles)
- **GUI:** Host-provided generic parameter editors (no custom GUIs)
- **Manufacturer code:** `OpPd`

## License

This project is licensed under the **GNU General Public License v3.0** -- see the [LICENSE](LICENSE) file for details.

JUCE is used under its GPLv3-compatible free license. The JUCE framework itself is included as a git submodule and is subject to its own licensing terms.
