/*
 * OpenPedals Noise Gate — Noise gate with attack, hold, and release
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
#include "EnvelopeFollower.h"

class NoiseGateProcessor : public juce::AudioProcessor
{
public:
    NoiseGateProcessor();
    ~NoiseGateProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Noise Gate"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float threshold, attack, hold, release; };

    static constexpr Preset presets[] = {
        // Gentle Gate: high threshold, slow transitions — transparent noise removal
        { "Gentle Gate",       -50.0f,  5.0f, 100.0f, 150.0f },
        // Tight Gate: fast attack/release, short hold — clean stop between notes
        { "Tight Gate",        -40.0f,  0.5f,  30.0f,  30.0f },
        // Heavy Metal Gate: aggressive gating for high-gain rhythm
        { "Heavy Metal Gate",  -35.0f,  0.2f,  20.0f,  20.0f },
        // Noise Reducer: conservative settings, just removes hiss
        { "Noise Reducer",     -60.0f, 10.0f, 200.0f, 250.0f },
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
    openpedals::EnvelopeFollower envFollower;

    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* attackParam    = nullptr;
    std::atomic<float>* holdParam      = nullptr;
    std::atomic<float>* releaseParam   = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    // Gate state
    float gateGain = 0.0f;          // current gate gain (0 = closed, 1 = open)
    int holdCounter = 0;            // samples remaining in hold phase
    bool gateOpen = false;          // whether the gate is currently open

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoiseGateProcessor)
};
