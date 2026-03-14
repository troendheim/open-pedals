/*
 * OpenPedals Ring Mod — Ring modulation with sine/square carrier
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

RingModProcessor::RingModProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    frequencyParam = apvts.getRawParameterValue ("frequency");
    mixParam       = apvts.getRawParameterValue ("mix");
    shapeParam     = apvts.getRawParameterValue ("shape");

    setCurrentProgram (0);
}

void RingModProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("frequency")->setValueNotifyingHost (apvts.getParameter ("frequency")->convertTo0to1 (p.frequency));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
    apvts.getParameter ("shape")->setValueNotifyingHost (apvts.getParameter ("shape")->convertTo0to1 (p.shape));
}

juce::AudioProcessorValueTreeState::ParameterLayout RingModProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "frequency", 1 }, "Frequency",
        juce::NormalisableRange<float> (20.0f, 2000.0f, 0.1f, 0.3f), 300.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " Hz"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "shape", 1 }, "Shape",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int)
        {
            if (value < 0.5f) return juce::String ("Sine");
            else              return juce::String ("Square");
        }));

    return { params.begin(), params.end() };
}

void RingModProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    carrierSine.setSampleRate (sampleRate);
    carrierSquare.setSampleRate (sampleRate);
    carrierSine.setWaveform (openpedals::LFO::Waveform::Sine);
    carrierSquare.setWaveform (openpedals::LFO::Waveform::Square);
    carrierSine.reset();
    carrierSquare.reset();
}

void RingModProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float freq  = frequencyParam->load();
    const float mix   = mixParam->load();
    const float shape = shapeParam->load();

    carrierSine.setFrequency (freq);
    carrierSquare.setFrequency (freq);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        float sinVal = carrierSine.tick();
        float sqrVal = carrierSquare.tick();

        // Crossfade between sine and square carrier
        float carrier = sinVal * (1.0f - shape) + sqrVal * shape;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float dry = buffer.getSample (ch, i);
            float wet = dry * carrier;
            buffer.setSample (ch, i, dry * (1.0f - mix) + wet * mix);
        }
    }
}

void RingModProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RingModProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RingModProcessor();
}
