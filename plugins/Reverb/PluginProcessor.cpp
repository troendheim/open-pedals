/*
 * OpenPedals Reverb — Hall/Room reverb with pre-delay
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

ReverbProcessor::ReverbProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    decayParam    = apvts.getRawParameterValue ("decay");
    dampingParam  = apvts.getRawParameterValue ("damping");
    mixParam      = apvts.getRawParameterValue ("mix");
    preDelayParam = apvts.getRawParameterValue ("predelay");
}

juce::AudioProcessorValueTreeState::ParameterLayout ReverbProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "decay", 1 }, "Decay",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.5f), 2.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " s"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "damping", 1 }, "Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "predelay", 1 }, "Pre-Delay",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 20.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " ms"; }));

    return { params.begin(), params.end() };
}

void ReverbProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 2;
    reverb.prepare (spec);

    // Pre-delay: max 100ms
    const int maxPreDelaySamples = static_cast<int> (sampleRate * 0.1) + 1;
    preDelayL.setMaxDelay (maxPreDelaySamples);
    preDelayR.setMaxDelay (maxPreDelaySamples);
    preDelayL.reset();
    preDelayR.reset();
}

void ReverbProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float decay    = decayParam->load();
    const float damping  = dampingParam->load();
    const float mix      = mixParam->load();
    const float preDelay = preDelayParam->load();

    // Map decay (0.1-10s) to room size (0-1) for JUCE's reverb
    // Freeverb's room size is 0-1 where 1 is longest
    const float roomSize = std::min (decay / 10.0f, 1.0f);

    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize   = roomSize;
    reverbParams.damping    = damping;
    reverbParams.wetLevel   = mix;
    reverbParams.dryLevel   = 1.0f - mix;
    reverbParams.width      = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters (reverbParams);

    // Apply pre-delay
    const float preDelaySamples = preDelay * static_cast<float> (currentSampleRate) / 1000.0f;
    preDelayL.setDelay (preDelaySamples);
    preDelayR.setDelay (preDelaySamples);

    const int numSamples = buffer.getNumSamples();

    if (preDelay > 0.0f)
    {
        // Create a copy for the wet signal with pre-delay applied
        juce::AudioBuffer<float> preDelayed (buffer.getNumChannels(), numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            if (buffer.getNumChannels() > 0)
            {
                preDelayL.push (buffer.getSample (0, i));
                preDelayed.setSample (0, i, preDelayL.get());
            }
            if (buffer.getNumChannels() > 1)
            {
                preDelayR.push (buffer.getSample (1, i));
                preDelayed.setSample (1, i, preDelayR.get());
            }
        }

        // Process the pre-delayed signal through reverb
        juce::dsp::AudioBlock<float> block (preDelayed);
        juce::dsp::ProcessContextReplacing<float> context (block);
        reverb.process (context);

        // Copy back to output
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, 0, preDelayed, ch, 0, numSamples);
    }
    else
    {
        // No pre-delay: process directly
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        reverb.process (context);
    }
}

void ReverbProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ReverbProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverbProcessor();
}
