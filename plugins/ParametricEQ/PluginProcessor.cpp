/*
 * OpenPedals Parametric EQ — 3-band fully parametric equalizer
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

ParametricEQProcessor::ParametricEQProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    freqParams[0] = apvts.getRawParameterValue ("freq1");
    gainParams[0] = apvts.getRawParameterValue ("gain1");
    qParams[0]    = apvts.getRawParameterValue ("q1");
    freqParams[1] = apvts.getRawParameterValue ("freq2");
    gainParams[1] = apvts.getRawParameterValue ("gain2");
    qParams[1]    = apvts.getRawParameterValue ("q2");
    freqParams[2] = apvts.getRawParameterValue ("freq3");
    gainParams[2] = apvts.getRawParameterValue ("gain3");
    qParams[2]    = apvts.getRawParameterValue ("q3");

    setCurrentProgram (0);
}

void ParametricEQProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];

    apvts.getParameter ("freq1")->setValueNotifyingHost (apvts.getParameter ("freq1")->convertTo0to1 (p.freq1));
    apvts.getParameter ("gain1")->setValueNotifyingHost (apvts.getParameter ("gain1")->convertTo0to1 (p.gain1));
    apvts.getParameter ("q1")->setValueNotifyingHost (apvts.getParameter ("q1")->convertTo0to1 (p.q1));
    apvts.getParameter ("freq2")->setValueNotifyingHost (apvts.getParameter ("freq2")->convertTo0to1 (p.freq2));
    apvts.getParameter ("gain2")->setValueNotifyingHost (apvts.getParameter ("gain2")->convertTo0to1 (p.gain2));
    apvts.getParameter ("q2")->setValueNotifyingHost (apvts.getParameter ("q2")->convertTo0to1 (p.q2));
    apvts.getParameter ("freq3")->setValueNotifyingHost (apvts.getParameter ("freq3")->convertTo0to1 (p.freq3));
    apvts.getParameter ("gain3")->setValueNotifyingHost (apvts.getParameter ("gain3")->convertTo0to1 (p.gain3));
    apvts.getParameter ("q3")->setValueNotifyingHost (apvts.getParameter ("q3")->convertTo0to1 (p.q3));
}

juce::AudioProcessorValueTreeState::ParameterLayout ParametricEQProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Band 1 (Low)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "freq1", 1 }, "Band 1 Freq",
        juce::NormalisableRange<float> (20.0f, 500.0f, 1.0f, 0.5f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " Hz"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain1", 1 }, "Band 1 Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "q1", 1 }, "Band 1 Q",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.5f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 2); }));

    // Band 2 (Mid)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "freq2", 1 }, "Band 2 Freq",
        juce::NormalisableRange<float> (200.0f, 5000.0f, 1.0f, 0.5f), 1000.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int)
        {
            if (value >= 1000.0f)
                return juce::String (value / 1000.0f, 1) + " kHz";
            return juce::String (static_cast<int> (value)) + " Hz";
        }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain2", 1 }, "Band 2 Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "q2", 1 }, "Band 2 Q",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.5f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 2); }));

    // Band 3 (High)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "freq3", 1 }, "Band 3 Freq",
        juce::NormalisableRange<float> (1000.0f, 20000.0f, 1.0f, 0.5f), 5000.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int)
        {
            if (value >= 1000.0f)
                return juce::String (value / 1000.0f, 1) + " kHz";
            return juce::String (static_cast<int> (value)) + " Hz";
        }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain3", 1 }, "Band 3 Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "q3", 1 }, "Band 3 Q",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.5f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 2); }));

    return { params.begin(), params.end() };
}

void ParametricEQProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    for (int i = 0; i < numBands; ++i)
    {
        filtersL[i].reset();
        filtersR[i].reset();
    }
}

void ParametricEQProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Update filter coefficients for each band
    for (int b = 0; b < numBands; ++b)
    {
        const float freq   = freqParams[b]->load();
        const float gainDB = gainParams[b]->load();
        const float q      = qParams[b]->load();

        filtersL[b].setParameters (openpedals::BiquadFilter::Type::Peaking,
                                   freq, q, gainDB, currentSampleRate);
        filtersR[b].setParameters (openpedals::BiquadFilter::Type::Peaking,
                                   freq, q, gainDB, currentSampleRate);
    }

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        if (numChannels > 0)
        {
            float sample = buffer.getSample (0, i);
            for (int b = 0; b < numBands; ++b)
                sample = filtersL[b].process (sample);
            buffer.setSample (0, i, sample);
        }

        if (numChannels > 1)
        {
            float sample = buffer.getSample (1, i);
            for (int b = 0; b < numBands; ++b)
                sample = filtersR[b].process (sample);
            buffer.setSample (1, i, sample);
        }
    }
}

void ParametricEQProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ParametricEQProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ParametricEQProcessor();
}
