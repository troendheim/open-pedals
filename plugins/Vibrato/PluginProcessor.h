/*
 * OpenPedals Vibrato — Pure pitch modulation via LFO-modulated delay
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
#include "LFO.h"

class VibratoProcessor : public juce::AudioProcessor
{
public:
    VibratoProcessor();
    ~VibratoProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Vibrato"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float rate, depth; };

    static constexpr Preset presets[] = {
        { "Subtle",   3.0f, 0.20f },
        { "Classic",  5.0f, 0.50f },
        { "Wide",     6.0f, 0.75f },
        { "Extreme",  8.0f, 1.00f },
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
    openpedals::DelayLine<float> delayLineL, delayLineR;
    openpedals::LFO lfoL, lfoR;

    std::atomic<float>* rateParam  = nullptr;
    std::atomic<float>* depthParam = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    static constexpr float centerDelayMs = 5.0f;
    static constexpr float maxDepthMs    = 3.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibratoProcessor)
};
