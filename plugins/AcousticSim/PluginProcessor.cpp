/*
 * OpenPedals Acoustic Sim — Electric-to-acoustic guitar tone shaping
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

AcousticSimProcessor::AcousticSimProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    bodyParam  = apvts.getRawParameterValue ("body");
    topParam   = apvts.getRawParameterValue ("top");
    levelParam = apvts.getRawParameterValue ("level");

    setCurrentProgram (0);
}

void AcousticSimProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("body")->setValueNotifyingHost (apvts.getParameter ("body")->convertTo0to1 (p.body));
    apvts.getParameter ("top")->setValueNotifyingHost (apvts.getParameter ("top")->convertTo0to1 (p.top));
    apvts.getParameter ("level")->setValueNotifyingHost (apvts.getParameter ("level")->convertTo0to1 (p.level));
}

juce::AudioProcessorValueTreeState::ParameterLayout AcousticSimProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "body", 1 }, "Body",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "top", 1 }, "Top",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "level", 1 }, "Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void AcousticSimProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    bodyFilterL.reset();
    bodyFilterR.reset();
    midCutFilterL.reset();
    midCutFilterR.reset();
    topFilterL.reset();
    topFilterR.reset();
    lpFilterL.reset();
    lpFilterR.reset();
}

void AcousticSimProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float body  = bodyParam->load();
    const float top   = topParam->load();
    const float level = levelParam->load();

    // Body resonance: peaking boost around 300Hz, up to 8dB
    const float bodyGainDB = body * 8.0f;
    bodyFilterL.setParameters (openpedals::BiquadFilter::Type::Peaking, 300.0, 1.0, bodyGainDB, currentSampleRate);
    bodyFilterR.setParameters (openpedals::BiquadFilter::Type::Peaking, 300.0, 1.0, bodyGainDB, currentSampleRate);

    // Cut harsh upper mids around 2.5kHz by -6dB (fixed)
    midCutFilterL.setParameters (openpedals::BiquadFilter::Type::Peaking, 2500.0, 1.2, -6.0, currentSampleRate);
    midCutFilterR.setParameters (openpedals::BiquadFilter::Type::Peaking, 2500.0, 1.2, -6.0, currentSampleRate);

    // Top / presence: high shelf boost around 6kHz, up to 6dB
    const float topGainDB = top * 6.0f;
    topFilterL.setParameters (openpedals::BiquadFilter::Type::HighShelf, 6000.0, 0.707, topGainDB, currentSampleRate);
    topFilterR.setParameters (openpedals::BiquadFilter::Type::HighShelf, 6000.0, 0.707, topGainDB, currentSampleRate);

    // Lowpass rolloff above 10kHz
    lpFilterL.setParameters (openpedals::BiquadFilter::Type::LowPass, 10000.0, 0.707, 0.0, currentSampleRate);
    lpFilterR.setParameters (openpedals::BiquadFilter::Type::LowPass, 10000.0, 0.707, 0.0, currentSampleRate);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        if (numChannels > 0)
        {
            float sample = buffer.getSample (0, i);
            sample = bodyFilterL.process (sample);
            sample = midCutFilterL.process (sample);
            sample = topFilterL.process (sample);
            sample = lpFilterL.process (sample);
            buffer.setSample (0, i, sample * level);
        }

        if (numChannels > 1)
        {
            float sample = buffer.getSample (1, i);
            sample = bodyFilterR.process (sample);
            sample = midCutFilterR.process (sample);
            sample = topFilterR.process (sample);
            sample = lpFilterR.process (sample);
            buffer.setSample (1, i, sample * level);
        }
    }
}

void AcousticSimProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AcousticSimProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AcousticSimProcessor();
}
