/*
 * OpenPedals Graphic EQ — 7-band graphic equalizer
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

class GraphicEQProcessor : public juce::AudioProcessor
{
public:
    GraphicEQProcessor();
    ~GraphicEQProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Graphic EQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float b100, b200, b400, b800, b1k6, b3k2, b6k4; };

    static constexpr Preset presets[] = {
        // Flat: all bands at unity
        { "Flat",            0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
        // Mid Scoop: classic V-shape for heavy rhythm guitar
        { "Mid Scoop",       4.0f,  1.0f, -3.0f, -5.0f, -3.0f,  2.0f,  4.0f },
        // Presence Boost: push upper mids and highs for cut-through
        { "Presence Boost",  0.0f,  0.0f,  0.0f,  2.0f,  5.0f,  4.0f,  2.0f },
        // Bass Boost: warm low-end lift for jazz or clean tones
        { "Bass Boost",      6.0f,  4.0f,  1.0f,  0.0f, -1.0f, -2.0f, -2.0f },
        // Treble Cut: roll off highs, darken the tone
        { "Treble Cut",      0.0f,  0.0f,  0.0f, -1.0f, -3.0f, -6.0f, -9.0f },
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

    // DSP — 7 bands, stereo (L/R)
    static constexpr int numBands = 7;
    static constexpr float bandFrequencies[numBands] = { 100.0f, 200.0f, 400.0f, 800.0f, 1600.0f, 3200.0f, 6400.0f };
    static constexpr float bandQ = 1.4f;

    openpedals::BiquadFilter filtersL[numBands];
    openpedals::BiquadFilter filtersR[numBands];

    std::atomic<float>* bandParams[numBands] = {};

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    static constexpr const char* bandIDs[numBands] = { "band100", "band200", "band400", "band800", "band1k6", "band3k2", "band6k4" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphicEQProcessor)
};
