/*
 * OpenPedals Tremolo — LFO-modulated amplitude effect
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

TremoloProcessor::TremoloProcessor()
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

void TremoloProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("rate")->setValueNotifyingHost (apvts.getParameter ("rate")->convertTo0to1 (p.rate));
    apvts.getParameter ("depth")->setValueNotifyingHost (apvts.getParameter ("depth")->convertTo0to1 (p.depth));
    apvts.getParameter ("shape")->setValueNotifyingHost (apvts.getParameter ("shape")->convertTo0to1 (p.shape));
}

juce::AudioProcessorValueTreeState::ParameterLayout TremoloProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rate", 1 }, "Rate",
        juce::NormalisableRange<float> (0.5f, 15.0f, 0.01f, 0.5f), 4.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 2) + " Hz"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "shape", 1 }, "Shape",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int)
        {
            if (value < 0.25f)       return juce::String ("Sine");
            else if (value < 0.75f)  return juce::String ("Triangle");
            else                     return juce::String ("Square");
        }));

    return { params.begin(), params.end() };
}

void TremoloProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    lfoSine.setSampleRate (sampleRate);
    lfoTriangle.setSampleRate (sampleRate);
    lfoSquare.setSampleRate (sampleRate);

    lfoSine.setWaveform (openpedals::LFO::Waveform::Sine);
    lfoTriangle.setWaveform (openpedals::LFO::Waveform::Triangle);
    lfoSquare.setWaveform (openpedals::LFO::Waveform::Square);

    lfoSine.reset();
    lfoTriangle.reset();
    lfoSquare.reset();
}

void TremoloProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float rate  = rateParam->load();
    const float depth = depthParam->load();
    const float shape = shapeParam->load();

    lfoSine.setFrequency (rate);
    lfoTriangle.setFrequency (rate);
    lfoSquare.setFrequency (rate);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        float sinVal = lfoSine.tick();
        float triVal = lfoTriangle.tick();
        float sqrVal = lfoSquare.tick();

        // Crossfade between waveforms: 0=Sine, 0.5=Triangle, 1.0=Square
        float lfoVal;
        if (shape < 0.5f)
        {
            float t = shape * 2.0f;
            lfoVal = sinVal * (1.0f - t) + triVal * t;
        }
        else
        {
            float t = (shape - 0.5f) * 2.0f;
            lfoVal = triVal * (1.0f - t) + sqrVal * t;
        }

        // Convert bipolar LFO (-1..1) to unipolar (0..1) for amplitude modulation
        float modulation = 1.0f - depth * (lfoVal * 0.5f + 0.5f);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float sample = buffer.getSample (ch, i);
            buffer.setSample (ch, i, sample * modulation);
        }
    }
}

void TremoloProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TremoloProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TremoloProcessor();
}
