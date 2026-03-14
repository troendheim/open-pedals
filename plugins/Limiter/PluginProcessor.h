/*
 * OpenPedals Limiter — Brick-wall limiter for clipping prevention
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

class LimiterProcessor : public juce::AudioProcessor
{
public:
    LimiterProcessor();
    ~LimiterProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Limiter"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float threshold, release, ceiling; };

    static constexpr Preset presets[] = {
        // Transparent: gentle limiting, barely touching the signal
        { "Transparent",   -3.0f, 200.0f,  0.0f },
        // Loud: push the volume, aggressive threshold
        { "Loud",         -10.0f,  50.0f, -0.3f },
        // Brick Wall: hard limiting at a low threshold
        { "Brick Wall",   -6.0f,   20.0f, -0.1f },
        // Gentle Limit: soft catch, long release for transparent control
        { "Gentle Limit",  -1.0f, 500.0f,  0.0f },
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

    // DSP
    juce::dsp::Limiter<float> limiter;

    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* releaseParam   = nullptr;
    std::atomic<float>* ceilingParam   = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LimiterProcessor)
};
