/*
 * OpenPedals Compressor — Dynamic range compressor
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
#include "EnvelopeFollower.h"

class CompressorProcessor : public juce::AudioProcessor
{
public:
    CompressorProcessor();
    ~CompressorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Compressor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float threshold, ratio, attack, release, makeup; };

    static constexpr Preset presets[] = {
        // Light Touch: transparent leveling, barely noticeable. Gentle single-coil taming
        { "Light Touch",      -12.0f,  2.0f, 15.0f, 150.0f,  2.0f },
        // Studio: balanced all-purpose compression for tracking/mixing
        { "Studio",           -18.0f,  3.5f, 12.0f, 120.0f,  4.0f },
        // Country Squeeze: fast attack, moderate ratio — chicken-pickin' snap
        { "Country Squeeze",  -22.0f,  4.0f,  2.0f,  80.0f,  6.0f },
        // Sustainer: slow attack preserves pick transient, high ratio for singing sustain
        { "Sustainer",        -25.0f,  6.0f, 40.0f, 200.0f,  8.0f },
        // Rhythm Tightener: keeps strumming dynamics even, fast response
        { "Rhythm Tightener", -20.0f,  5.0f,  5.0f,  60.0f,  5.0f },
        // Limiter: brick-wall protection, aggressive clamping
        { "Limiter",          -10.0f, 20.0f,  0.5f,  50.0f,  3.0f },
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
    juce::dsp::Compressor<float> compressor;

    std::atomic<float>* thresholdParam  = nullptr;
    std::atomic<float>* ratioParam      = nullptr;
    std::atomic<float>* attackParam     = nullptr;
    std::atomic<float>* releaseParam    = nullptr;
    std::atomic<float>* makeupGainParam = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorProcessor)
};
