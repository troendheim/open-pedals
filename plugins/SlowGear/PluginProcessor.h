/*
 * OpenPedals Slow Gear — Auto-swell volume effect for violin-like attacks
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

class SlowGearProcessor : public juce::AudioProcessor
{
public:
    SlowGearProcessor();
    ~SlowGearProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Slow Gear"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float sensitivity, attack, level; };

    static constexpr Preset presets[] = {
        // Subtle Swell: gentle fade-in, high threshold — only catches hard picks
        { "Subtle Swell",  -20.0f,  300.0f, 0.7f },
        // Violin: classic slow-gear sound, medium sensitivity and attack
        { "Violin",        -30.0f,  500.0f, 0.8f },
        // Slow Pad: very slow attack for ambient pad-like swells
        { "Slow Pad",      -35.0f, 1500.0f, 0.6f },
        // Fast Response: quick attack, sensitive — subtle pick-attack softening
        { "Fast Response", -25.0f,   80.0f, 0.9f },
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
    openpedals::EnvelopeFollower envelopeFollower;

    std::atomic<float>* sensitivityParam = nullptr;
    std::atomic<float>* attackParam      = nullptr;
    std::atomic<float>* levelParam       = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    // Swell state
    float swellGain = 0.0f;     // Current gain ramp value (0..1)
    bool swellActive = false;   // Whether a swell is in progress
    float attackIncrement = 0.0f; // Per-sample increment during swell

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlowGearProcessor)
};
