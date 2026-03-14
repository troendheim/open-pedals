/*
 * Bosslike Reverb — Hall/Room reverb with pre-delay
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

class ReverbProcessor : public juce::AudioProcessor
{
public:
    ReverbProcessor();
    ~ReverbProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Bosslike Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }
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
    juce::dsp::Reverb reverb;
    bosslike::DelayLine<float> preDelayL, preDelayR;

    std::atomic<float>* decayParam    = nullptr;
    std::atomic<float>* dampingParam  = nullptr;
    std::atomic<float>* mixParam      = nullptr;
    std::atomic<float>* preDelayParam = nullptr;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbProcessor)
};
