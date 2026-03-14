/*
 * OpenPedals Uni-Vibe — Vintage phase shifter with non-uniform allpass spacing
 *
 * Part of the OpenPedals guitar effects plugin collection.
 * Copyright (C) 2026 Richard Troendheim
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "PluginProcessor.h"

UniVibeProcessor::UniVibeProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    speedParam     = apvts.getRawParameterValue ("speed");
    intensityParam = apvts.getRawParameterValue ("intensity");
    volumeParam    = apvts.getRawParameterValue ("volume");

    setCurrentProgram (0);
}

void UniVibeProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("speed")->setValueNotifyingHost (apvts.getParameter ("speed")->convertTo0to1 (p.speed));
    apvts.getParameter ("intensity")->setValueNotifyingHost (apvts.getParameter ("intensity")->convertTo0to1 (p.intensity));
    apvts.getParameter ("volume")->setValueNotifyingHost (apvts.getParameter ("volume")->convertTo0to1 (p.volume));
}

juce::AudioProcessorValueTreeState::ParameterLayout UniVibeProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "speed", 1 }, "Speed",
        juce::NormalisableRange<float> (0.5f, 10.0f, 0.01f, 0.5f), 2.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 2) + " Hz"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "intensity", 1 }, "Intensity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "volume", 1 }, "Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void UniVibeProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    lfo.setSampleRate (sampleRate);
    lfo.setWaveform (openpedals::LFO::Waveform::Sine);
    lfo.reset();

    for (int s = 0; s < numStages; ++s)
    {
        allpassL[s].reset();
        allpassR[s].reset();
    }
}

void UniVibeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float speed     = speedParam->load();
    const float intensity = intensityParam->load();
    const float volume    = volumeParam->load();

    lfo.setFrequency (speed);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        // LFO value in 0..1 range
        float lfoVal = lfo.tick() * 0.5f + 0.5f;

        // Update allpass filters with staggered frequencies modulated by LFO
        for (int s = 0; s < numStages; ++s)
        {
            // Each stage has its own center freq and modulation depth
            float modAmount = lfoVal * intensity * stageDepths[s];
            float freq = stageFreqs[s] * (1.0f + modAmount * 2.0f);
            freq = std::min (freq, static_cast<float> (currentSampleRate) * 0.45f);
            allpassL[s].setParameters (openpedals::BiquadFilter::Type::AllPass, freq, 0.707, 0.0, currentSampleRate);
            allpassR[s].setParameters (openpedals::BiquadFilter::Type::AllPass, freq, 0.707, 0.0, currentSampleRate);
        }

        // Left channel
        if (numChannels > 0)
        {
            float dry = buffer.getSample (0, i);
            float wet = dry;
            for (int s = 0; s < numStages; ++s)
                wet = allpassL[s].process (wet);
            // 50/50 internal mix for uni-vibe character
            float out = (dry + wet) * 0.5f * volume * 2.0f;
            buffer.setSample (0, i, out);
        }

        // Right channel
        if (numChannels > 1)
        {
            float dry = buffer.getSample (1, i);
            float wet = dry;
            for (int s = 0; s < numStages; ++s)
                wet = allpassR[s].process (wet);
            float out = (dry + wet) * 0.5f * volume * 2.0f;
            buffer.setSample (1, i, out);
        }
    }
}

void UniVibeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void UniVibeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UniVibeProcessor();
}
