/*
 * OpenPedals Rotary — Leslie speaker simulation with horn and drum
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

RotaryProcessor::RotaryProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    speedParam = apvts.getRawParameterValue ("speed");
    depthParam = apvts.getRawParameterValue ("depth");
    mixParam   = apvts.getRawParameterValue ("mix");

    setCurrentProgram (0);
}

void RotaryProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("speed")->setValueNotifyingHost (apvts.getParameter ("speed")->convertTo0to1 (p.speed));
    apvts.getParameter ("depth")->setValueNotifyingHost (apvts.getParameter ("depth")->convertTo0to1 (p.depth));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
}

juce::AudioProcessorValueTreeState::ParameterLayout RotaryProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "speed", 1 }, "Speed",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void RotaryProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    const int maxDelaySamples = static_cast<int> ((centerDelayMs + maxDepthMs + 2.0f) * sampleRate / 1000.0f);
    hornDelay.setMaxDelay (maxDelaySamples);
    drumDelay.setMaxDelay (maxDelaySamples);
    hornDelay.reset();
    drumDelay.reset();

    hornLfo.setSampleRate (sampleRate);
    drumLfo.setSampleRate (sampleRate);
    hornLfo.setWaveform (openpedals::LFO::Waveform::Sine);
    drumLfo.setWaveform (openpedals::LFO::Waveform::Sine);
    hornLfo.reset();
    drumLfo.reset();
}

void RotaryProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float speed = speedParam->load();
    const float depth = depthParam->load();
    const float mix   = mixParam->load();

    // Interpolate between slow and fast rates based on speed parameter
    float hornRate = hornSlowHz + speed * (hornFastHz - hornSlowHz);
    float drumRate = drumSlowHz + speed * (drumFastHz - drumSlowHz);

    hornLfo.setFrequency (hornRate);
    drumLfo.setFrequency (drumRate);

    const float depthMs = depth * maxDepthMs;
    const float centerDelaySamples = centerDelayMs * static_cast<float> (currentSampleRate) / 1000.0f;
    const float depthSamples = depthMs * static_cast<float> (currentSampleRate) / 1000.0f;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        float hornVal = hornLfo.tick();
        float drumVal = drumLfo.tick();

        // Amplitude modulation: convert to 0..1 range
        float hornAmp = 1.0f - depth * 0.5f * (hornVal * 0.5f + 0.5f);
        float drumAmp = 1.0f - depth * 0.5f * (drumVal * 0.5f + 0.5f);

        // Pitch modulation via delay
        float hornDelayMod = centerDelaySamples + hornVal * depthSamples;
        float drumDelayMod = centerDelaySamples + drumVal * depthSamples;
        hornDelayMod = std::max (hornDelayMod, 1.0f);
        drumDelayMod = std::max (drumDelayMod, 1.0f);

        // Stereo panning: horn pans left, drum pans right
        float hornPanL = 0.5f + hornVal * 0.5f * depth;
        float hornPanR = 1.0f - hornPanL;
        float drumPanL = 1.0f - (0.5f + drumVal * 0.5f * depth);
        float drumPanR = 1.0f - drumPanL;

        // Left channel — horn effect
        if (numChannels > 0)
        {
            float dry = buffer.getSample (0, i);
            hornDelay.push (dry);
            float hornWet = hornDelay.getInterpolated (hornDelayMod) * hornAmp * hornPanL;
            float drumWet = dry * drumAmp * drumPanL;
            float wet = (hornWet + drumWet) * 0.5f;
            buffer.setSample (0, i, dry * (1.0f - mix) + wet * mix);
        }

        // Right channel — drum effect
        if (numChannels > 1)
        {
            float dry = buffer.getSample (1, i);
            drumDelay.push (dry);
            float hornWet = dry * hornAmp * hornPanR;
            float drumWet = drumDelay.getInterpolated (drumDelayMod) * drumAmp * drumPanR;
            float wet = (hornWet + drumWet) * 0.5f;
            buffer.setSample (1, i, dry * (1.0f - mix) + wet * mix);
        }
    }
}

void RotaryProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RotaryProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RotaryProcessor();
}
