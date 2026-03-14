/*
 * OpenPedals Detune — Stereo pitch-detuning for thickening and widening
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

DetuneProcessor::DetuneProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    detuneParam = apvts.getRawParameterValue ("detune");
    mixParam    = apvts.getRawParameterValue ("mix");

    setCurrentProgram (0);
}

void DetuneProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("detune")->setValueNotifyingHost (apvts.getParameter ("detune")->convertTo0to1 (p.detune));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
}

juce::AudioProcessorValueTreeState::ParameterLayout DetuneProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "detune", 1 }, "Detune",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 10.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " ct"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void DetuneProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    // Grain size ~50ms, adjusted to sample rate
    grainSize = static_cast<int> (sampleRate * 0.05);
    if (grainSize < 256) grainSize = 256;

    const int bufferSize = grainSize * 4;

    // Left channel (shifts down)
    channelStateL.delayLine.setMaxDelay (bufferSize);
    channelStateL.delayLine.reset();
    channelStateL.writePos  = 0;
    channelStateL.grainPos0 = 0.0f;
    channelStateL.grainPos1 = static_cast<float> (grainSize / 2);

    // Right channel (shifts up)
    channelStateR.delayLine.setMaxDelay (bufferSize);
    channelStateR.delayLine.reset();
    channelStateR.writePos  = 0;
    channelStateR.grainPos0 = 0.0f;
    channelStateR.grainPos1 = static_cast<float> (grainSize / 2);
}

float DetuneProcessor::processGrainPitchShift (ChannelState& state, float input, float pitchRatio)
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

void DetuneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float detuneCents = detuneParam->load();
    const float mix         = mixParam->load();

    // Left channel shifts down, right channel shifts up
    const float pitchRatioDown = std::pow (2.0f, -detuneCents / 1200.0f);
    const float pitchRatioUp   = std::pow (2.0f,  detuneCents / 1200.0f);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Left channel — shift down
    if (numChannels > 0)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float dry = buffer.getSample (0, i);
            float wet = processGrainPitchShift (channelStateL, dry, pitchRatioDown);
            buffer.setSample (0, i, dry * (1.0f - mix) + wet * mix);
        }
    }

    // Right channel — shift up
    if (numChannels > 1)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float dry = buffer.getSample (1, i);
            float wet = processGrainPitchShift (channelStateR, dry, pitchRatioUp);
            buffer.setSample (1, i, dry * (1.0f - mix) + wet * mix);
        }
    }
}

void DetuneProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DetuneProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DetuneProcessor();
}
