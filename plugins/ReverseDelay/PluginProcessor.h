/*
 * OpenPedals Reverse Delay — Reversed playback delay for backwards guitar effects
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
#include <vector>

class ReverseDelayProcessor : public juce::AudioProcessor
{
public:
    ReverseDelayProcessor();
    ~ReverseDelayProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Reverse Delay"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float time, feedback, mix; };

    static constexpr Preset presets[] = {
        // Subtle Reverse: short reverse tail, low in the mix
        { "Subtle Reverse",  300.0f, 0.15f, 0.30f },
        // Ambient Swells: medium time, moderate feedback for evolving textures
        { "Ambient Swells",  600.0f, 0.40f, 0.50f },
        // Full Reverse: long reverse, prominent in the mix, single repeat
        { "Full Reverse",   1000.0f, 0.10f, 0.65f },
        // Infinite Mirror: high feedback, layered reverse reflections
        { "Infinite Mirror",  500.0f, 0.80f, 0.55f },
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

    // Double-buffer for reverse playback — per channel
    static constexpr int maxDelayMs = 2500;
    static constexpr int crossfadeSamples = 64;

    struct ReverseChannel
    {
        std::vector<float> bufferA;
        std::vector<float> bufferB;
        int writePos = 0;
        int readPos = 0;
        int bufferSize = 0;
        bool writeToA = true; // currently writing to A, reading B backwards
    };

    ReverseChannel channelL, channelR;

    void initChannel (ReverseChannel& ch, int maxSamples);
    void resetChannel (ReverseChannel& ch);
    float processChannel (ReverseChannel& ch, float input, int currentBufferSize, float feedback);

    std::atomic<float>* timeParam     = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* mixParam      = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverseDelayProcessor)
};
