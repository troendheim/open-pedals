/*
 * OpenPedals Amp Sim — Preamp, tone stack, power amp and cabinet simulation
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

AmpSimProcessor::AmpSimProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    gainParam     = apvts.getRawParameterValue ("gain");
    bassParam     = apvts.getRawParameterValue ("bass");
    midParam      = apvts.getRawParameterValue ("mid");
    trebleParam   = apvts.getRawParameterValue ("treble");
    volumeParam   = apvts.getRawParameterValue ("volume");
    presenceParam = apvts.getRawParameterValue ("presence");

    // Load the default preset (first one)
    setCurrentProgram (0);
}

void AmpSimProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("gain")->setValueNotifyingHost (apvts.getParameter ("gain")->convertTo0to1 (p.gain));
    apvts.getParameter ("bass")->setValueNotifyingHost (apvts.getParameter ("bass")->convertTo0to1 (p.bass));
    apvts.getParameter ("mid")->setValueNotifyingHost (apvts.getParameter ("mid")->convertTo0to1 (p.mid));
    apvts.getParameter ("treble")->setValueNotifyingHost (apvts.getParameter ("treble")->convertTo0to1 (p.treble));
    apvts.getParameter ("volume")->setValueNotifyingHost (apvts.getParameter ("volume")->convertTo0to1 (p.volume));
    apvts.getParameter ("presence")->setValueNotifyingHost (apvts.getParameter ("presence")->convertTo0to1 (p.presence));
}

juce::AudioProcessorValueTreeState::ParameterLayout AmpSimProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 }, "Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "bass", 1 }, "Bass",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mid", 1 }, "Mid",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "treble", 1 }, "Treble",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "volume", 1 }, "Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "presence", 1 }, "Presence",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void AmpSimProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    oversampling.initProcessing (static_cast<size_t> (samplesPerBlock));
    oversampling.reset();

    // Tone stack and cabinet run at base sample rate (after downsampling)
    toneStackL.prepare (sampleRate);
    toneStackR.prepare (sampleRate);
    cabinetL.prepare (sampleRate);
    cabinetR.prepare (sampleRate);

    // Report oversampling latency to the host for PDC (plugin delay compensation)
    setLatencySamples (static_cast<int> (oversampling.getLatencyInSamples()));
}

void AmpSimProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float gain     = gainParam->load();
    const float bass     = bassParam->load();
    const float mid      = midParam->load();
    const float treble   = trebleParam->load();
    const float volume   = volumeParam->load();
    const float presence = presenceParam->load();

    // Map gain 0-1 to preamp gain 1x-50x (exponential mapping)
    const float preampGain = 1.0f + gain * gain * 49.0f;

    // Map volume 0-1 to output gain
    const float outputGain = volume * 2.0f;

    // Update tone stack EQ
    toneStackL.setParameters (bass, mid, treble);
    toneStackR.setParameters (bass, mid, treble);

    // Update cabinet sim: presence maps to brightness, fixed resonance
    cabinetL.setParameters (presence, 0.5f);
    cabinetR.setParameters (presence, 0.5f);

    // =====================================================================
    // Oversampled section: preamp gain stage + power amp saturation
    // =====================================================================
    juce::dsp::AudioBlock<float> block (buffer);
    auto oversampledBlock = oversampling.processSamplesUp (block);

    const int numChannels = static_cast<int> (oversampledBlock.getNumChannels());
    const int numSamples  = static_cast<int> (oversampledBlock.getNumSamples());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelData = oversampledBlock.getChannelPointer (static_cast<size_t> (ch));

        for (int i = 0; i < numSamples; ++i)
        {
            // Preamp gain stage: sigmoid waveshaper with adjustable gain
            float sample = channelData[i] * preampGain;
            sample = openpedals::Waveshaper::sigmoid (sample, 2.0f);

            // Power amp stage: gentle tube-style saturation
            sample = openpedals::Waveshaper::tubeStyle (sample);

            channelData[i] = sample;
        }
    }

    // Downsample back to original rate
    oversampling.processSamplesDown (block);

    // =====================================================================
    // Post-oversampling: tone stack + cabinet sim (at base sample rate)
    // =====================================================================
    const int baseSamples = buffer.getNumSamples();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* channelData = buffer.getWritePointer (ch);
        auto& toneStack = (ch == 0) ? toneStackL : toneStackR;
        auto& cabinet   = (ch == 0) ? cabinetL   : cabinetR;

        for (int i = 0; i < baseSamples; ++i)
        {
            // Tone stack (3-band EQ)
            float sample = toneStack.process (channelData[i]);

            // Cabinet simulation (speaker emulation)
            sample = cabinet.process (sample);

            // Master volume
            sample *= outputGain;

            channelData[i] = sample;
        }
    }
}

void AmpSimProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AmpSimProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AmpSimProcessor();
}
