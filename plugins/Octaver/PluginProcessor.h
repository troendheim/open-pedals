/*
 * OpenPedals Octaver — Sub-octave and octave-up voice generator
 *
 * Part of the OpenPedals guitar effects plugin collection.
 * Copyright (C) 2026 Richard Troendheim
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "BiquadFilter.h"

class OctaverProcessor : public juce::AudioProcessor
{
public:
    OctaverProcessor();
    ~OctaverProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Octaver"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float octDown, octUp, dry; };

    static constexpr Preset presets[] = {
        // Sub Bass: heavy sub-octave, no octave up, moderate dry
        { "Sub Bass",        0.80f, 0.00f, 0.40f },
        // Organ: balanced sub and dry for an organ-like tone
        { "Organ",           0.50f, 0.00f, 0.70f },
        // Full Range: all three voices for a massive sound
        { "Full Range",      0.50f, 0.40f, 0.60f },
        // Synth Bass: heavy sub with reduced dry for synth-like bass
        { "Synth Bass",      0.90f, 0.00f, 0.20f },
        // Octave Up Only: pure octave-up ring-mod shimmer
        { "Octave Up Only",  0.00f, 0.70f, 0.50f },
    };

    static constexpr int numPresets = static_cast<int> (sizeof (presets) / sizeof (presets[0]));

    int getNumPrograms() override { return numPresets; }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override
    {
        return (index >= 0 && index < numPresets) ? presets[index].name : juce::String();
    }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP — per-channel state for octave tracking
    struct ChannelState
    {
        // Octave-down tracking
        float prevSample = 0.0f;
        bool  flipFlop   = false;   // divides zero-crossing frequency by 2
        float subSquare  = 0.0f;    // current sub-octave square wave value

        // Filters
        openpedals::BiquadFilter subLowPass;     // smooth the sub square wave
        openpedals::BiquadFilter octUpBandPass;   // clean up the octave-up signal
    };

    ChannelState channelState[2];

    std::atomic<float>* octDownParam = nullptr;
    std::atomic<float>* octUpParam   = nullptr;
    std::atomic<float>* dryParam     = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctaverProcessor)
};
