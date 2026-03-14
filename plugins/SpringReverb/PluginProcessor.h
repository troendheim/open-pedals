/*
 * OpenPedals Spring Reverb — Spring reverb with metallic drip character
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

class SpringReverbProcessor : public juce::AudioProcessor
{
public:
    SpringReverbProcessor();
    ~SpringReverbProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Spring Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float decay, tone, mix, drip; };

    static constexpr Preset presets[] = {
        // Surf: classic surf-rock spring, lots of drip, bright and splashy
        { "Surf",           2.00f, 0.70f, 0.40f, 0.80f },
        // Vintage Amp: warm, subtle spring like a Fender combo amp
        { "Vintage Amp",    1.50f, 0.40f, 0.25f, 0.40f },
        // Dark Spring: mellow, rolled-off highs, smooth decay
        { "Dark Spring",    1.80f, 0.20f, 0.30f, 0.30f },
        // Bright Spring: sparkling, present, snappy response
        { "Bright Spring",  1.20f, 0.80f, 0.35f, 0.60f },
        // Drippy: exaggerated spring character, maximum metallic resonance
        { "Drippy",         2.50f, 0.55f, 0.45f, 1.00f },
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

    // Pre-filter: bandpass to emphasize spring character on the input
    openpedals::BiquadFilter preFilterL, preFilterR;

    // Drip filter: resonant bandpass on the wet signal for metallic quality
    openpedals::BiquadFilter dripFilterL, dripFilterR;

    // Tone filter: lowpass on wet signal
    openpedals::BiquadFilter toneFilterL, toneFilterR;

    // Allpass filters to add metallic diffusion before the reverb
    openpedals::BiquadFilter allpassL1, allpassL2;
    openpedals::BiquadFilter allpassR1, allpassR2;

    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* toneParam  = nullptr;
    std::atomic<float>* mixParam   = nullptr;
    std::atomic<float>* dripParam  = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpringReverbProcessor)
};
