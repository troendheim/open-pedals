/*
 * OpenPedals Detune — Stereo pitch-detuning for thickening and widening
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

class DetuneProcessor : public juce::AudioProcessor
{
public:
    DetuneProcessor();
    ~DetuneProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Detune"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float detune, mix; };

    static constexpr Preset presets[] = {
        // Subtle Wide: gentle widening, barely perceptible pitch shift
        { "Subtle Wide",     5.0f, 0.40f },
        // Thick: noticeable thickening, good for leads
        { "Thick",          15.0f, 0.50f },
        // 12-String: simulates a 12-string guitar doubling
        { "12-String",      10.0f, 0.60f },
        // Extreme Spread: wide stereo with heavy detuning
        { "Extreme Spread", 40.0f, 0.50f },
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

    // DSP — one grain shifter per channel (L shifts down, R shifts up)
    struct ChannelState
    {
        openpedals::DelayLine<float> delayLine;
        int   writePos   = 0;
        float grainPos0  = 0.0f;
        float grainPos1  = 0.0f;
    };

    ChannelState channelStateL, channelStateR;

    std::atomic<float>* detuneParam = nullptr;
    std::atomic<float>* mixParam    = nullptr;

    double currentSampleRate = 44100.0;
    int grainSize = 2048;
    int currentPreset = 0;

    float processGrainPitchShift (ChannelState& state, float input, float pitchRatio);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DetuneProcessor)
};
