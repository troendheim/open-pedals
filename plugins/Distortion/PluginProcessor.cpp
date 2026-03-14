/*
 * OpenPedals Distortion — Hard-clipping distortion pedal with selectable modes
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

DistortionProcessor::DistortionProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    gainParam     = apvts.getRawParameterValue ("gain");
    toneParam     = apvts.getRawParameterValue ("tone");
    levelParam    = apvts.getRawParameterValue ("level");
    clippingParam = apvts.getRawParameterValue ("clipping");

    // Load the default preset (first one)
    setCurrentProgram (0);
}

void DistortionProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("gain")->setValueNotifyingHost (apvts.getParameter ("gain")->convertTo0to1 (p.gain));
    apvts.getParameter ("tone")->setValueNotifyingHost (apvts.getParameter ("tone")->convertTo0to1 (p.tone));
    apvts.getParameter ("level")->setValueNotifyingHost (apvts.getParameter ("level")->convertTo0to1 (p.level));
    apvts.getParameter ("clipping")->setValueNotifyingHost (apvts.getParameter ("clipping")->convertTo0to1 (p.clipping));
}

juce::AudioProcessorValueTreeState::ParameterLayout DistortionProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 }, "Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 }, "Tone",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "level", 1 }, "Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "clipping", 1 }, "Clipping",
        juce::NormalisableRange<float> (0.0f, 2.0f, 1.0f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int)
        {
            if (value < 0.5f) return juce::String ("Hard");
            if (value < 1.5f) return juce::String ("Tube");
            return juce::String ("Fuzz");
        }));

    return { params.begin(), params.end() };
}

void DistortionProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    oversampling.initProcessing (static_cast<size_t> (samplesPerBlock));
    oversampling.reset();
    toneFilterL.reset();
    toneFilterR.reset();

    // Report oversampling latency to the host for PDC (plugin delay compensation)
    setLatencySamples (static_cast<int> (oversampling.getLatencyInSamples()));
}

void DistortionProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float gain     = gainParam->load();
    const float tone     = toneParam->load();
    const float level    = levelParam->load();
    const float clipping = clippingParam->load();

    // Map gain 0-1 to input gain 1x-100x (exponential mapping, more aggressive than overdrive)
    const float inputGain = 1.0f + gain * gain * 99.0f;

    // Map tone 0-1 to lowpass cutoff 500Hz-12kHz (logarithmic)
    const float toneCutoff = 500.0f * std::pow (24.0f, tone);

    // Map level 0-1 to output gain (with compensation for clipping loss)
    const float outputGain = level * 2.0f;

    // Determine clipping mode
    const int clipMode = static_cast<int> (std::round (clipping));

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
            // Apply input gain
            float sample = channelData[i] * inputGain;

            // Waveshape based on clipping mode
            switch (clipMode)
            {
                case 0:  // Hard clip — aggressive, square-wave-like distortion
                    sample = openpedals::Waveshaper::hardClip (sample, 0.8f);
                    break;
                case 1:  // Tube — asymmetric saturation, warmer feel
                    sample = openpedals::Waveshaper::tubeStyle (sample);
                    break;
                case 2:  // Fuzz — sigmoid with high drive for buzzy, saturated tone
                    sample = openpedals::Waveshaper::sigmoid (sample, 5.0f);
                    break;
                default:
                    sample = openpedals::Waveshaper::hardClip (sample, 0.8f);
                    break;
            }

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

void DistortionProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DistortionProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DistortionProcessor();
}
