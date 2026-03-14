/*
 * OpenPedals Spring Reverb — Spring reverb with metallic drip character
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

SpringReverbProcessor::SpringReverbProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    decayParam = apvts.getRawParameterValue ("decay");
    toneParam  = apvts.getRawParameterValue ("tone");
    mixParam   = apvts.getRawParameterValue ("mix");
    dripParam  = apvts.getRawParameterValue ("drip");

    setCurrentProgram (0);
}

void SpringReverbProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("decay")->setValueNotifyingHost (apvts.getParameter ("decay")->convertTo0to1 (p.decay));
    apvts.getParameter ("tone")->setValueNotifyingHost (apvts.getParameter ("tone")->convertTo0to1 (p.tone));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
    apvts.getParameter ("drip")->setValueNotifyingHost (apvts.getParameter ("drip")->convertTo0to1 (p.drip));
}

juce::AudioProcessorValueTreeState::ParameterLayout SpringReverbProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "decay", 1 }, "Decay",
        juce::NormalisableRange<float> (0.1f, 4.0f, 0.01f, 0.5f), 1.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " s"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 }, "Tone",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.35f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drip", 1 }, "Drip",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void SpringReverbProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 2;
    reverb.prepare (spec);

    // Pre-filter: bandpass around 1.5kHz to emphasize spring input character
    preFilterL.setParameters (openpedals::BiquadFilter::Type::BandPass, 1500.0, 0.7, 0.0, sampleRate);
    preFilterR.setParameters (openpedals::BiquadFilter::Type::BandPass, 1500.0, 0.7, 0.0, sampleRate);
    preFilterL.reset();
    preFilterR.reset();

    // Drip filters: resonant bandpass around 2.5kHz for metallic spring character
    dripFilterL.setParameters (openpedals::BiquadFilter::Type::BandPass, 2500.0, 3.0, 0.0, sampleRate);
    dripFilterR.setParameters (openpedals::BiquadFilter::Type::BandPass, 2500.0, 3.0, 0.0, sampleRate);
    dripFilterL.reset();
    dripFilterR.reset();

    // Tone filters: lowpass, will be updated per block
    toneFilterL.setParameters (openpedals::BiquadFilter::Type::LowPass, 8000.0, 0.707, 0.0, sampleRate);
    toneFilterR.setParameters (openpedals::BiquadFilter::Type::LowPass, 8000.0, 0.707, 0.0, sampleRate);
    toneFilterL.reset();
    toneFilterR.reset();

    // Allpass filters at different frequencies for metallic diffusion
    allpassL1.setParameters (openpedals::BiquadFilter::Type::AllPass, 800.0, 0.5, 0.0, sampleRate);
    allpassL2.setParameters (openpedals::BiquadFilter::Type::AllPass, 2200.0, 0.5, 0.0, sampleRate);
    allpassR1.setParameters (openpedals::BiquadFilter::Type::AllPass, 900.0, 0.5, 0.0, sampleRate);
    allpassR2.setParameters (openpedals::BiquadFilter::Type::AllPass, 2400.0, 0.5, 0.0, sampleRate);
    allpassL1.reset();
    allpassL2.reset();
    allpassR1.reset();
    allpassR2.reset();
}

void SpringReverbProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float decay = decayParam->load();
    const float tone  = toneParam->load();
    const float mix   = mixParam->load();
    const float drip  = dripParam->load();

    // Spring reverb: shorter decay, high damping for characteristic quick falloff
    const float roomSize = std::min (decay / 4.0f, 1.0f);

    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize   = roomSize;
    reverbParams.damping    = 0.6f;             // Higher damping for spring character
    reverbParams.wetLevel   = 1.0f;             // We handle wet/dry mix manually
    reverbParams.dryLevel   = 0.0f;
    reverbParams.width      = 0.6f;             // Narrower stereo for spring character
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters (reverbParams);

    // Update tone filter: map 0-1 to 1kHz-10kHz lowpass
    const float toneFreq = 1000.0f + tone * 9000.0f;
    toneFilterL.setParameters (openpedals::BiquadFilter::Type::LowPass, toneFreq, 0.707, 0.0, currentSampleRate);
    toneFilterR.setParameters (openpedals::BiquadFilter::Type::LowPass, toneFreq, 0.707, 0.0, currentSampleRate);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Store dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf (buffer);

    // Pre-filter input through allpass chain for metallic quality,
    // and blend with original to keep body
    for (int i = 0; i < numSamples; ++i)
    {
        if (numChannels > 0)
        {
            float sample = buffer.getSample (0, i);
            float filtered = allpassL1.process (sample);
            filtered = allpassL2.process (filtered);
            // Blend: 50% original + 50% allpass-filtered for metallic character
            buffer.setSample (0, i, sample * 0.5f + filtered * 0.5f);
        }
        if (numChannels > 1)
        {
            float sample = buffer.getSample (1, i);
            float filtered = allpassR1.process (sample);
            filtered = allpassR2.process (filtered);
            buffer.setSample (1, i, sample * 0.5f + filtered * 0.5f);
        }
    }

    // Process through reverb (wet only)
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    reverb.process (context);

    // Apply tone filter and drip resonance to the wet signal, then mix
    for (int i = 0; i < numSamples; ++i)
    {
        if (numChannels > 0)
        {
            float wet = buffer.getSample (0, i);
            float dry = dryBuffer.getSample (0, i);

            // Apply tone filtering
            wet = toneFilterL.process (wet);

            // Add drip: resonant bandpass emphasizes spring metallic character
            float dripSig = dripFilterL.process (wet);
            wet = wet + dripSig * drip * 1.5f;

            buffer.setSample (0, i, dry * (1.0f - mix) + wet * mix);
        }
        if (numChannels > 1)
        {
            float wet = buffer.getSample (1, i);
            float dry = dryBuffer.getSample (1, i);

            wet = toneFilterR.process (wet);

            float dripSig = dripFilterR.process (wet);
            wet = wet + dripSig * drip * 1.5f;

            buffer.setSample (1, i, dry * (1.0f - mix) + wet * mix);
        }
    }
}

void SpringReverbProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SpringReverbProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpringReverbProcessor();
}
