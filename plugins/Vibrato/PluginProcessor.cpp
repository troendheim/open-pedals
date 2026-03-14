/*
 * OpenPedals Vibrato — Pure pitch modulation via LFO-modulated delay
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

VibratoProcessor::VibratoProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    rateParam  = apvts.getRawParameterValue ("rate");
    depthParam = apvts.getRawParameterValue ("depth");

    setCurrentProgram (0);
}

void VibratoProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("rate")->setValueNotifyingHost (apvts.getParameter ("rate")->convertTo0to1 (p.rate));
    apvts.getParameter ("depth")->setValueNotifyingHost (apvts.getParameter ("depth")->convertTo0to1 (p.depth));
}

juce::AudioProcessorValueTreeState::ParameterLayout VibratoProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rate", 1 }, "Rate",
        juce::NormalisableRange<float> (0.5f, 10.0f, 0.01f, 0.5f), 5.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 2) + " Hz"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void VibratoProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    const int maxDelaySamples = static_cast<int> ((centerDelayMs + maxDepthMs + 2.0f) * sampleRate / 1000.0f);
    delayLineL.setMaxDelay (maxDelaySamples);
    delayLineR.setMaxDelay (maxDelaySamples);
    delayLineL.reset();
    delayLineR.reset();

    lfoL.setSampleRate (sampleRate);
    lfoR.setSampleRate (sampleRate);
    lfoL.setWaveform (openpedals::LFO::Waveform::Sine);
    lfoR.setWaveform (openpedals::LFO::Waveform::Sine);
    lfoL.reset();
    lfoR.reset();
    lfoR.setPhase (0.25);
}

void VibratoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float rate  = rateParam->load();
    const float depth = depthParam->load();

    lfoL.setFrequency (rate);
    lfoR.setFrequency (rate);

    const float depthMs = depth * maxDepthMs;
    const float centerDelaySamples = centerDelayMs * static_cast<float> (currentSampleRate) / 1000.0f;
    const float depthSamples = depthMs * static_cast<float> (currentSampleRate) / 1000.0f;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        float lfoValL = lfoL.tick();
        float lfoValR = lfoR.tick();

        float modulatedDelayL = centerDelaySamples + lfoValL * depthSamples;
        float modulatedDelayR = centerDelaySamples + lfoValR * depthSamples;

        modulatedDelayL = std::max (modulatedDelayL, 1.0f);
        modulatedDelayR = std::max (modulatedDelayR, 1.0f);

        // Left channel — 100% wet (vibrato = no dry signal)
        if (numChannels > 0)
        {
            float dry = buffer.getSample (0, i);
            delayLineL.push (dry);
            float wet = delayLineL.getInterpolated (modulatedDelayL);
            buffer.setSample (0, i, wet);
        }

        // Right channel — 100% wet
        if (numChannels > 1)
        {
            float dry = buffer.getSample (1, i);
            delayLineR.push (dry);
            float wet = delayLineR.getInterpolated (modulatedDelayR);
            buffer.setSample (1, i, wet);
        }
    }
}

void VibratoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VibratoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibratoProcessor();
}
