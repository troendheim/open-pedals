/*
 * OpenPedals Phaser — 4-stage allpass phaser with LFO modulation
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

PhaserProcessor::PhaserProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    rateParam     = apvts.getRawParameterValue ("rate");
    depthParam    = apvts.getRawParameterValue ("depth");
    feedbackParam = apvts.getRawParameterValue ("feedback");
    mixParam      = apvts.getRawParameterValue ("mix");

    setCurrentProgram (0);
}

void PhaserProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("rate")->setValueNotifyingHost (apvts.getParameter ("rate")->convertTo0to1 (p.rate));
    apvts.getParameter ("depth")->setValueNotifyingHost (apvts.getParameter ("depth")->convertTo0to1 (p.depth));
    apvts.getParameter ("feedback")->setValueNotifyingHost (apvts.getParameter ("feedback")->convertTo0to1 (p.feedback));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
}

juce::AudioProcessorValueTreeState::ParameterLayout PhaserProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rate", 1 }, "Rate",
        juce::NormalisableRange<float> (0.05f, 5.0f, 0.01f, 0.5f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 2) + " Hz"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "feedback", 1 }, "Feedback",
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.3f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void PhaserProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    lfo.setSampleRate (sampleRate);
    lfo.setWaveform (openpedals::LFO::Waveform::Sine);
    lfo.reset();

    for (int s = 0; s < numStages; ++s)
    {
        allpassL[s].reset();
        allpassR[s].reset();
    }

    feedbackL = 0.0f;
    feedbackR = 0.0f;
}

void PhaserProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float rate     = rateParam->load();
    const float depth    = depthParam->load();
    const float feedback = feedbackParam->load();
    const float mix      = mixParam->load();

    lfo.setFrequency (rate);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        // LFO value mapped to 0..1 range
        float lfoVal = lfo.tick() * 0.5f + 0.5f;
        lfoVal *= depth;

        // Modulated center frequency for allpass filters
        float modFreq = minFreq * std::pow (maxFreq / minFreq, lfoVal);

        // Update allpass filter coefficients
        for (int s = 0; s < numStages; ++s)
        {
            // Spread stages across frequency range
            float stageFreq = modFreq * static_cast<float> (s + 1) / static_cast<float> (numStages);
            stageFreq = std::min (stageFreq, static_cast<float> (currentSampleRate) * 0.45f);
            allpassL[s].setParameters (openpedals::BiquadFilter::Type::AllPass, stageFreq, 0.707, 0.0, currentSampleRate);
            allpassR[s].setParameters (openpedals::BiquadFilter::Type::AllPass, stageFreq, 0.707, 0.0, currentSampleRate);
        }

        // Left channel
        if (numChannels > 0)
        {
            float dry = buffer.getSample (0, i);
            float input = dry + feedbackL * feedback;
            float wet = input;
            for (int s = 0; s < numStages; ++s)
                wet = allpassL[s].process (wet);
            feedbackL = wet;
            buffer.setSample (0, i, dry * (1.0f - mix) + wet * mix);
        }

        // Right channel
        if (numChannels > 1)
        {
            float dry = buffer.getSample (1, i);
            float input = dry + feedbackR * feedback;
            float wet = input;
            for (int s = 0; s < numStages; ++s)
                wet = allpassR[s].process (wet);
            feedbackR = wet;
            buffer.setSample (1, i, dry * (1.0f - mix) + wet * mix);
        }
    }
}

void PhaserProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PhaserProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhaserProcessor();
}
