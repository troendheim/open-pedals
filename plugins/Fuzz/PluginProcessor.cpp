/*
 * OpenPedals Fuzz — Extreme asymmetric clipping fuzz pedal
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

FuzzProcessor::FuzzProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    fuzzParam   = apvts.getRawParameterValue ("fuzz");
    toneParam   = apvts.getRawParameterValue ("tone");
    volumeParam = apvts.getRawParameterValue ("volume");

    // Load the default preset (first one)
    setCurrentProgram (0);
}

void FuzzProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("fuzz")->setValueNotifyingHost (apvts.getParameter ("fuzz")->convertTo0to1 (p.fuzz));
    apvts.getParameter ("tone")->setValueNotifyingHost (apvts.getParameter ("tone")->convertTo0to1 (p.tone));
    apvts.getParameter ("volume")->setValueNotifyingHost (apvts.getParameter ("volume")->convertTo0to1 (p.volume));
}

juce::AudioProcessorValueTreeState::ParameterLayout FuzzProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "fuzz", 1 }, "Fuzz",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 }, "Tone",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.4f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "volume", 1 }, "Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.4f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void FuzzProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    oversampling.initProcessing (static_cast<size_t> (samplesPerBlock));
    oversampling.reset();
    toneFilterL.reset();
    toneFilterR.reset();

    // Report oversampling latency to the host for PDC (plugin delay compensation)
    setLatencySamples (static_cast<int> (oversampling.getLatencyInSamples()));
}

void FuzzProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float fuzz   = fuzzParam->load();
    const float tone   = toneParam->load();
    const float volume = volumeParam->load();

    // Map fuzz 0-1 to input gain 5x-200x (exponential mapping, extreme amplification)
    const float inputGain = 5.0f + fuzz * fuzz * 195.0f;

    // Map tone 0-1 to lowpass cutoff 500Hz-8kHz (logarithmic)
    const float toneCutoff = 500.0f * std::pow (16.0f, tone);

    // Map volume 0-1 to output gain
    const float outputGain = volume * 2.0f;

    // Update tone filters at oversampled rate
    const double oversampledRate = currentSampleRate * 4.0;
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
            // Apply extreme input gain
            float sample = channelData[i] * inputGain;

            // Asymmetric waveshaping cascade: tubeStyle first (asymmetric soft clip),
            // then hardClip to brutally flatten the peaks — classic fuzz character
            sample = openpedals::Waveshaper::tubeStyle (sample);
            sample = openpedals::Waveshaper::hardClip (sample, 0.7f);

            // Tone filter (lowpass to tame harsh high frequencies)
            sample = toneFilter.process (sample);

            // Output volume
            sample *= outputGain;

            channelData[i] = sample;
        }
    }

    // Downsample back to original rate
    oversampling.processSamplesDown (block);
}

void FuzzProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void FuzzProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FuzzProcessor();
}
