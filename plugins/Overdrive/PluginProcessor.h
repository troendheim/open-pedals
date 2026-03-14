/*
 * OpenPedals Overdrive — Tube-style soft-clipping overdrive pedal
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
#include "BiquadFilter.h"
#include "Waveshaper.h"

class OverdriveProcessor : public juce::AudioProcessor
{
public:
    OverdriveProcessor();
    ~OverdriveProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Overdrive"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float drive, tone, level; };

    static constexpr Preset presets[] = {
        // Clean Boost: minimal clipping, full bandwidth, push the amp harder
        { "Clean Boost",   0.08f, 0.75f, 0.85f },
        // Blues: edge of breakup, warm and vocal. Think B.B. King, SRV clean-ish
        { "Blues",         0.30f, 0.45f, 0.55f },
        // Classic Rock: crunchy mids, balanced tone. AC/DC rhythm territory
        { "Classic Rock",  0.50f, 0.50f, 0.50f },
        // Hard Rock: aggressive saturation, slightly brighter for cut
        { "Hard Rock",     0.70f, 0.55f, 0.45f },
        // Lead: heavy sustain for solos, bright for articulation, lower level to compensate gain
        { "Lead",          0.90f, 0.65f, 0.35f },
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

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP
    openpedals::BiquadFilter toneFilterL, toneFilterR;
    juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };

    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* toneParam = nullptr;
    std::atomic<float>* levelParam = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OverdriveProcessor)
};
