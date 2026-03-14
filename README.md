# Bosslike

A growing collection of guitar effect pedal plugins for macOS, built as Audio Units for use in GarageBand, Logic Pro, and any other AU-compatible host.

## Motivation

Stock DAW effects are designed for mixing, not for guitarists. They lack the character, responsiveness, and familiar parameter layouts that pedalboard players expect. Bosslike aims to fill that gap with a comprehensive set of stompbox-style effects -- each focused on doing one thing well, with a minimal parameter set that makes sense to guitar players, not mixing engineers.

Every plugin is free and open source. No subscriptions, no dongles, no telemetry.

## Available Effects

| Plugin | Parameters | Description |
|--------|-----------|-------------|
| **Bosslike Overdrive** | Drive, Tone, Level | Soft-clip overdrive with 4x oversampling and tone shaping |
| **Bosslike Delay** | Time, Feedback, Mix, Tone | Digital delay with filtered feedback path (1--2000 ms) |
| **Bosslike Chorus** | Rate, Depth, Mix | LFO-modulated delay line chorus |
| **Bosslike Reverb** | Decay, Damping, Mix, Pre-Delay | Room reverb with adjustable pre-delay |
| **Bosslike Compressor** | Threshold, Ratio, Attack, Release, Makeup Gain | Dynamics compressor with makeup gain stage |

All plugins pass Apple's `auval` validation suite and are built as Universal Binaries (Apple Silicon + Intel).

## Building from Source

### Requirements

- macOS 11.0 or later
- Xcode (with command-line tools)
- CMake 3.22+
- Git

### Clone

```bash
git clone --recurse-submodules https://github.com/your-username/bosslike-vsts.git
cd bosslike-vsts
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

List registered Bosslike plugins:

```bash
auval -a | grep -i bosslike
```

Run full validation on a specific plugin (e.g., Overdrive):

```bash
auval -v aufx Ovdr BsLk
```

All five plugin codes for validation:

| Plugin | auval command |
|--------|--------------|
| Overdrive | `auval -v aufx Ovdr BsLk` |
| Delay | `auval -v aufx DDly BsLk` |
| Chorus | `auval -v aufx Chrs BsLk` |
| Reverb | `auval -v aufx Rvrb BsLk` |
| Compressor | `auval -v aufx Comp BsLk` |

### Using in GarageBand

1. Build and install the plugins (see above)
2. Open GarageBand and create or open a project
3. Select a track, open the Smart Controls or plugin chain
4. Under **Audio Units > Bosslike**, you will find all installed effects

## Project Structure

```
bosslike-vsts/
  CMakeLists.txt          # Root build configuration
  JUCE/                   # JUCE framework (git submodule)
  common/                 # Shared DSP library (header-only)
    BiquadFilter.h        #   IIR filter (Audio EQ Cookbook)
    DelayLine.h           #   Circular buffer with interpolation
    EnvelopeFollower.h    #   Amplitude envelope detector
    LFO.h                 #   Low-frequency oscillator
    Oversampling.h        #   JUCE oversampling wrapper
    Waveshaper.h          #   Nonlinear clipping functions
  plugins/
    Overdrive/            # Each plugin has its own CMakeLists.txt,
    Delay/                #   PluginProcessor.h, and
    Chorus/               #   PluginProcessor.cpp
    Reverb/
    Compressor/
```

The shared DSP library in `common/` provides reusable building blocks that all plugins draw from. Each plugin is a thin layer that wires together the appropriate DSP components with an APVTS parameter layout.

## Roadmap

### Gain & Drive

- [ ] Distortion -- harder clipping character
- [ ] Fuzz -- square-wave-style fuzz with gating
- [ ] Preamp / Amp Sim -- tube amp modeling

### Modulation

- [ ] Flanger -- short swept delay with feedback
- [ ] Phaser -- all-pass filter sweep
- [ ] Vibrato -- pitch modulation without dry mix
- [ ] Tremolo -- amplitude modulation
- [ ] Rotary Speaker -- Leslie cabinet simulation
- [ ] Uni-Vibe -- photocell-style phaser/vibrato
- [ ] Ring Modulator -- carrier frequency multiplication

### Delay

- [ ] Analog Delay -- BBD-style with degradation and filtering
- [ ] Tape Delay -- tape saturation, wow/flutter, and modulated repeats
- [ ] Reverse Delay -- reversed playback delay

### Reverb

- [ ] Plate Reverb -- dense metallic reverb character
- [ ] Spring Reverb -- drip and splash of spring tanks
- [ ] Shimmer Reverb -- octave-shifted reverb tails

### EQ & Filter

- [ ] Graphic EQ -- fixed-band graphic equalizer
- [ ] Parametric EQ -- fully parametric bands
- [ ] Wah -- pedal-controlled bandpass sweep
- [ ] Auto-Wah -- envelope-controlled filter

### Dynamics

- [ ] Noise Gate -- threshold-based signal gate
- [ ] Limiter -- brick-wall peak limiter

### Pitch

- [ ] Octaver -- sub-octave and octave-up generation
- [ ] Pitch Shifter -- chromatic pitch transposition
- [ ] Harmonizer -- intelligent harmony generation
- [ ] Detune / Doubler -- subtle pitch detune for widening

### Utility & Special

- [ ] Tuner -- chromatic tuner display
- [ ] Slow Gear -- auto-swell volume effect
- [ ] Looper -- real-time loop recording and playback
- [ ] Slicer -- rhythmic gating patterns
- [ ] Acoustic Simulator -- electric-to-acoustic tone shaping
- [ ] Humanizer -- vowel formant filter (talk box style)

### Future Export Targets

The project currently builds AU (Audio Unit v2) plugins for macOS. Planned format expansions:

- [ ] **VST3** -- broad DAW compatibility (Ableton Live, FL Studio, Reaper, Cubase, etc.)
- [ ] **AUv3** -- modern Audio Unit format for iOS / iPadOS (GarageBand mobile, AUM, Cubasis)
- [ ] **CLAP** -- open plugin standard gaining traction in Bitwig, Reaper, and others
- [ ] **Standalone** -- self-contained apps with built-in audio I/O for practice and live use
- [ ] **LV2** -- Linux plugin format for Ardour, Carla, and other Linux DAWs

JUCE supports all of these formats from a single codebase, so expanding is largely a matter of adding format flags to the CMake configuration and testing on each platform.

## Technical Details

- **Framework:** JUCE 8.x
- **Language:** C++17
- **Architecture:** Universal Binary (arm64 + x86_64)
- **Minimum macOS:** 11.0 (Big Sur)
- **Plugin format:** AU v2 (`.component` bundles)
- **GUI:** Host-provided generic parameter editors (no custom GUIs)
- **Manufacturer code:** `BsLk`

## License

This project is licensed under the **GNU General Public License v3.0** -- see the [LICENSE](LICENSE) file for details.

JUCE is used under its GPLv3-compatible free license. The JUCE framework itself is included as a git submodule and is subject to its own licensing terms.
