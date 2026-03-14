/*
 * OpenPedals Pitch Shifter — Granular pitch shifter with two crossfaded grains
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

class PitchShifterProcessor : public juce::AudioProcessor
{
public:
    PitchShifterProcessor();
    ~PitchShifterProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Pitch Shifter"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float semitones, fine, mix; };

    static constexpr Preset presets[] = {
        // Octave Down: one octave below
        { "Octave Down",    -12.0f,  0.0f, 0.50f },
        // Fifth Up: classic harmony interval
        { "Fifth Up",         7.0f,  0.0f, 0.50f },
        // Fourth Down: down a fourth
        { "Fourth Down",     -5.0f,  0.0f, 0.50f },
        // Subtle Detune: slight pitch offset for doubling effect
        { "Subtle Detune",    0.0f, 12.0f, 0.40f },
        // Octave Up: one octave above
        { "Octave Up",       12.0f,  0.0f, 0.50f },
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
        float grainPos0  = 0.0f;   // read position offset for grain 0
        float grainPos1  = 0.0f;   // read position offset for grain 1
    };

    ChannelState channelState[2];

    std::atomic<float>* semitonesParam = nullptr;
    std::atomic<float>* fineParam      = nullptr;
    std::atomic<float>* mixParam       = nullptr;

    double currentSampleRate = 44100.0;
    int grainSize = 2048;
    int currentPreset = 0;

    float processGrainPitchShift (ChannelState& state, float input, float pitchRatio);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchShifterProcessor)
};
