/*
 * OpenPedals Plate Reverb — Dense, smooth plate reverb with bright character
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

class PlateReverbProcessor : public juce::AudioProcessor
{
public:
    PlateReverbProcessor();
    ~PlateReverbProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Plate Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float decay, damping, mix, predelay; };

    static constexpr Preset presets[] = {
        // Small Plate: tight, controlled, subtle ambience
        { "Small Plate",    0.80f, 0.45f, 0.20f,  5.0f },
        // Studio Plate: classic studio reverb, balanced and smooth
        { "Studio Plate",   2.50f, 0.30f, 0.35f, 10.0f },
        // Bright Plate: shimmery, airy, lots of high-end presence
        { "Bright Plate",   2.00f, 0.15f, 0.30f,  8.0f },
        // Large Plate: expansive, lush tail
        { "Large Plate",    5.00f, 0.35f, 0.40f, 15.0f },
        // Ambient Plate: huge, enveloping wash for ambient textures
        { "Ambient Plate",  7.50f, 0.20f, 0.55f, 25.0f },
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
    juce::AudioBuffer<float> preDelayBuffer;

    std::atomic<float>* decayParam    = nullptr;
    std::atomic<float>* dampingParam  = nullptr;
    std::atomic<float>* mixParam      = nullptr;
    std::atomic<float>* preDelayParam = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlateReverbProcessor)
};
