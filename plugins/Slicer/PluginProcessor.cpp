/*
 * OpenPedals Slicer — Rhythmic gate effect with LFO-driven chopping
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

SlicerProcessor::SlicerProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    rateParam  = apvts.getRawParameterValue ("rate");
    depthParam = apvts.getRawParameterValue ("depth");
    shapeParam = apvts.getRawParameterValue ("shape");

    setCurrentProgram (0);
}

void SlicerProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("rate")->setValueNotifyingHost (apvts.getParameter ("rate")->convertTo0to1 (p.rate));
    apvts.getParameter ("depth")->setValueNotifyingHost (apvts.getParameter ("depth")->convertTo0to1 (p.depth));
    apvts.getParameter ("shape")->setValueNotifyingHost (apvts.getParameter ("shape")->convertTo0to1 (p.shape));
}

juce::AudioProcessorValueTreeState::ParameterLayout SlicerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rate", 1 }, "Rate",
        juce::NormalisableRange<float> (1.0f, 20.0f, 0.01f, 0.5f), 4.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 2) + " Hz"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "shape", 1 }, "Shape",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void SlicerProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    lfoSine.setSampleRate (sampleRate);
    lfoSine.setWaveform (openpedals::LFO::Waveform::Sine);
    lfoSine.reset();

    lfoSquare.setSampleRate (sampleRate);
    lfoSquare.setWaveform (openpedals::LFO::Waveform::Square);
    lfoSquare.reset();
}

void SlicerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float rate  = rateParam->load();
    const float depth = depthParam->load();
    const float shape = shapeParam->load();

    lfoSine.setFrequency (rate);
    lfoSquare.setFrequency (rate);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        // Get both LFO shapes (range: -1 to 1)
        float sineVal   = lfoSine.tick();
        float squareVal = lfoSquare.tick();

        // Map LFO from [-1, 1] to [0, 1]
        float sineMapped   = sineVal   * 0.5f + 0.5f;
        float squareMapped = squareVal * 0.5f + 0.5f;

        // Crossfade between sine and square based on shape parameter
        float gateValue = sineMapped * (1.0f - shape) + squareMapped * shape;

        // Apply depth: gate ranges from (1-depth) to 1
        float gate = 1.0f - depth * (1.0f - gateValue);

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample (ch, i, buffer.getSample (ch, i) * gate);
    }
}

void SlicerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SlicerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SlicerProcessor();
}
