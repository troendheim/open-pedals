/*
 * OpenPedals Shimmer Reverb — Reverb with pitch-shifted feedback for ethereal pads
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
#include "BiquadFilter.h"

class ShimmerReverbProcessor : public juce::AudioProcessor
{
public:
    ShimmerReverbProcessor();
    ~ShimmerReverbProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Shimmer Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float decay, shimmer, mix, tone; };

    static constexpr Preset presets[] = {
        // Ethereal: balanced shimmer with moderate decay, dreamy
        { "Ethereal",           4.00f, 0.50f, 0.40f, 0.60f },
        // Cathedral Shimmer: long reverb with strong shimmer, massive space
        { "Cathedral Shimmer",  8.00f, 0.70f, 0.50f, 0.55f },
        // Pad Wash: very long, heavily shimmered, ambient pad texture
        { "Pad Wash",           9.00f, 0.80f, 0.60f, 0.45f },
        // Subtle Shine: gentle shimmer, keeps clarity
        { "Subtle Shine",       3.00f, 0.25f, 0.30f, 0.70f },
        // Frozen: infinite-feeling, dense shimmer wash
        { "Frozen",             10.0f, 0.90f, 0.55f, 0.40f },
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

    // Pitch shift: two delay lines per channel for grain-based octave-up
    openpedals::DelayLine<float> grainDelayL1, grainDelayL2;
    openpedals::DelayLine<float> grainDelayR1, grainDelayR2;

    // Tone filter on shimmer path
    openpedals::BiquadFilter toneFilterL, toneFilterR;

    std::atomic<float>* decayParam   = nullptr;
    std::atomic<float>* shimmerParam = nullptr;
    std::atomic<float>* mixParam     = nullptr;
    std::atomic<float>* toneParam    = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    // Pitch shifter state
    double grainPhase = 0.0;        // 0..1 ramp for grain crossfade
    int grainSize = 0;              // grain size in samples
    float shimmerFeedbackL = 0.0f;  // feedback accumulator
    float shimmerFeedbackR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShimmerReverbProcessor)
};
