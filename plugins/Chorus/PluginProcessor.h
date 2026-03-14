/*
 * OpenPedals Chorus — Classic chorus effect with LFO-modulated delay
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

class ChorusProcessor : public juce::AudioProcessor
{
public:
    ChorusProcessor();
    ~ChorusProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Chorus"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float rate, depth, mix; };

    static constexpr Preset presets[] = {
        // Subtle Shimmer: barely-there movement, adds life without obvious modulation
        { "Subtle Shimmer",  0.40f, 0.20f, 0.25f },
        // Classic: the Roland JC-120 sound. Smooth, musical, the gold standard
        { "Classic",         0.80f, 0.40f, 0.50f },
        // 80s Clean: faster, deeper — think The Police, The Cure rhythm parts
        { "80s Clean",       1.80f, 0.55f, 0.55f },
        // Thick Detune: very slow, deep. Acts like a doubler/thickener
        { "Thick Detune",    0.15f, 0.70f, 0.60f },
        // Vibrato: 100% wet, fast modulation — pitch wobble, no dry signal
        { "Vibrato",         5.50f, 0.80f, 1.00f },
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
    std::atomic<float>* mixParam   = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    // Center delay in ms and max modulation depth in ms
    static constexpr float centerDelayMs = 7.0f;
    static constexpr float maxDepthMs    = 7.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusProcessor)
};
