/*
 * Bosslike Chorus — Classic chorus effect with LFO-modulated delay
 *
 * Part of the Bosslike guitar effects plugin collection.
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
    const juce::String getName() const override { return "Bosslike Chorus"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP
    bosslike::DelayLine<float> delayLineL, delayLineR;
    bosslike::LFO lfoL, lfoR;

    std::atomic<float>* rateParam  = nullptr;
    std::atomic<float>* depthParam = nullptr;
    std::atomic<float>* mixParam   = nullptr;

    double currentSampleRate = 44100.0;

    // Center delay in ms and max modulation depth in ms
    static constexpr float centerDelayMs = 7.0f;
    static constexpr float maxDepthMs    = 7.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusProcessor)
};
