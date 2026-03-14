/*
 * OpenPedals Harmonizer — Adds a pitch-shifted harmony voice to the dry signal
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
#include <cmath>

HarmonizerProcessor::HarmonizerProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    intervalParam = apvts.getRawParameterValue ("interval");
    levelParam    = apvts.getRawParameterValue ("level");
    dryParam      = apvts.getRawParameterValue ("dry");

    setCurrentProgram (0);
}

void HarmonizerProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("interval")->setValueNotifyingHost (apvts.getParameter ("interval")->convertTo0to1 (p.interval));
    apvts.getParameter ("level")->setValueNotifyingHost (apvts.getParameter ("level")->convertTo0to1 (p.level));
    apvts.getParameter ("dry")->setValueNotifyingHost (apvts.getParameter ("dry")->convertTo0to1 (p.dry));
}

juce::AudioProcessorValueTreeState::ParameterLayout HarmonizerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "interval", 1 }, "Interval",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 1.0f), 7.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " st"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "level", 1 }, "Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "dry", 1 }, "Dry",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void HarmonizerProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    // Grain size ~50ms, adjusted to sample rate
    grainSize = static_cast<int> (sampleRate * 0.05);
    if (grainSize < 256) grainSize = 256;

    const int bufferSize = grainSize * 4;

    for (auto& ch : channelState)
    {
        ch.delayLine.setMaxDelay (bufferSize);
        ch.delayLine.reset();
        ch.writePos  = 0;
        ch.grainPos0 = 0.0f;
        ch.grainPos1 = static_cast<float> (grainSize / 2);
    }
}

float HarmonizerProcessor::processGrainPitchShift (ChannelState& state, float input, float pitchRatio)
{
    state.delayLine.push (input);
    state.writePos++;

    float readIncrement = 1.0f - pitchRatio;

    state.grainPos0 += readIncrement;
    state.grainPos1 += readIncrement;

    float gs = static_cast<float> (grainSize);

    if (state.grainPos0 >= gs * 2.0f)
        state.grainPos0 -= gs * 2.0f;
    if (state.grainPos0 < 0.0f)
        state.grainPos0 += gs * 2.0f;
    if (state.grainPos1 >= gs * 2.0f)
        state.grainPos1 -= gs * 2.0f;
    if (state.grainPos1 < 0.0f)
        state.grainPos1 += gs * 2.0f;

    float delay0 = std::max (state.grainPos0, 1.0f);
    float delay1 = std::max (state.grainPos1, 1.0f);

    float grain0 = state.delayLine.getInterpolated (delay0);
    float grain1 = state.delayLine.getInterpolated (delay1);

    float phase0 = state.grainPos0 / gs;
    if (phase0 > 1.0f) phase0 -= 1.0f;
    float phase1 = state.grainPos1 / gs;
    if (phase1 > 1.0f) phase1 -= 1.0f;

    float window0 = 0.5f - 0.5f * std::cos (2.0f * static_cast<float> (M_PI) * phase0);
    float window1 = 0.5f - 0.5f * std::cos (2.0f * static_cast<float> (M_PI) * phase1);

    return grain0 * window0 + grain1 * window1;
}

void HarmonizerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float interval = intervalParam->load();
    const float level    = levelParam->load();
    const float dry      = dryParam->load();

    const float pitchRatio = std::pow (2.0f, interval / 12.0f);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < std::min (numChannels, 2); ++ch)
    {
        auto& state = channelState[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            float input   = buffer.getSample (ch, i);
            float shifted = processGrainPitchShift (state, input, pitchRatio);
            buffer.setSample (ch, i, dry * input + level * shifted);
        }
    }
}

void HarmonizerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void HarmonizerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HarmonizerProcessor();
}
