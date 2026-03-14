/*
 * OpenPedals Rotary — Leslie speaker simulation with horn and drum
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

class RotaryProcessor : public juce::AudioProcessor
{
public:
    RotaryProcessor();
    ~RotaryProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Rotary"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float speed, depth, mix; };

    static constexpr Preset presets[] = {
        { "Slow & Easy",     0.10f, 0.50f, 0.60f },
        { "Classic Fast",    0.80f, 0.70f, 0.70f },
        { "Gentle Warble",   0.20f, 0.40f, 0.50f },
        { "Full Spin",       1.00f, 1.00f, 0.80f },
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

    // DSP — horn and drum LFOs + delay lines for pitch modulation
    openpedals::LFO hornLfo, drumLfo;
    openpedals::DelayLine<float> hornDelay, drumDelay;

    std::atomic<float>* speedParam = nullptr;
    std::atomic<float>* depthParam = nullptr;
    std::atomic<float>* mixParam   = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    // Leslie speed ranges
    static constexpr float hornSlowHz = 0.8f;
    static constexpr float hornFastHz = 6.0f;
    static constexpr float drumSlowHz = 0.7f;
    static constexpr float drumFastHz = 5.0f;

    static constexpr float centerDelayMs = 3.0f;
    static constexpr float maxDepthMs    = 2.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotaryProcessor)
};
