/*
 * OpenPedals Auto-Wah — Envelope-controlled bandpass filter
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

AutoWahProcessor::AutoWahProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    sensitivityParam = apvts.getRawParameterValue ("sensitivity");
    rangeParam       = apvts.getRawParameterValue ("range");
    speedParam       = apvts.getRawParameterValue ("speed");
    mixParam         = apvts.getRawParameterValue ("mix");

    setCurrentProgram (0);
}

void AutoWahProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("sensitivity")->setValueNotifyingHost (apvts.getParameter ("sensitivity")->convertTo0to1 (p.sensitivity));
    apvts.getParameter ("range")->setValueNotifyingHost (apvts.getParameter ("range")->convertTo0to1 (p.range));
    apvts.getParameter ("speed")->setValueNotifyingHost (apvts.getParameter ("speed")->convertTo0to1 (p.speed));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
}

juce::AudioProcessorValueTreeState::ParameterLayout AutoWahProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sensitivity", 1 }, "Sensitivity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "range", 1 }, "Range",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "speed", 1 }, "Speed",
        juce::NormalisableRange<float> (1.0f, 100.0f, 0.1f, 0.5f), 10.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " ms"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void AutoWahProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    filterL.reset();
    filterR.reset();

    envFollowerL.setSampleRate (sampleRate);
    envFollowerR.setSampleRate (sampleRate);
    envFollowerL.reset();
    envFollowerR.reset();
}

void AutoWahProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float sensitivity = sensitivityParam->load();
    const float range       = rangeParam->load();
    const float speed       = speedParam->load();
    const float mix         = mixParam->load();

    // Speed parameter controls envelope attack time
    envFollowerL.setAttack (speed);
    envFollowerR.setAttack (speed);
    // Release is slower than attack for natural decay
    envFollowerL.setRelease (speed * 5.0f);
    envFollowerR.setRelease (speed * 5.0f);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        // Left channel
        if (numChannels > 0)
        {
            float dry = buffer.getSample (0, i);

            // Track envelope
            float env = envFollowerL.process (dry);

            // Scale envelope by sensitivity (0-1 -> 0-2x amplification of envelope)
            float scaledEnv = std::min (env * (1.0f + sensitivity * 9.0f), 1.0f);

            // Map envelope to frequency: baseFreq + scaledEnv * range * freqRange
            float freq = baseFreq + scaledEnv * range * freqRange;
            freq = std::min (freq, baseFreq + freqRange);

            filterL.setParameters (openpedals::BiquadFilter::Type::BandPass, freq, filterQ, 0.0, currentSampleRate);
            float wet = filterL.process (dry);

            buffer.setSample (0, i, dry * (1.0f - mix) + wet * mix);
        }

        // Right channel
        if (numChannels > 1)
        {
            float dry = buffer.getSample (1, i);

            float env = envFollowerR.process (dry);
            float scaledEnv = std::min (env * (1.0f + sensitivity * 9.0f), 1.0f);
            float freq = baseFreq + scaledEnv * range * freqRange;
            freq = std::min (freq, baseFreq + freqRange);

            filterR.setParameters (openpedals::BiquadFilter::Type::BandPass, freq, filterQ, 0.0, currentSampleRate);
            float wet = filterR.process (dry);

            buffer.setSample (1, i, dry * (1.0f - mix) + wet * mix);
        }
    }
}

void AutoWahProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AutoWahProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutoWahProcessor();
}
