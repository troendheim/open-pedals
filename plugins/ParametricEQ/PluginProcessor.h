/*
 * OpenPedals Parametric EQ — 3-band fully parametric equalizer
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

class ParametricEQProcessor : public juce::AudioProcessor
{
public:
    ParametricEQProcessor();
    ~ParametricEQProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Parametric EQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float freq1, gain1, q1, freq2, gain2, q2, freq3, gain3, q3; };

    static constexpr Preset presets[] = {
        // Flat: all bands at unity
        { "Flat",          100.0f, 0.0f, 1.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f, 1.0f },
        // Vocal Cut: notch out vocal range for instrumental clarity
        { "Vocal Cut",     100.0f, 0.0f, 1.0f, 1500.0f, -8.0f, 2.0f, 5000.0f, 0.0f, 1.0f },
        // Mid Focus: boost mids for lead guitar cut-through
        { "Mid Focus",     250.0f, -2.0f, 1.0f, 800.0f, 6.0f, 1.5f, 5000.0f, 2.0f, 1.0f },
        // High Cut: roll off harsh highs, warm the tone
        { "High Cut",      100.0f, 2.0f, 0.8f, 1000.0f, 0.0f, 1.0f, 4000.0f, -10.0f, 0.7f },
        // Smiley Face: boost lows and highs, scoop mids
        { "Smiley Face",   80.0f, 6.0f, 0.8f, 700.0f, -4.0f, 1.0f, 8000.0f, 5.0f, 0.8f },
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

    // DSP — 3 parametric bands, stereo
    static constexpr int numBands = 3;
    openpedals::BiquadFilter filtersL[numBands];
    openpedals::BiquadFilter filtersR[numBands];

    std::atomic<float>* freqParams[numBands]  = {};
    std::atomic<float>* gainParams[numBands]  = {};
    std::atomic<float>* qParams[numBands]     = {};

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametricEQProcessor)
};
