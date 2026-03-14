/*
 * OpenPedals Limiter — Brick-wall limiter for clipping prevention
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

LimiterProcessor::LimiterProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    thresholdParam = apvts.getRawParameterValue ("threshold");
    releaseParam   = apvts.getRawParameterValue ("release");
    ceilingParam   = apvts.getRawParameterValue ("ceiling");

    setCurrentProgram (0);
}

void LimiterProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("threshold")->setValueNotifyingHost (apvts.getParameter ("threshold")->convertTo0to1 (p.threshold));
    apvts.getParameter ("release")->setValueNotifyingHost (apvts.getParameter ("release")->convertTo0to1 (p.release));
    apvts.getParameter ("ceiling")->setValueNotifyingHost (apvts.getParameter ("ceiling")->convertTo0to1 (p.ceiling));
}

juce::AudioProcessorValueTreeState::ParameterLayout LimiterProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 }, "Threshold",
        juce::NormalisableRange<float> (-20.0f, 0.0f, 0.1f), -1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 }, "Release",
        juce::NormalisableRange<float> (10.0f, 1000.0f, 1.0f, 0.5f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " ms"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ceiling", 1 }, "Ceiling",
        juce::NormalisableRange<float> (-6.0f, 0.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; }));

    return { params.begin(), params.end() };
}

void LimiterProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());
    limiter.prepare (spec);
}

void LimiterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float threshold = thresholdParam->load();
    const float release   = releaseParam->load();
    const float ceilingDB = ceilingParam->load();

    // Update limiter parameters
    limiter.setThreshold (threshold);
    limiter.setRelease (release);

    // Process through JUCE's limiter
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    limiter.process (context);

    // Apply ceiling gain
    if (ceilingDB != 0.0f)
    {
        const float ceilingGain = juce::Decibels::decibelsToGain (ceilingDB);
        buffer.applyGain (ceilingGain);
    }
}

void LimiterProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LimiterProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LimiterProcessor();
}
