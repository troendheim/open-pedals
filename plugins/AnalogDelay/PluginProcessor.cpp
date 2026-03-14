/*
 * OpenPedals Analog Delay — BBD-style delay with filtered feedback and modulation
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

AnalogDelayProcessor::AnalogDelayProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    timeParam     = apvts.getRawParameterValue ("time");
    feedbackParam = apvts.getRawParameterValue ("feedback");
    mixParam      = apvts.getRawParameterValue ("mix");
    modParam      = apvts.getRawParameterValue ("mod");

    setCurrentProgram (0);
}

void AnalogDelayProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("time")->setValueNotifyingHost (apvts.getParameter ("time")->convertTo0to1 (p.time));
    apvts.getParameter ("feedback")->setValueNotifyingHost (apvts.getParameter ("feedback")->convertTo0to1 (p.feedback));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
    apvts.getParameter ("mod")->setValueNotifyingHost (apvts.getParameter ("mod")->convertTo0to1 (p.mod));
}

juce::AudioProcessorValueTreeState::ParameterLayout AnalogDelayProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "time", 1 }, "Time",
        juce::NormalisableRange<float> (20.0f, 1000.0f, 1.0f, 0.5f), 300.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " ms"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "feedback", 1 }, "Feedback",
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.45f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.4f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mod", 1 }, "Mod",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void AnalogDelayProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    const int maxDelaySamples = static_cast<int> (sampleRate * maxDelayMs / 1000.0) + 1;
    delayLineL.setMaxDelay (maxDelaySamples);
    delayLineR.setMaxDelay (maxDelaySamples);
    delayLineL.reset();
    delayLineR.reset();

    feedbackFilterL.reset();
    feedbackFilterR.reset();

    // BBD lowpass at 3kHz to darken repeats
    feedbackFilterL.setParameters (openpedals::BiquadFilter::Type::LowPass, 3000.0, 0.707, 0.0, sampleRate);
    feedbackFilterR.setParameters (openpedals::BiquadFilter::Type::LowPass, 3000.0, 0.707, 0.0, sampleRate);

    lfoL.setSampleRate (sampleRate);
    lfoR.setSampleRate (sampleRate);
    lfoL.setWaveform (openpedals::LFO::Waveform::Triangle);
    lfoR.setWaveform (openpedals::LFO::Waveform::Triangle);
    lfoL.setFrequency (0.5);
    lfoR.setFrequency (0.5);
    lfoL.reset();
    lfoR.reset();
    lfoR.setPhase (0.25); // 90° offset for stereo

    feedbackSampleL = 0.0f;
    feedbackSampleR = 0.0f;
}

void AnalogDelayProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float delayMs  = timeParam->load();
    const float feedback = feedbackParam->load();
    const float mix      = mixParam->load();
    const float mod      = modParam->load();

    // BBD lowpass at 3kHz — fixed cutoff to emulate bucket-brigade darkening
    feedbackFilterL.setParameters (openpedals::BiquadFilter::Type::LowPass, 3000.0, 0.707, 0.0, currentSampleRate);
    feedbackFilterR.setParameters (openpedals::BiquadFilter::Type::LowPass, 3000.0, 0.707, 0.0, currentSampleRate);

    const float baseDelaySamples = static_cast<float> (delayMs * currentSampleRate / 1000.0);

    // Mod depth: up to ±1.5ms of wobble at max mod, scaled by base delay time
    const float maxModDepthMs = 1.5f;
    const float modDepthSamples = mod * maxModDepthMs * static_cast<float> (currentSampleRate) / 1000.0f;

    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        float lfoValL = lfoL.tick();
        float lfoValR = lfoR.tick();

        float modulatedDelayL = baseDelaySamples + lfoValL * modDepthSamples;
        float modulatedDelayR = baseDelaySamples + lfoValR * modDepthSamples;

        modulatedDelayL = std::max (modulatedDelayL, 1.0f);
        modulatedDelayR = std::max (modulatedDelayR, 1.0f);

        // Left channel
        if (buffer.getNumChannels() > 0)
        {
            float dry = buffer.getSample (0, i);
            float delayedL = delayLineL.getInterpolated (modulatedDelayL);
            float filteredFeedback = feedbackFilterL.process (delayedL);
            float toDelay = dry + filteredFeedback * feedback;
            delayLineL.push (toDelay);
            feedbackSampleL = delayedL;
            buffer.setSample (0, i, dry * (1.0f - mix) + delayedL * mix);
        }

        // Right channel
        if (buffer.getNumChannels() > 1)
        {
            float dry = buffer.getSample (1, i);
            float delayedR = delayLineR.getInterpolated (modulatedDelayR);
            float filteredFeedback = feedbackFilterR.process (delayedR);
            float toDelay = dry + filteredFeedback * feedback;
            delayLineR.push (toDelay);
            feedbackSampleR = delayedR;
            buffer.setSample (1, i, dry * (1.0f - mix) + delayedR * mix);
        }
    }
}

void AnalogDelayProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AnalogDelayProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnalogDelayProcessor();
}
