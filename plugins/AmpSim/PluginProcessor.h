/*
 * OpenPedals Amp Sim — Preamp, tone stack, power amp and cabinet simulation
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
#include "Waveshaper.h"
#include "ToneStack.h"
#include "CabinetSim.h"

class AmpSimProcessor : public juce::AudioProcessor
{
public:
    AmpSimProcessor();
    ~AmpSimProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OP Amp Sim"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Factory presets
    struct Preset { const char* name; float gain, bass, mid, treble, volume, presence; };

    static constexpr Preset presets[] = {
        // Clean Channel: low gain, flat EQ, gentle power amp, bright cabinet
        { "Clean Channel",    0.15f,  0.0f,  0.0f,  0.0f, 0.60f, 0.60f },
        // Crunch: edge of breakup, slight mid boost, warm presence
        { "Crunch",           0.35f,  1.0f,  2.0f, -1.0f, 0.55f, 0.50f },
        // British Stack: Marshall-style crunch, scooped mids, bright top
        { "British Stack",    0.60f,  2.0f, -3.0f,  4.0f, 0.45f, 0.55f },
        // American Lead: Mesa-style high gain, tight bass, aggressive mids
        { "American Lead",    0.75f, -2.0f,  4.0f,  2.0f, 0.40f, 0.50f },
        // Modern Metal: extreme gain, scooped mids, tight low end
        { "Modern Metal",     0.90f,  3.0f, -4.0f,  5.0f, 0.35f, 0.45f },
        // Boutique: smooth gain, rich mids, warm presence, organic feel
        { "Boutique",         0.45f,  1.0f,  3.0f,  1.0f, 0.50f, 0.65f },
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
    openpedals::ToneStack toneStackL, toneStackR;
    openpedals::CabinetSim cabinetL, cabinetR;
    juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };

    std::atomic<float>* gainParam     = nullptr;
    std::atomic<float>* bassParam     = nullptr;
    std::atomic<float>* midParam      = nullptr;
    std::atomic<float>* trebleParam   = nullptr;
    std::atomic<float>* volumeParam   = nullptr;
    std::atomic<float>* presenceParam = nullptr;

    double currentSampleRate = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmpSimProcessor)
};
