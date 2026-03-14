# Guitar Effects Pedal AU Plugins — Implementation Plan

## Status: Ready to Execute

## Overview
Build 5 guitar effect pedal Audio Unit plugins for GarageBand using JUCE + C++17.
Open source under GPLv3. Universal Binary (arm64 + x86_64).

## Completed Setup
- [x] Xcode 26.0 available
- [x] CMake 3.31.4 downloaded to `/tmp/cmake-3.31.4-macos-universal/CMake.app/Contents/bin/cmake`
- [x] JUCE cloned as git submodule into `./JUCE/`
- [x] Directory structure created: `common/`, `plugins/{Overdrive,Delay,Chorus,Reverb,Compressor}/`

## Files to Create

### 1. Root CMakeLists.txt
- cmake_minimum_required 3.22, project OpenPedalsVSTS, C++17
- CMAKE_OSX_ARCHITECTURES "arm64;x86_64"
- CMAKE_OSX_DEPLOYMENT_TARGET "11.0"
- add_subdirectory(JUCE)
- Header-only OpenPedalsDSP interface library from common/
- Helper function `openpedals_add_plugin()` wrapping `juce_add_plugin()`:
  - COMPANY_NAME "OpenPedals", MANUFACTURER_CODE "OpPd"
  - FORMATS AU only
  - COPY_PLUGIN_AFTER_BUILD TRUE (auto-install)
  - Links: OpenPedalsDSP, juce_audio_utils, juce_dsp
  - Defines: JUCE_WEB_BROWSER=0, JUCE_USE_CURL=0, JUCE_DISPLAY_SPLASH_SCREEN=0
- add_subdirectory for each of the 5 plugins

### 2. Shared DSP Library (common/ — all header-only)

#### common/DelayLine.h
- Template class `DelayLine<typename SampleType>`
- Circular buffer with linear interpolation for fractional delay
- Methods: `setMaxDelay()`, `setDelay()`, `push()`, `get()`, `getInterpolated()`
- Used by: Delay, Chorus, Reverb

#### common/BiquadFilter.h
- Class `BiquadFilter`
- Filter types enum: LowPass, HighPass, BandPass, Peaking, AllPass, LowShelf, HighShelf
- Coefficient calculation from frequency, Q, gain (Audio EQ Cookbook formulas)
- `process(sample)` using Direct Form II Transposed
- `setParameters(type, freq, Q, gainDB, sampleRate)`
- Used by: all plugins for tone shaping

#### common/LFO.h
- Class `LFO`
- Waveforms: Sine, Triangle, Square, Saw
- Phase-accumulator based (no discontinuities)
- Methods: `setFrequency()`, `tick()` returns value in [-1, 1]
- Used by: Chorus, (future: Flanger, Phaser, Tremolo)

#### common/EnvelopeFollower.h
- Class `EnvelopeFollower`
- Peak and RMS detection modes
- Configurable attack/release time constants
- `process(sample)` returns current envelope value
- `setAttack(ms)`, `setRelease(ms)`, `setSampleRate()`
- Used by: Compressor, (future: Gate, Auto-wah)

#### common/Waveshaper.h
- Namespace `Waveshaper` with static functions
- `softClip(x)` — tanh-based soft clipping
- `hardClip(x, threshold)` — hard clip at threshold
- `tubeStyle(x)` — asymmetric soft clipping (different positive/negative curves)
- `sigmoid(x, drive)` — adjustable drive sigmoid
- All functions operate on single samples
- Used by: Overdrive, (future: Distortion, Fuzz)

#### common/Oversampling.h
- Class `Oversampling`
- Uses JUCE's `juce::dsp::Oversampling` internally (wraps it for convenience)
- 2x or 4x oversampling factor
- `prepare(sampleRate, blockSize)`, `upsample(buffer)`, `downsample(buffer)`
- Used by: Overdrive (prevents aliasing from nonlinear waveshaping)

### 3. Plugin: Overdrive (plugins/Overdrive/)

#### plugins/Overdrive/CMakeLists.txt
- Calls `openpedals_add_plugin(TARGET OpenPedalsOverdrive PLUGIN_NAME "OpenPedals Overdrive" PLUGIN_CODE Ovdr)`

#### plugins/Overdrive/PluginProcessor.h / .cpp
- Parameters:
  - Drive: 0.0-1.0 (default 0.5) — maps to input gain 1x-40x
  - Tone: 0.0-1.0 (default 0.5) — lowpass filter cutoff 800Hz-12kHz
  - Level: 0.0-1.0 (default 0.5) — output volume
- DSP chain: Input gain → 4x Oversampling → tanh soft clip → Downsample → BiquadFilter(lowpass) tone → Output level
- Uses GenericAudioProcessorEditor (no custom GUI)

### 4. Plugin: Digital Delay (plugins/Delay/)

#### plugins/Delay/CMakeLists.txt
- `openpedals_add_plugin(TARGET OpenPedalsDelay PLUGIN_NAME "OpenPedals Delay" PLUGIN_CODE DDly)`

#### plugins/Delay/PluginProcessor.h / .cpp
- Parameters:
  - Time: 1-2000ms (default 400ms)
  - Feedback: 0-95% (default 40%)
  - Mix: 0-100% (default 50%)
  - Tone: 0-100% (default 70%) — lowpass cutoff on feedback path 1kHz-20kHz
- DSP: DelayLine with feedback loop, BiquadFilter in feedback path, dry/wet mix

### 5. Plugin: Chorus (plugins/Chorus/)

#### plugins/Chorus/CMakeLists.txt
- `openpedals_add_plugin(TARGET OpenPedalsChorus PLUGIN_NAME "OpenPedals Chorus" PLUGIN_CODE Chrs)`

#### plugins/Chorus/PluginProcessor.h / .cpp
- Parameters:
  - Rate: 0.1-10 Hz (default 1.5 Hz)
  - Depth: 0-100% (default 50%) — modulation depth in ms (0-7ms range)
  - Mix: 0-100% (default 50%)
- DSP: 20ms max delay line, LFO (sine) modulates delay time around center point, mix wet with dry

### 6. Plugin: Reverb (plugins/Reverb/)

#### plugins/Reverb/CMakeLists.txt
- `openpedals_add_plugin(TARGET OpenPedalsReverb PLUGIN_NAME "OpenPedals Reverb" PLUGIN_CODE Rvrb)`

#### plugins/Reverb/PluginProcessor.h / .cpp
- Parameters:
  - Decay: 0.1-10s (default 2.0s)
  - Damping: 0-100% (default 50%) — high-frequency absorption
  - Mix: 0-100% (default 30%)
  - PreDelay: 0-100ms (default 20ms)
- DSP: Uses JUCE's `juce::dsp::Reverb` (Freeverb-based algorithm) which provides room size, damping, wet/dry. Pre-delay via a short DelayLine. This is a pragmatic choice — Freeverb sounds good and is well-tested.

### 7. Plugin: Compressor (plugins/Compressor/)

#### plugins/Compressor/CMakeLists.txt
- `openpedals_add_plugin(TARGET OpenPedalsCompressor PLUGIN_NAME "OpenPedals Compressor" PLUGIN_CODE Comp)`

#### plugins/Compressor/PluginProcessor.h / .cpp
- Parameters:
  - Threshold: -60 to 0 dB (default -20 dB)
  - Ratio: 1.0-20.0 (default 4.0)
  - Attack: 0.1-100ms (default 10ms)
  - Release: 10-1000ms (default 100ms)
  - MakeupGain: 0-30 dB (default 0 dB)
- DSP: Uses JUCE's `juce::dsp::Compressor` for the core algorithm, plus makeup gain stage

## Build & Install Commands

```bash
export PATH="/tmp/cmake-3.31.4-macos-universal/CMake.app/Contents/bin:$PATH"

# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build all plugins
cmake --build build --config Release -j$(sysctl -n hw.ncpu)

# Plugins are auto-installed by COPY_PLUGIN_AFTER_BUILD
# They'll appear in ~/Library/Audio/Plug-Ins/Components/

# Validate
auval -a | grep -i openpedals
```

## Plugin Registration Details (AU Component Descriptions)
| Plugin | Type | Subtype | Manufacturer |
|--------|------|---------|-------------|
| Overdrive | aufx | Ovdr | OpPd |
| Delay | aufx | DDly | OpPd |
| Chorus | aufx | Chrs | OpPd |
| Reverb | aufx | Rvrb | OpPd |
| Compressor | aufx | Comp | OpPd |

## License
GPLv3 (required by JUCE's free license)

## Future Expansion (Phase 4+)
Remaining 30 pedals to implement using the shared DSP library:
- Distortion, Fuzz, Preamp/Amp Sim
- Flanger, Phaser, Vibrato, Tremolo, Rotary, Uni-Vibe, Ring Mod
- Analog Delay, Tape Delay, Reverse Delay
- Plate Reverb, Spring Reverb, Shimmer Reverb
- EQ (Graphic + Parametric), Wah, Auto-Wah, Noise Gate, Limiter
- Octaver, Pitch Shifter, Harmonizer, Detune
- Tuner, Slow Gear, Looper, Slicer, Acoustic Sim, Humanizer
