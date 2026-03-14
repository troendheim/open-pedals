/*
 * OpenPedals Phaser — 4-stage allpass phaser with LFO modulation
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

class PhaserProcessor : public juce::AudioProcessor
{
public:
    PhaserProcessor();
    ~PhaserProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Phaser"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float rate, depth, feedback, mix; };

    static constexpr Preset presets[] = {
        { "Slow Phase",   0.15f, 0.50f, 0.30f, 0.50f },
        { "Classic Rock", 0.50f, 0.70f, 0.40f, 0.50f },
        { "Fast Swirl",   2.50f, 0.80f, 0.50f, 0.55f },
        { "Deep Space",   0.10f, 1.00f, 0.80f, 0.60f },
        { "Funky",        1.20f, 0.60f, 0.60f, 0.50f },
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

    // DSP — 4 allpass stages per channel
    static constexpr int numStages = 4;
    openpedals::BiquadFilter allpassL[numStages];
    openpedals::BiquadFilter allpassR[numStages];
    openpedals::LFO lfo;

    std::atomic<float>* rateParam     = nullptr;
    std::atomic<float>* depthParam    = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* mixParam      = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    float feedbackL = 0.0f;
    float feedbackR = 0.0f;

    // Frequency range for allpass modulation
    static constexpr float minFreq = 200.0f;
    static constexpr float maxFreq = 5000.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaserProcessor)
};
