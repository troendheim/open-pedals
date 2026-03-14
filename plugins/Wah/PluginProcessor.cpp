/*
 * OpenPedals Wah — Manual wah pedal with automatable position
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

WahProcessor::WahProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    positionParam = apvts.getRawParameterValue ("position");
    qParam        = apvts.getRawParameterValue ("q");
    volumeParam   = apvts.getRawParameterValue ("volume");

    setCurrentProgram (0);
}

void WahProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("position")->setValueNotifyingHost (apvts.getParameter ("position")->convertTo0to1 (p.position));
    apvts.getParameter ("q")->setValueNotifyingHost (apvts.getParameter ("q")->convertTo0to1 (p.q));
    apvts.getParameter ("volume")->setValueNotifyingHost (apvts.getParameter ("volume")->convertTo0to1 (p.volume));
}

juce::AudioProcessorValueTreeState::ParameterLayout WahProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "position", 1 }, "Position",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "q", 1 }, "Q",
        juce::NormalisableRange<float> (1.0f, 15.0f, 0.1f, 0.5f), 5.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1); }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "volume", 1 }, "Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void WahProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    filterL.reset();
    filterR.reset();
}

void WahProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float position = positionParam->load();
    const float q        = qParam->load();
    const float volume   = volumeParam->load();

    // Exponential mapping from position (0-1) to frequency (350-5000 Hz)
    const float freq = minFreq * std::pow (maxFreq / minFreq, position);

    // Output gain: volume 0-1 maps to 0-2x
    const float outputGain = volume * 2.0f;

    // Update bandpass filters
    filterL.setParameters (openpedals::BiquadFilter::Type::BandPass, freq, q, 0.0, currentSampleRate);
    filterR.setParameters (openpedals::BiquadFilter::Type::BandPass, freq, q, 0.0, currentSampleRate);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        if (numChannels > 0)
        {
            float sample = buffer.getSample (0, i);
            sample = filterL.process (sample) * outputGain;
            buffer.setSample (0, i, sample);
        }

        if (numChannels > 1)
        {
            float sample = buffer.getSample (1, i);
            sample = filterR.process (sample) * outputGain;
            buffer.setSample (1, i, sample);
        }
    }
}

void WahProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void WahProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WahProcessor();
}
