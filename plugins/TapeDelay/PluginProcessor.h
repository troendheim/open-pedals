/*
 * OpenPedals Tape Delay — Tape echo simulation with saturation and flutter
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

class TapeDelayProcessor : public juce::AudioProcessor
{
public:
    TapeDelayProcessor();
    ~TapeDelayProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Tape Delay"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float time, feedback, mix, saturation, flutter; };

    static constexpr Preset presets[] = {
        // Vintage Tape: classic warm tape echo, moderate saturation and flutter
        { "Vintage Tape",    400.0f, 0.40f, 0.35f, 0.45f, 0.25f },
        // Worn Out: heavily degraded tape, lots of saturation, wobbly flutter
        { "Worn Out",        350.0f, 0.55f, 0.40f, 0.80f, 0.70f },
        // Echo Chamber: long clean repeats with gentle tape character
        { "Echo Chamber",    600.0f, 0.50f, 0.30f, 0.25f, 0.15f },
        // Lo-Fi Warble: short delay, heavy flutter and saturation for lo-fi texture
        { "Lo-Fi Warble",    200.0f, 0.45f, 0.50f, 0.70f, 0.85f },
        // Space Echo: long trails, moderate saturation, subtle flutter
        { "Space Echo",      800.0f, 0.65f, 0.35f, 0.50f, 0.30f },
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

    // Tape-style soft clipping
    static float softClip (float x)
    {
        return std::tanh (x);
    }

    // DSP — one delay line, feedback filter, and LFO per channel
    static constexpr int maxDelayMs = 2000;
    openpedals::DelayLine<float> delayLineL, delayLineR;
    openpedals::BiquadFilter feedbackFilterL, feedbackFilterR;
    openpedals::LFO lfoL, lfoR;

    float feedbackSampleL = 0.0f;
    float feedbackSampleR = 0.0f;

    std::atomic<float>* timeParam       = nullptr;
    std::atomic<float>* feedbackParam   = nullptr;
    std::atomic<float>* mixParam        = nullptr;
    std::atomic<float>* saturationParam = nullptr;
    std::atomic<float>* flutterParam    = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeDelayProcessor)
};
