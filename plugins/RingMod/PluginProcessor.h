/*
 * OpenPedals Ring Mod — Ring modulation with sine/square carrier
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
#include "LFO.h"

class RingModProcessor : public juce::AudioProcessor
{
public:
    RingModProcessor();
    ~RingModProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Ring Mod"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float frequency, mix, shape; };

    static constexpr Preset presets[] = {
        { "Subtle Bell",   80.0f,  0.30f, 0.00f },
        { "Robot Voice",  200.0f,  0.60f, 0.00f },
        { "Dalek",        400.0f,  0.80f, 1.00f },
        { "Deep Tremolo",  30.0f,  0.50f, 0.00f },
        { "Alien",        800.0f,  0.70f, 0.50f },
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

    // DSP — two carriers for crossfading between sine and square
    openpedals::LFO carrierSine, carrierSquare;

    std::atomic<float>* frequencyParam = nullptr;
    std::atomic<float>* mixParam       = nullptr;
    std::atomic<float>* shapeParam     = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingModProcessor)
};
