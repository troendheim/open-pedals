/*
 * OpenPedals Harmonizer — Adds a pitch-shifted harmony voice to the dry signal
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

class HarmonizerProcessor : public juce::AudioProcessor
{
public:
    HarmonizerProcessor();
    ~HarmonizerProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Harmonizer"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float interval, level, dry; };

    static constexpr Preset presets[] = {
        // Third Up: major third harmony above
        { "Third Up",       4.0f, 0.50f, 0.80f },
        // Fifth Up: classic perfect fifth harmony
        { "Fifth Up",       7.0f, 0.50f, 0.80f },
        // Octave Up: doubled one octave above
        { "Octave Up",     12.0f, 0.45f, 0.80f },
        // Fourth Down: harmony a fourth below
        { "Fourth Down",   -5.0f, 0.50f, 0.80f },
        // Minor Third: dark minor third above
        { "Minor Third",    3.0f, 0.50f, 0.80f },
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

    // DSP — grain-based pitch shifter per channel
    struct ChannelState
    {
        openpedals::DelayLine<float> delayLine;
        int   writePos   = 0;
        float grainPos0  = 0.0f;
        float grainPos1  = 0.0f;
    };

    ChannelState channelState[2];

    std::atomic<float>* intervalParam = nullptr;
    std::atomic<float>* levelParam    = nullptr;
    std::atomic<float>* dryParam      = nullptr;

    double currentSampleRate = 44100.0;
    int grainSize = 2048;
    int currentPreset = 0;

    float processGrainPitchShift (ChannelState& state, float input, float pitchRatio);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonizerProcessor)
};
