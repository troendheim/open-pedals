/*
 * OpenPedals Uni-Vibe — Vintage phase shifter with non-uniform allpass spacing
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
#include "LFO.h"

class UniVibeProcessor : public juce::AudioProcessor
{
public:
    UniVibeProcessor();
    ~UniVibeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Uni-Vibe"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float speed, intensity, volume; };

    static constexpr Preset presets[] = {
        { "Chorus Mode",     2.0f, 0.40f, 0.50f },
        { "Vibrato Mode",    5.0f, 0.90f, 0.50f },
        { "Classic Hendrix", 3.5f, 0.70f, 0.55f },
        { "Slow Throb",      0.8f, 0.80f, 0.50f },
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

    // DSP — 4 allpass stages per channel with staggered center frequencies
    static constexpr int numStages = 4;
    openpedals::BiquadFilter allpassL[numStages];
    openpedals::BiquadFilter allpassR[numStages];
    openpedals::LFO lfo;

    // Staggered center frequencies for non-uniform allpass spacing
    static constexpr float stageFreqs[numStages] = { 150.0f, 500.0f, 1500.0f, 4000.0f };
    // Different modulation depth per stage for organic sound
    static constexpr float stageDepths[numStages] = { 1.0f, 0.8f, 0.6f, 0.4f };

    std::atomic<float>* speedParam     = nullptr;
    std::atomic<float>* intensityParam = nullptr;
    std::atomic<float>* volumeParam    = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UniVibeProcessor)
};
