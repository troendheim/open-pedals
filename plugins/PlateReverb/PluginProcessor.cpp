/*
 * OpenPedals Plate Reverb — Dense, smooth plate reverb with bright character
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

PlateReverbProcessor::PlateReverbProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    decayParam    = apvts.getRawParameterValue ("decay");
    dampingParam  = apvts.getRawParameterValue ("damping");
    mixParam      = apvts.getRawParameterValue ("mix");
    preDelayParam = apvts.getRawParameterValue ("predelay");

    setCurrentProgram (0);
}

void PlateReverbProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("decay")->setValueNotifyingHost (apvts.getParameter ("decay")->convertTo0to1 (p.decay));
    apvts.getParameter ("damping")->setValueNotifyingHost (apvts.getParameter ("damping")->convertTo0to1 (p.damping));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
    apvts.getParameter ("predelay")->setValueNotifyingHost (apvts.getParameter ("predelay")->convertTo0to1 (p.predelay));
}

juce::AudioProcessorValueTreeState::ParameterLayout PlateReverbProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "decay", 1 }, "Decay",
        juce::NormalisableRange<float> (0.1f, 8.0f, 0.01f, 0.5f), 2.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " s"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "damping", 1 }, "Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.35f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "predelay", 1 }, "Pre-Delay",
        juce::NormalisableRange<float> (0.0f, 60.0f, 1.0f), 10.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " ms"; }));

    return { params.begin(), params.end() };
}

void PlateReverbProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 2;
    reverb.prepare (spec);

    // Pre-delay: max 60ms
    const int maxPreDelaySamples = static_cast<int> (sampleRate * 0.06) + 1;
    preDelayL.setMaxDelay (maxPreDelaySamples);
    preDelayR.setMaxDelay (maxPreDelaySamples);
    preDelayL.reset();
    preDelayR.reset();

    preDelayBuffer.setSize (2, samplesPerBlock);
}

void PlateReverbProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float decay    = decayParam->load();
    const float damping  = dampingParam->load();
    const float mix      = mixParam->load();
    const float preDelay = preDelayParam->load();

    // Plate character: tighter room size mapping (max 8s -> 0-1),
    // lower damping for brighter tone, wide stereo
    const float roomSize = std::min (decay / 8.0f, 1.0f);

    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize   = roomSize;
    reverbParams.damping    = damping * 0.7f;   // Reduce damping for brighter plate character
    reverbParams.wetLevel   = mix;
    reverbParams.dryLevel   = 1.0f - mix;
    reverbParams.width      = 1.0f;             // Full stereo width for plate diffusion
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters (reverbParams);

    // Apply pre-delay
    const float preDelaySamples = preDelay * static_cast<float> (currentSampleRate) / 1000.0f;
    preDelayL.setDelay (preDelaySamples);
    preDelayR.setDelay (preDelaySamples);

    const int numSamples = buffer.getNumSamples();

    if (preDelay > 0.0f)
    {
        preDelayBuffer.setSize (buffer.getNumChannels(), numSamples, false, false, true);

        for (int i = 0; i < numSamples; ++i)
        {
            if (buffer.getNumChannels() > 0)
            {
                preDelayL.push (buffer.getSample (0, i));
                preDelayBuffer.setSample (0, i, preDelayL.get());
            }
            if (buffer.getNumChannels() > 1)
            {
                preDelayR.push (buffer.getSample (1, i));
                preDelayBuffer.setSample (1, i, preDelayR.get());
            }
        }

        juce::dsp::AudioBlock<float> block (preDelayBuffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        reverb.process (context);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, 0, preDelayBuffer, ch, 0, numSamples);
    }
    else
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        reverb.process (context);
    }
}

void PlateReverbProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PlateReverbProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlateReverbProcessor();
}
