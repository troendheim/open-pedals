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

#include "PluginProcessor.h"

GraphicEQProcessor::GraphicEQProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    for (int i = 0; i < numBands; ++i)
        bandParams[i] = apvts.getRawParameterValue (bandIDs[i]);

    setCurrentProgram (0);
}

void GraphicEQProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    const float gains[numBands] = { p.b100, p.b200, p.b400, p.b800, p.b1k6, p.b3k2, p.b6k4 };
    for (int i = 0; i < numBands; ++i)
        apvts.getParameter (bandIDs[i])->setValueNotifyingHost (apvts.getParameter (bandIDs[i])->convertTo0to1 (gains[i]));
}

juce::AudioProcessorValueTreeState::ParameterLayout GraphicEQProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addBand = [&] (const char* id, const char* name)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
            juce::String(), juce::AudioProcessorParameter::genericParameter,
            [] (float value, int) { return juce::String (value, 1) + " dB"; }));
    };

    addBand ("band100", "100 Hz");
    addBand ("band200", "200 Hz");
    addBand ("band400", "400 Hz");
    addBand ("band800", "800 Hz");
    addBand ("band1k6", "1.6 kHz");
    addBand ("band3k2", "3.2 kHz");
    addBand ("band6k4", "6.4 kHz");

    return { params.begin(), params.end() };
}

void GraphicEQProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    for (int i = 0; i < numBands; ++i)
    {
        filtersL[i].reset();
        filtersR[i].reset();
    }
}

void GraphicEQProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Update filter coefficients
    for (int i = 0; i < numBands; ++i)
    {
        const float gainDB = bandParams[i]->load();
        filtersL[i].setParameters (openpedals::BiquadFilter::Type::Peaking,
                                   bandFrequencies[i], bandQ, gainDB, currentSampleRate);
        filtersR[i].setParameters (openpedals::BiquadFilter::Type::Peaking,
                                   bandFrequencies[i], bandQ, gainDB, currentSampleRate);
    }

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        // Left channel
        if (numChannels > 0)
        {
            float sample = buffer.getSample (0, i);
            for (int b = 0; b < numBands; ++b)
                sample = filtersL[b].process (sample);
            buffer.setSample (0, i, sample);
        }

        // Right channel
        if (numChannels > 1)
        {
            float sample = buffer.getSample (1, i);
            for (int b = 0; b < numBands; ++b)
                sample = filtersR[b].process (sample);
            buffer.setSample (1, i, sample);
        }
    }
}

void GraphicEQProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void GraphicEQProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GraphicEQProcessor();
}
