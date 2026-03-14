/*
 * OpenPedals Auto-Wah — Envelope-controlled bandpass filter
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
#include "EnvelopeFollower.h"

class AutoWahProcessor : public juce::AudioProcessor
{
public:
    AutoWahProcessor();
    ~AutoWahProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Auto-Wah"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float sensitivity, range, speed, mix; };

    static constexpr Preset presets[] = {
        // Subtle Quack: gentle envelope response, narrow range
        { "Subtle Quack",    0.35f, 0.40f, 15.0f, 0.60f },
        // Funk Guitar: classic auto-wah for funky rhythm playing
        { "Funk Guitar",     0.55f, 0.65f, 10.0f, 0.80f },
        // Synth Wah: wide range, high sensitivity for synth-like sweeps
        { "Synth Wah",       0.75f, 0.90f,  5.0f, 0.90f },
        // Slow Filter: slow attack, smooth filter movement
        { "Slow Filter",     0.50f, 0.50f, 60.0f, 0.70f },
        // Extreme Quack: maximum quack, fast and aggressive
        { "Extreme Quack",   0.90f, 1.00f,  3.0f, 1.00f },
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
    openpedals::BiquadFilter filterL, filterR;
    openpedals::EnvelopeFollower envFollowerL, envFollowerR;

    std::atomic<float>* sensitivityParam = nullptr;
    std::atomic<float>* rangeParam       = nullptr;
    std::atomic<float>* speedParam       = nullptr;
    std::atomic<float>* mixParam         = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    // Frequency range constants
    static constexpr float baseFreq = 350.0f;
    static constexpr float freqRange = 4650.0f;  // baseFreq + freqRange = 5kHz max
    static constexpr float filterQ = 4.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoWahProcessor)
};
