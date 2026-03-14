/*
 * OpenPedals Octaver — Sub-octave and octave-up voice generator
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

OctaverProcessor::OctaverProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    octDownParam = apvts.getRawParameterValue ("octdown");
    octUpParam   = apvts.getRawParameterValue ("octup");
    dryParam     = apvts.getRawParameterValue ("dry");

    setCurrentProgram (0);
}

void OctaverProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("octdown")->setValueNotifyingHost (apvts.getParameter ("octdown")->convertTo0to1 (p.octDown));
    apvts.getParameter ("octup")->setValueNotifyingHost (apvts.getParameter ("octup")->convertTo0to1 (p.octUp));
    apvts.getParameter ("dry")->setValueNotifyingHost (apvts.getParameter ("dry")->convertTo0to1 (p.dry));
}

juce::AudioProcessorValueTreeState::ParameterLayout OctaverProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "octdown", 1 }, "Oct Down",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "octup", 1 }, "Oct Up",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "dry", 1 }, "Dry",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void OctaverProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    for (auto& ch : channelState)
    {
        ch.prevSample = 0.0f;
        ch.flipFlop   = false;
        ch.subSquare  = 0.0f;

        // Low-pass filter for sub-octave: cut high harmonics from the square wave
        ch.subLowPass.setParameters (openpedals::BiquadFilter::Type::LowPass,
                                     200.0, 0.707, 0.0, sampleRate);
        ch.subLowPass.reset();

        // Band-pass filter for octave-up: center around guitar range
        ch.octUpBandPass.setParameters (openpedals::BiquadFilter::Type::BandPass,
                                        800.0, 1.0, 0.0, sampleRate);
        ch.octUpBandPass.reset();
    }
}

void OctaverProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float octDown = octDownParam->load();
    const float octUp   = octUpParam->load();
    const float dry     = dryParam->load();

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < std::min (numChannels, 2); ++ch)
    {
        auto& state = channelState[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            float input = buffer.getSample (ch, i);

            // --- Octave Down: zero-crossing flip-flop to divide frequency by 2 ---
            // Detect positive-going zero crossing
            if (state.prevSample <= 0.0f && input > 0.0f)
                state.flipFlop = !state.flipFlop;

            state.subSquare = state.flipFlop ? 1.0f : -1.0f;
            state.prevSample = input;

            // Scale square wave by input envelope for more natural tracking
            float envelope = std::abs (input);
            float subRaw = state.subSquare * envelope;

            // Heavy low-pass to smooth the sub square into something more sinusoidal
            float subFiltered = state.subLowPass.process (subRaw);

            // --- Octave Up: full-wave rectification ---
            float octUpRaw = std::abs (input);

            // Band-pass to clean up the rectified signal
            float octUpFiltered = state.octUpBandPass.process (octUpRaw);

            // --- Mix ---
            float output = dry * input + octDown * subFiltered + octUp * octUpFiltered;

            buffer.setSample (ch, i, output);
        }
    }
}

void OctaverProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void OctaverProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OctaverProcessor();
}
