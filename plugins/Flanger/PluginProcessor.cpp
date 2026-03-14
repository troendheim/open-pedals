/*
 * OpenPedals Flanger — Classic flanging with LFO-modulated short delay
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

FlangerProcessor::FlangerProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    rateParam     = apvts.getRawParameterValue ("rate");
    depthParam    = apvts.getRawParameterValue ("depth");
    feedbackParam = apvts.getRawParameterValue ("feedback");
    mixParam      = apvts.getRawParameterValue ("mix");

    setCurrentProgram (0);
}

void FlangerProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("rate")->setValueNotifyingHost (apvts.getParameter ("rate")->convertTo0to1 (p.rate));
    apvts.getParameter ("depth")->setValueNotifyingHost (apvts.getParameter ("depth")->convertTo0to1 (p.depth));
    apvts.getParameter ("feedback")->setValueNotifyingHost (apvts.getParameter ("feedback")->convertTo0to1 (p.feedback));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
}

juce::AudioProcessorValueTreeState::ParameterLayout FlangerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rate", 1 }, "Rate",
        juce::NormalisableRange<float> (0.05f, 5.0f, 0.01f, 0.5f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 2) + " Hz"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "feedback", 1 }, "Feedback",
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void FlangerProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    const int maxDelaySamples = static_cast<int> ((centerDelayMs + maxDepthMs + 2.0f) * sampleRate / 1000.0f);
    delayLineL.setMaxDelay (maxDelaySamples);
    delayLineR.setMaxDelay (maxDelaySamples);
    delayLineL.reset();
    delayLineR.reset();

    lfoL.setSampleRate (sampleRate);
    lfoR.setSampleRate (sampleRate);
    lfoL.setWaveform (openpedals::LFO::Waveform::Sine);
    lfoR.setWaveform (openpedals::LFO::Waveform::Sine);
    lfoL.reset();
    lfoR.reset();
    lfoR.setPhase (0.25);

    feedbackL = 0.0f;
    feedbackR = 0.0f;
}

void FlangerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float rate     = rateParam->load();
    const float depth    = depthParam->load();
    const float feedback = feedbackParam->load();
    const float mix      = mixParam->load();

    lfoL.setFrequency (rate);
    lfoR.setFrequency (rate);

    const float depthMs = depth * maxDepthMs;
    const float centerDelaySamples = centerDelayMs * static_cast<float> (currentSampleRate) / 1000.0f;
    const float depthSamples = depthMs * static_cast<float> (currentSampleRate) / 1000.0f;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        float lfoValL = lfoL.tick();
        float lfoValR = lfoR.tick();

        float modulatedDelayL = centerDelaySamples + lfoValL * depthSamples;
        float modulatedDelayR = centerDelaySamples + lfoValR * depthSamples;

        modulatedDelayL = std::max (modulatedDelayL, 1.0f);
        modulatedDelayR = std::max (modulatedDelayR, 1.0f);

        if (numChannels > 0)
        {
            float dry = buffer.getSample (0, i);
            delayLineL.push (dry + feedbackL * feedback);
            float wet = delayLineL.getInterpolated (modulatedDelayL);
            feedbackL = wet;
            buffer.setSample (0, i, dry * (1.0f - mix) + wet * mix);
        }

        if (numChannels > 1)
        {
            float dry = buffer.getSample (1, i);
            delayLineR.push (dry + feedbackR * feedback);
            float wet = delayLineR.getInterpolated (modulatedDelayR);
            feedbackR = wet;
            buffer.setSample (1, i, dry * (1.0f - mix) + wet * mix);
        }
    }
}

void FlangerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void FlangerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FlangerProcessor();
}
