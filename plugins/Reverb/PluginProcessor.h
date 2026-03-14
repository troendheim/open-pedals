/*
 * OpenPedals Reverb — Hall/Room reverb with pre-delay
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

class ReverbProcessor : public juce::AudioProcessor
{
public:
    ReverbProcessor();
    ~ReverbProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float decay, damping, mix, predelay; };

    static constexpr Preset presets[] = {
        // Tight Room: small practice-amp feel, controlled and dry
        { "Tight Room",     0.60f, 0.70f, 0.15f,  5.0f },
        // Studio Room: natural room ambience for recording, keeps clarity
        { "Studio Room",    1.20f, 0.55f, 0.20f, 15.0f },
        // Spring: surf/twang character, moderate decay, high damping for drip
        { "Spring",         1.50f, 0.75f, 0.30f,  0.0f },
        // Warm Hall: concert hall, long and warm with moderate presence
        { "Warm Hall",      3.50f, 0.45f, 0.30f, 30.0f },
        // Cathedral: massive space, long tail, very immersive
        { "Cathedral",      7.00f, 0.25f, 0.40f, 50.0f },
        // Ambient Wash: post-rock/shoegaze, huge reverb dominates the signal
        { "Ambient Wash",   8.50f, 0.20f, 0.60f, 80.0f },
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
    juce::dsp::Reverb reverb;
    openpedals::DelayLine<float> preDelayL, preDelayR;
    juce::AudioBuffer<float> preDelayBuffer; // Pre-allocated buffer for real-time safety

    std::atomic<float>* decayParam    = nullptr;
    std::atomic<float>* dampingParam  = nullptr;
    std::atomic<float>* mixParam      = nullptr;
    std::atomic<float>* preDelayParam = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbProcessor)
};
