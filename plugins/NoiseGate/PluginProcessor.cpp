/*
 * OpenPedals Noise Gate — Noise gate with attack, hold, and release
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

NoiseGateProcessor::NoiseGateProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    thresholdParam = apvts.getRawParameterValue ("threshold");
    attackParam    = apvts.getRawParameterValue ("attack");
    holdParam      = apvts.getRawParameterValue ("hold");
    releaseParam   = apvts.getRawParameterValue ("release");

    setCurrentProgram (0);
}

void NoiseGateProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("threshold")->setValueNotifyingHost (apvts.getParameter ("threshold")->convertTo0to1 (p.threshold));
    apvts.getParameter ("attack")->setValueNotifyingHost (apvts.getParameter ("attack")->convertTo0to1 (p.attack));
    apvts.getParameter ("hold")->setValueNotifyingHost (apvts.getParameter ("hold")->convertTo0to1 (p.hold));
    apvts.getParameter ("release")->setValueNotifyingHost (apvts.getParameter ("release")->convertTo0to1 (p.release));
}

juce::AudioProcessorValueTreeState::ParameterLayout NoiseGateProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 }, "Threshold",
        juce::NormalisableRange<float> (-80.0f, 0.0f, 0.1f), -40.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack", 1 }, "Attack",
        juce::NormalisableRange<float> (0.1f, 50.0f, 0.1f, 0.5f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " ms"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "hold", 1 }, "Hold",
        juce::NormalisableRange<float> (0.0f, 500.0f, 1.0f, 0.5f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " ms"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 }, "Release",
        juce::NormalisableRange<float> (5.0f, 500.0f, 1.0f, 0.5f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " ms"; }));

    return { params.begin(), params.end() };
}

void NoiseGateProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    envFollower.setSampleRate (sampleRate);
    envFollower.setAttack (0.1);   // fast envelope detection
    envFollower.setRelease (5.0);  // fast envelope detection
    envFollower.reset();

    gateGain = 0.0f;
    holdCounter = 0;
    gateOpen = false;
}

void NoiseGateProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float thresholdDB = thresholdParam->load();
    const float attackMs    = attackParam->load();
    const float holdMs      = holdParam->load();
    const float releaseMs   = releaseParam->load();

    // Convert threshold from dB to linear
    const float thresholdLin = juce::Decibels::decibelsToGain (thresholdDB);

    // Calculate smoothing coefficients for gate gain ramping
    const float attackCoeff  = (attackMs > 0.0f)
        ? 1.0f - std::exp (-1.0f / (attackMs * 0.001f * static_cast<float> (currentSampleRate)))
        : 1.0f;
    const float releaseCoeff = (releaseMs > 0.0f)
        ? 1.0f - std::exp (-1.0f / (releaseMs * 0.001f * static_cast<float> (currentSampleRate)))
        : 1.0f;

    // Hold time in samples
    const int holdSamples = static_cast<int> (holdMs * 0.001f * static_cast<float> (currentSampleRate));

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        // Detect level from left channel (or mono sum for stereo)
        float inputLevel = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            inputLevel = std::max (inputLevel, std::abs (buffer.getSample (ch, i)));

        float envelope = envFollower.process (inputLevel);

        // Gate logic
        if (envelope >= thresholdLin)
        {
            // Signal above threshold: open gate
            gateOpen = true;
            holdCounter = holdSamples;
        }
        else if (holdCounter > 0)
        {
            // In hold phase: keep gate open
            --holdCounter;
        }
        else
        {
            // Below threshold and hold expired: close gate
            gateOpen = false;
        }

        // Smooth the gate gain
        if (gateOpen)
            gateGain += attackCoeff * (1.0f - gateGain);
        else
            gateGain += releaseCoeff * (0.0f - gateGain);

        // Clamp gate gain
        gateGain = std::max (0.0f, std::min (1.0f, gateGain));

        // Apply gate gain to all channels
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample (ch, i, buffer.getSample (ch, i) * gateGain);
    }
}

void NoiseGateProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void NoiseGateProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NoiseGateProcessor();
}
