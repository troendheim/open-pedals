/*
 * OpenPedals Acoustic Sim — Electric-to-acoustic guitar tone shaping
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

class AcousticSimProcessor : public juce::AudioProcessor
{
public:
    AcousticSimProcessor();
    ~AcousticSimProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Acoustic Sim"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float body, top, level; };

    static constexpr Preset presets[] = {
        // Dreadnought: full-body acoustic with strong low-mid resonance
        { "Dreadnought",    0.8f, 0.4f, 0.5f },
        // Classical: warm nylon-string character, less top end
        { "Classical",       0.5f, 0.2f, 0.5f },
        // Bright Acoustic: crisp fingerpicking tone with sparkle
        { "Bright Acoustic", 0.3f, 0.8f, 0.5f },
        // Warm Body: emphasis on body resonance, rolled-off highs
        { "Warm Body",       0.9f, 0.1f, 0.5f },
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

    // DSP — EQ filters per channel
    // Body resonance boost (peaking around 300Hz)
    openpedals::BiquadFilter bodyFilterL, bodyFilterR;
    // Upper-mid cut (peaking around 2.5kHz, negative gain)
    openpedals::BiquadFilter midCutFilterL, midCutFilterR;
    // Top / presence boost (high shelf around 6kHz)
    openpedals::BiquadFilter topFilterL, topFilterR;
    // Lowpass rolloff above 10kHz
    openpedals::BiquadFilter lpFilterL, lpFilterR;

    std::atomic<float>* bodyParam  = nullptr;
    std::atomic<float>* topParam   = nullptr;
    std::atomic<float>* levelParam = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcousticSimProcessor)
};
