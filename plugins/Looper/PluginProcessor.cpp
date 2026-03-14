/*
 * OpenPedals Looper — Freeze/loop effect with circular buffer
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

LooperProcessor::LooperProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    lengthParam = apvts.getRawParameterValue ("length");
    mixParam    = apvts.getRawParameterValue ("mix");
    freezeParam = apvts.getRawParameterValue ("freeze");

    setCurrentProgram (0);
}

void LooperProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("length")->setValueNotifyingHost (apvts.getParameter ("length")->convertTo0to1 (p.length));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
    apvts.getParameter ("freeze")->setValueNotifyingHost (apvts.getParameter ("freeze")->convertTo0to1 (p.freeze));
}

juce::AudioProcessorValueTreeState::ParameterLayout LooperProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "length", 1 }, "Length",
        juce::NormalisableRange<float> (0.1f, 5.0f, 0.01f, 0.5f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 2) + " s"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "freeze", 1 }, "Freeze",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return value >= 0.5f ? "Frozen" : "Normal"; }));

    return { params.begin(), params.end() };
}

void LooperProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    // Max buffer: 5 seconds at current sample rate
    maxBufferSize = static_cast<int> (5.0 * sampleRate);

    for (int ch = 0; ch < maxChannels; ++ch)
    {
        loopBuffer[ch].resize (static_cast<size_t> (maxBufferSize), 0.0f);
        std::fill (loopBuffer[ch].begin(), loopBuffer[ch].end(), 0.0f);
    }

    writePos = 0;
    readPos = 0;
    frozen = false;
    frozenLength = 0;
}

void LooperProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float length = lengthParam->load();
    const float mix    = mixParam->load();
    const float freeze = freezeParam->load();

    const int loopLengthSamples = std::max (1, static_cast<int> (length * static_cast<float> (currentSampleRate)));
    const int effectiveLength = std::min (loopLengthSamples, maxBufferSize);

    const bool shouldFreeze = (freeze >= 0.5f);

    // Handle freeze state transitions
    if (shouldFreeze && !frozen)
    {
        // Entering freeze: capture the current loop length
        frozen = true;
        frozenLength = effectiveLength;
        readPos = 0;
    }
    else if (!shouldFreeze && frozen)
    {
        // Exiting freeze: resume recording
        frozen = false;
        writePos = 0;
    }

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = std::min (buffer.getNumChannels(), maxChannels);

    for (int i = 0; i < numSamples; ++i)
    {
        if (!frozen)
        {
            // Normal mode: record into circular buffer, pass through
            for (int ch = 0; ch < numChannels; ++ch)
            {
                loopBuffer[ch][static_cast<size_t> (writePos)] = buffer.getSample (ch, i);
            }
            writePos = (writePos + 1) % maxBufferSize;
        }
        else
        {
            // Frozen mode: play back the loop and mix with dry
            int currentLoopLen = std::min (frozenLength, maxBufferSize);
            if (currentLoopLen < 1) currentLoopLen = 1;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float dry = buffer.getSample (ch, i);
                float wet = loopBuffer[ch][static_cast<size_t> (readPos)];

                // Apply crossfade at loop boundary
                if (readPos < crossfadeSamples && currentLoopLen > crossfadeSamples * 2)
                {
                    float fadeFactor = static_cast<float> (readPos) / static_cast<float> (crossfadeSamples);
                    int tailPos = (currentLoopLen - crossfadeSamples + readPos) % currentLoopLen;
                    float tailSample = loopBuffer[ch][static_cast<size_t> (tailPos)];
                    wet = wet * fadeFactor + tailSample * (1.0f - fadeFactor);
                }

                buffer.setSample (ch, i, dry * (1.0f - mix) + wet * mix);
            }

            readPos = (readPos + 1) % currentLoopLen;
        }
    }
}

void LooperProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LooperProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LooperProcessor();
}
