/*
 * OpenPedals Humanizer — Formant filter for vowel-shaping effects
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

class HumanizerProcessor : public juce::AudioProcessor
{
public:
    HumanizerProcessor();
    ~HumanizerProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Humanizer"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float vowel, resonance, mix; };

    static constexpr Preset presets[] = {
        // Talk Box: mid vowel with moderate resonance, classic talkbox vibe
        { "Talk Box",     0.5f, 0.6f, 0.7f },
        // Robot Vowel: high resonance for synthetic robotic character
        { "Robot Vowel",  0.25f, 0.9f, 0.8f },
        // A Shape: locked on the "A" vowel
        { "A Shape",      0.0f, 0.5f, 0.6f },
        // O Shape: locked on the "O" vowel
        { "O Shape",      0.75f, 0.5f, 0.6f },
        // Morphing: mid position, lower resonance for smooth sweeps
        { "Morphing",     0.5f, 0.3f, 0.5f },
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

    // DSP — 2 formant bandpass filters per channel
    openpedals::BiquadFilter formant1L, formant1R;
    openpedals::BiquadFilter formant2L, formant2R;

    std::atomic<float>* vowelParam     = nullptr;
    std::atomic<float>* resonanceParam = nullptr;
    std::atomic<float>* mixParam       = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    // Vowel formant frequencies: A, E, I, O, U
    // Each vowel has F1 (low formant) and F2 (high formant)
    static constexpr int numVowels = 5;
    static constexpr float vowelF1[numVowels] = { 800.0f, 400.0f, 300.0f, 500.0f, 350.0f };
    static constexpr float vowelF2[numVowels] = { 1200.0f, 2200.0f, 2800.0f, 800.0f, 700.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HumanizerProcessor)
};
