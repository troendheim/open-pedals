/*
 * OpenPedals Reverse Delay — Reversed playback delay for backwards guitar effects
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

ReverseDelayProcessor::ReverseDelayProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    timeParam     = apvts.getRawParameterValue ("time");
    feedbackParam = apvts.getRawParameterValue ("feedback");
    mixParam      = apvts.getRawParameterValue ("mix");

    setCurrentProgram (0);
}

void ReverseDelayProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("time")->setValueNotifyingHost (apvts.getParameter ("time")->convertTo0to1 (p.time));
    apvts.getParameter ("feedback")->setValueNotifyingHost (apvts.getParameter ("feedback")->convertTo0to1 (p.feedback));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
}

juce::AudioProcessorValueTreeState::ParameterLayout ReverseDelayProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "time", 1 }, "Time",
        juce::NormalisableRange<float> (100.0f, 2000.0f, 1.0f, 0.5f), 500.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " ms"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "feedback", 1 }, "Feedback",
        juce::NormalisableRange<float> (0.0f, 0.90f, 0.01f), 0.3f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void ReverseDelayProcessor::initChannel (ReverseChannel& ch, int maxSamples)
{
    ch.bufferA.resize (static_cast<size_t> (maxSamples), 0.0f);
    ch.bufferB.resize (static_cast<size_t> (maxSamples), 0.0f);
}

void ReverseDelayProcessor::resetChannel (ReverseChannel& ch)
{
    std::fill (ch.bufferA.begin(), ch.bufferA.end(), 0.0f);
    std::fill (ch.bufferB.begin(), ch.bufferB.end(), 0.0f);
    ch.writePos = 0;
    ch.readPos = 0;
    ch.bufferSize = 0;
    ch.writeToA = true;
}

float ReverseDelayProcessor::processChannel (ReverseChannel& ch, float input, int currentBufferSize, float feedback)
{
    // Get pointers to write and read buffers
    auto& writeBuf = ch.writeToA ? ch.bufferA : ch.bufferB;
    auto& readBuf  = ch.writeToA ? ch.bufferB : ch.bufferA;

    // Write input (plus feedback from reversed output) into write buffer
    writeBuf[static_cast<size_t> (ch.writePos)] = input;

    // Read from the read buffer backwards
    float reversed = 0.0f;
    if (ch.bufferSize > 0)
    {
        // Read position goes from (bufferSize - 1) down to 0
        int readIdx = ch.bufferSize - 1 - ch.readPos;
        if (readIdx >= 0 && readIdx < static_cast<int> (readBuf.size()))
            reversed = readBuf[static_cast<size_t> (readIdx)];

        // Apply crossfade at buffer boundaries to avoid clicks
        if (ch.readPos < crossfadeSamples)
        {
            float fade = static_cast<float> (ch.readPos) / static_cast<float> (crossfadeSamples);
            reversed *= fade;
        }
        else if (ch.readPos >= ch.bufferSize - crossfadeSamples)
        {
            float fade = static_cast<float> (ch.bufferSize - 1 - ch.readPos) / static_cast<float> (crossfadeSamples);
            fade = std::max (fade, 0.0f);
            reversed *= fade;
        }
    }

    // Feed reversed output back into the write buffer with feedback
    writeBuf[static_cast<size_t> (ch.writePos)] += reversed * feedback;

    ch.writePos++;
    ch.readPos++;

    // When write buffer is full, swap buffers
    if (ch.writePos >= currentBufferSize)
    {
        ch.writeToA = !ch.writeToA;
        ch.bufferSize = currentBufferSize;
        ch.writePos = 0;
        ch.readPos = 0;
    }

    // If read buffer hasn't been filled yet, readPos may exceed bufferSize
    if (ch.readPos >= ch.bufferSize && ch.bufferSize > 0)
        ch.readPos = ch.bufferSize - 1;

    return reversed;
}

void ReverseDelayProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    const int maxSamples = static_cast<int> (sampleRate * maxDelayMs / 1000.0) + 1;
    initChannel (channelL, maxSamples);
    initChannel (channelR, maxSamples);
    resetChannel (channelL);
    resetChannel (channelR);
}

void ReverseDelayProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float delayMs  = timeParam->load();
    const float feedback = feedbackParam->load();
    const float mix      = mixParam->load();

    const int bufferSizeInSamples = std::max (1, static_cast<int> (delayMs * currentSampleRate / 1000.0));

    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        // Left channel
        if (buffer.getNumChannels() > 0)
        {
            float dry = buffer.getSample (0, i);
            float wet = processChannel (channelL, dry, bufferSizeInSamples, feedback);
            buffer.setSample (0, i, dry * (1.0f - mix) + wet * mix);
        }

        // Right channel
        if (buffer.getNumChannels() > 1)
        {
            float dry = buffer.getSample (1, i);
            float wet = processChannel (channelR, dry, bufferSizeInSamples, feedback);
            buffer.setSample (1, i, dry * (1.0f - mix) + wet * mix);
        }
    }
}

void ReverseDelayProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ReverseDelayProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverseDelayProcessor();
}
