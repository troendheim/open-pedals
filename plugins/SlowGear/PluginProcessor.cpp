/*
 * OpenPedals Slow Gear — Auto-swell volume effect for violin-like attacks
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

SlowGearProcessor::SlowGearProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    sensitivityParam = apvts.getRawParameterValue ("sensitivity");
    attackParam      = apvts.getRawParameterValue ("attack");
    levelParam       = apvts.getRawParameterValue ("level");

    setCurrentProgram (0);
}

void SlowGearProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("sensitivity")->setValueNotifyingHost (apvts.getParameter ("sensitivity")->convertTo0to1 (p.sensitivity));
    apvts.getParameter ("attack")->setValueNotifyingHost (apvts.getParameter ("attack")->convertTo0to1 (p.attack));
    apvts.getParameter ("level")->setValueNotifyingHost (apvts.getParameter ("level")->convertTo0to1 (p.level));
}

juce::AudioProcessorValueTreeState::ParameterLayout SlowGearProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sensitivity", 1 }, "Sensitivity",
        juce::NormalisableRange<float> (-60.0f, -10.0f, 0.1f), -30.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack", 1 }, "Attack",
        juce::NormalisableRange<float> (10.0f, 2000.0f, 1.0f, 0.5f), 200.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " ms"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "level", 1 }, "Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void SlowGearProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    envelopeFollower.setSampleRate (sampleRate);
    envelopeFollower.setAttack (1.0);   // Fast attack for onset detection
    envelopeFollower.setRelease (20.0); // Moderate release for onset detection
    envelopeFollower.reset();

    swellGain = 0.0f;
    swellActive = false;
    attackIncrement = 0.0f;
}

void SlowGearProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float sensitivity = sensitivityParam->load();
    const float attackMs    = attackParam->load();
    const float level       = levelParam->load();

    // Convert sensitivity from dB to linear threshold
    const float threshold = juce::Decibels::decibelsToGain (sensitivity);

    // Per-sample attack increment: ramp from 0 to 1 over attackMs
    const float attackSamples = static_cast<float> (currentSampleRate) * attackMs / 1000.0f;
    attackIncrement = (attackSamples > 0.0f) ? (1.0f / attackSamples) : 1.0f;

    // Per-sample release decrement: fast fade-out (20ms)
    const float releaseSamples = static_cast<float> (currentSampleRate) * 0.02f;
    const float releaseDecrement = (releaseSamples > 0.0f) ? (1.0f / releaseSamples) : 1.0f;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        // Use left channel (or mono) for envelope detection
        float inputSample = (numChannels > 0) ? buffer.getSample (0, i) : 0.0f;
        float env = envelopeFollower.process (inputSample);

        // Onset detection: signal crosses threshold
        if (env > threshold && !swellActive)
        {
            swellActive = true;
            swellGain = 0.0f; // Start swell from silence
        }

        // Update swell gain
        if (swellActive)
        {
            swellGain += attackIncrement;
            if (swellGain >= 1.0f)
                swellGain = 1.0f;
        }

        // When signal drops below threshold, fade out
        if (env < threshold * 0.5f)
        {
            swellActive = false;
            swellGain -= releaseDecrement;
            if (swellGain < 0.0f)
                swellGain = 0.0f;
        }

        // Apply gain to all channels
        float gain = swellGain * level;
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample (ch, i, buffer.getSample (ch, i) * gain);
    }
}

void SlowGearProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SlowGearProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SlowGearProcessor();
}
