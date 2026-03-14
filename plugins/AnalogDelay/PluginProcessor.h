/*
 * OpenPedals Analog Delay — BBD-style delay with filtered feedback and modulation
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
#include "LFO.h"

class AnalogDelayProcessor : public juce::AudioProcessor
{
public:
    AnalogDelayProcessor();
    ~AnalogDelayProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Analog Delay"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float time, feedback, mix, mod; };

    static constexpr Preset presets[] = {
        // Warm Echo: medium time, moderate feedback, warm and subtle
        { "Warm Echo",          300.0f, 0.40f, 0.35f, 0.20f },
        // Dark Repeats: longer delay, high feedback, heavy filtering, minimal mod
        { "Dark Repeats",       450.0f, 0.65f, 0.30f, 0.10f },
        // Tape-ish: short-medium delay with noticeable wobble
        { "Tape-ish",           250.0f, 0.50f, 0.40f, 0.55f },
        // Modulated: prominent LFO modulation, chorus-like repeats
        { "Modulated",          350.0f, 0.45f, 0.45f, 0.80f },
        // Self-Oscillation: near-runaway feedback, wild and psychedelic
        { "Self-Oscillation",   200.0f, 0.92f, 0.50f, 0.35f },
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

    // DSP — one delay line, feedback filter, and LFO per channel
    static constexpr int maxDelayMs = 1500;
    openpedals::DelayLine<float> delayLineL, delayLineR;
    openpedals::BiquadFilter feedbackFilterL, feedbackFilterR;
    openpedals::LFO lfoL, lfoR;

    float feedbackSampleL = 0.0f;
    float feedbackSampleR = 0.0f;

    std::atomic<float>* timeParam     = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* mixParam      = nullptr;
    std::atomic<float>* modParam      = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogDelayProcessor)
};
