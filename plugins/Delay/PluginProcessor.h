/*
 * OpenPedals Delay — Digital delay with feedback and tone control
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
#include "DelayLine.h"
#include "BiquadFilter.h"

class DelayProcessor : public juce::AudioProcessor
{
public:
    DelayProcessor();
    ~DelayProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Delay"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float time, feedback, mix, tone; };

    static constexpr Preset presets[] = {
        // Slapback: short rockabilly/country echo, single repeat, bright
        { "Slapback",        80.0f,  0.05f, 0.45f, 0.80f },
        // Analog Echo: warm, dark repeats like a BBD or tape unit. ~300ms
        { "Analog Echo",    300.0f,  0.45f, 0.35f, 0.30f },
        // Quarter Note: standard rhythmic delay at ~120bpm (500ms), clear repeats
        { "Quarter Note",   500.0f,  0.35f, 0.30f, 0.65f },
        // Dotted Eighth: the U2/Edge rhythm pattern at ~120bpm (375ms)
        { "Dotted Eighth",  375.0f,  0.40f, 0.35f, 0.60f },
        // Long Ambient: spacious trails, dark and receding, subtle in the mix
        { "Long Ambient",   700.0f,  0.60f, 0.25f, 0.25f },
        // Oscillation: near-runaway feedback, bright and aggressive. Use with care!
        { "Oscillation",    250.0f,  0.88f, 0.40f, 0.85f },
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

    // DSP — one delay line and feedback filter per channel
    static constexpr int maxDelayMs = 2000;
    openpedals::DelayLine<float> delayLineL, delayLineR;
    openpedals::BiquadFilter feedbackFilterL, feedbackFilterR;

    float feedbackSampleL = 0.0f;
    float feedbackSampleR = 0.0f;

    std::atomic<float>* timeParam     = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* mixParam      = nullptr;
    std::atomic<float>* toneParam     = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayProcessor)
};
