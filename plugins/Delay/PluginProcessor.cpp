/*
 * Bosslike Delay — Digital delay with feedback and tone control
 *
 * Part of the Bosslike guitar effects plugin collection.
 * Copyright (C) 2026 Richard Troendheim
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "PluginProcessor.h"

DelayProcessor::DelayProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    timeParam     = apvts.getRawParameterValue ("time");
    feedbackParam = apvts.getRawParameterValue ("feedback");
    mixParam      = apvts.getRawParameterValue ("mix");
    toneParam     = apvts.getRawParameterValue ("tone");
}

juce::AudioProcessorValueTreeState::ParameterLayout DelayProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "time", 1 }, "Time",
        juce::NormalisableRange<float> (1.0f, 2000.0f, 1.0f, 0.5f), 400.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " ms"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "feedback", 1 }, "Feedback",
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.4f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 }, "Tone",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

    return { params.begin(), params.end() };
}

void DelayProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    const int maxDelaySamples = static_cast<int> (sampleRate * maxDelayMs / 1000.0) + 1;
    delayLineL.setMaxDelay (maxDelaySamples);
    delayLineR.setMaxDelay (maxDelaySamples);
    delayLineL.reset();
    delayLineR.reset();

    feedbackFilterL.reset();
    feedbackFilterR.reset();

    feedbackSampleL = 0.0f;
    feedbackSampleR = 0.0f;
}

void DelayProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float delayMs  = timeParam->load();
    const float feedback = feedbackParam->load();
    const float mix      = mixParam->load();
    const float tone     = toneParam->load();

    // Map tone 0-1 to lowpass cutoff 1kHz-20kHz on feedback path
    const float toneCutoff = 1000.0f * std::pow (20.0f, tone);

    feedbackFilterL.setParameters (bosslike::BiquadFilter::Type::LowPass, toneCutoff, 0.707, 0.0, currentSampleRate);
    feedbackFilterR.setParameters (bosslike::BiquadFilter::Type::LowPass, toneCutoff, 0.707, 0.0, currentSampleRate);

    const float delaySamples = static_cast<float> (delayMs * currentSampleRate / 1000.0);
    delayLineL.setDelay (delaySamples);
    delayLineR.setDelay (delaySamples);

    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        // Left channel
        if (buffer.getNumChannels() > 0)
        {
            float dry = buffer.getSample (0, i);
            float delayedL = delayLineL.get();
            float toDelay = dry + feedbackFilterL.process (feedbackSampleL) * feedback;
            delayLineL.push (toDelay);
            feedbackSampleL = delayedL;
            buffer.setSample (0, i, dry * (1.0f - mix) + delayedL * mix);
        }

        // Right channel
        if (buffer.getNumChannels() > 1)
        {
            float dry = buffer.getSample (1, i);
            float delayedR = delayLineR.get();
            float toDelay = dry + feedbackFilterR.process (feedbackSampleR) * feedback;
            delayLineR.push (toDelay);
            feedbackSampleR = delayedR;
            buffer.setSample (1, i, dry * (1.0f - mix) + delayedR * mix);
        }
    }
}

void DelayProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DelayProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayProcessor();
}
