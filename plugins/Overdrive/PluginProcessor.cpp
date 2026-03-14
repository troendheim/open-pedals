/*
 * OpenPedals Overdrive — Tube-style soft-clipping overdrive pedal
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

OverdriveProcessor::OverdriveProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    driveParam = apvts.getRawParameterValue ("drive");
    toneParam  = apvts.getRawParameterValue ("tone");
    levelParam = apvts.getRawParameterValue ("level");
}

juce::AudioProcessorValueTreeState::ParameterLayout OverdriveProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 }, "Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 }, "Tone",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "level", 1 }, "Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    return { params.begin(), params.end() };
}

void OverdriveProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    oversampling.initProcessing (static_cast<size_t> (samplesPerBlock));
    oversampling.reset(); // Clear stale filter state to avoid clicks
    toneFilterL.reset();
    toneFilterR.reset();

    // Report oversampling latency to the host for PDC (plugin delay compensation)
    setLatencySamples (static_cast<int> (oversampling.getLatencyInSamples()));
}

void OverdriveProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float drive = driveParam->load();
    const float tone  = toneParam->load();
    const float level = levelParam->load();

    // Map drive 0-1 to input gain 1x-40x (exponential mapping)
    const float inputGain = 1.0f + drive * drive * 39.0f;

    // Map tone 0-1 to lowpass cutoff 800Hz-12kHz (logarithmic)
    const float toneCutoff = 800.0f * std::pow (15.0f, tone);

    // Map level 0-1 to output gain (with some compensation for drive loss)
    const float outputGain = level * 2.0f;

    // Update tone filters
    const double oversampledRate = currentSampleRate * 4.0; // 4x oversampling
    toneFilterL.setParameters (openpedals::BiquadFilter::Type::LowPass, toneCutoff, 0.707, 0.0, oversampledRate);
    toneFilterR.setParameters (openpedals::BiquadFilter::Type::LowPass, toneCutoff, 0.707, 0.0, oversampledRate);

    // Upsample, process, downsample
    juce::dsp::AudioBlock<float> block (buffer);
    auto oversampledBlock = oversampling.processSamplesUp (block);

    const int numChannels = static_cast<int> (oversampledBlock.getNumChannels());
    const int numSamples  = static_cast<int> (oversampledBlock.getNumSamples());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelData = oversampledBlock.getChannelPointer (static_cast<size_t> (ch));
        auto& toneFilter = (ch == 0) ? toneFilterL : toneFilterR;

        for (int i = 0; i < numSamples; ++i)
        {
            // Apply input gain
            float sample = channelData[i] * inputGain;

            // Waveshape: soft clip using tanh
            sample = openpedals::Waveshaper::softClip (sample);

            // Tone filter
            sample = toneFilter.process (sample);

            // Output level
            sample *= outputGain;

            channelData[i] = sample;
        }
    }

    // Downsample back to original rate
    oversampling.processSamplesDown (block);
}

void OverdriveProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void OverdriveProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OverdriveProcessor();
}
