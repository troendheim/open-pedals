/*
 * OpenPedals Wah — Manual wah pedal with automatable position
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

class WahProcessor : public juce::AudioProcessor
{
public:
    WahProcessor();
    ~WahProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Wah"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float position, q, volume; };

    static constexpr Preset presets[] = {
        // Half Cocked: classic parked-wah midrange honk
        { "Half Cocked",    0.50f,  5.0f, 0.50f },
        // Parked Bass: toe-up, warm bass emphasis
        { "Parked Bass",    0.15f,  4.0f, 0.55f },
        // Parked Treble: toe-down, bright and nasal
        { "Parked Treble",  0.85f,  5.0f, 0.45f },
        // Narrow Sweep: high Q for vocal, focused tone
        { "Narrow Sweep",   0.50f, 12.0f, 0.50f },
        // Wide Sweep: low Q for broad, subtle filtering
        { "Wide Sweep",     0.50f,  2.0f, 0.55f },
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

    std::atomic<float>* positionParam = nullptr;
    std::atomic<float>* qParam        = nullptr;
    std::atomic<float>* volumeParam   = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    // Frequency range: 350 Hz to 5 kHz (exponential mapping via position)
    static constexpr float minFreq = 350.0f;
    static constexpr float maxFreq = 5000.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WahProcessor)
};
