/*
 * OpenPedals Compressor — Dynamic range compressor
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

CompressorProcessor::CompressorProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    thresholdParam  = apvts.getRawParameterValue ("threshold");
    ratioParam      = apvts.getRawParameterValue ("ratio");
    attackParam     = apvts.getRawParameterValue ("attack");
    releaseParam    = apvts.getRawParameterValue ("release");
    makeupGainParam = apvts.getRawParameterValue ("makeup");

    setCurrentProgram (0);
}

void CompressorProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("threshold")->setValueNotifyingHost (apvts.getParameter ("threshold")->convertTo0to1 (p.threshold));
    apvts.getParameter ("ratio")->setValueNotifyingHost (apvts.getParameter ("ratio")->convertTo0to1 (p.ratio));
    apvts.getParameter ("attack")->setValueNotifyingHost (apvts.getParameter ("attack")->convertTo0to1 (p.attack));
    apvts.getParameter ("release")->setValueNotifyingHost (apvts.getParameter ("release")->convertTo0to1 (p.release));
    apvts.getParameter ("makeup")->setValueNotifyingHost (apvts.getParameter ("makeup")->convertTo0to1 (p.makeup));
}

juce::AudioProcessorValueTreeState::ParameterLayout CompressorProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 }, "Threshold",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -20.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ratio", 1 }, "Ratio",
        juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f, 0.5f), 4.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + ":1"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack", 1 }, "Attack",
        juce::NormalisableRange<float> (0.1f, 100.0f, 0.1f, 0.5f), 10.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " ms"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 }, "Release",
        juce::NormalisableRange<float> (10.0f, 1000.0f, 1.0f, 0.5f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " ms"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "makeup", 1 }, "Makeup Gain",
        juce::NormalisableRange<float> (0.0f, 30.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; }));

    return { params.begin(), params.end() };
}

void CompressorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());
    compressor.prepare (spec);
}

void CompressorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float threshold = thresholdParam->load();
    const float ratio     = ratioParam->load();
    const float attack    = attackParam->load();
    const float release   = releaseParam->load();
    const float makeupDB  = makeupGainParam->load();

    // Update compressor parameters
    compressor.setThreshold (threshold);
    compressor.setRatio (ratio);
    compressor.setAttack (attack);
    compressor.setRelease (release);

    // Process through JUCE's compressor
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    compressor.process (context);

    // Apply makeup gain
    if (makeupDB != 0.0f)
    {
        const float makeupGain = juce::Decibels::decibelsToGain (makeupDB);
        buffer.applyGain (makeupGain);
    }
}

void CompressorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CompressorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorProcessor();
}
