/*
 * OpenPedals Humanizer — Formant filter for vowel-shaping effects
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

constexpr float HumanizerProcessor::vowelF1[];
constexpr float HumanizerProcessor::vowelF2[];

HumanizerProcessor::HumanizerProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    vowelParam     = apvts.getRawParameterValue ("vowel");
    resonanceParam = apvts.getRawParameterValue ("resonance");
    mixParam       = apvts.getRawParameterValue ("mix");

    setCurrentProgram (0);
}

void HumanizerProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("vowel")->setValueNotifyingHost (apvts.getParameter ("vowel")->convertTo0to1 (p.vowel));
    apvts.getParameter ("resonance")->setValueNotifyingHost (apvts.getParameter ("resonance")->convertTo0to1 (p.resonance));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
}

juce::AudioProcessorValueTreeState::ParameterLayout HumanizerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vowel", 1 }, "Vowel",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int)
        {
            // Display which vowel region we're in
            if (value < 0.125f)       return juce::String ("A");
            else if (value < 0.375f)  return juce::String ("A-E");
            else if (value < 0.625f)  return juce::String ("E-I");
            else if (value < 0.875f)  return juce::String ("I-O");
            else                      return juce::String ("O-U");
        }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "resonance", 1 }, "Resonance",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void HumanizerProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    formant1L.reset();
    formant1R.reset();
    formant2L.reset();
    formant2R.reset();
}

void HumanizerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float vowel     = vowelParam->load();
    const float resonance = resonanceParam->load();
    const float mix       = mixParam->load();

    // Map resonance 0..1 to Q range 2..15
    const float Q = 2.0f + resonance * 13.0f;

    // Map vowel 0..1 to interpolation across 5 vowels (A, E, I, O, U)
    // vowel 0.0 = A, 0.25 = E, 0.5 = I, 0.75 = O, 1.0 = U
    const float scaledVowel = vowel * static_cast<float> (numVowels - 1);
    const int vowelIndex = std::min (static_cast<int> (scaledVowel), numVowels - 2);
    const float vowelFrac = scaledVowel - static_cast<float> (vowelIndex);

    // Interpolate formant frequencies between adjacent vowels
    const float f1 = vowelF1[vowelIndex] * (1.0f - vowelFrac) + vowelF1[vowelIndex + 1] * vowelFrac;
    const float f2 = vowelF2[vowelIndex] * (1.0f - vowelFrac) + vowelF2[vowelIndex + 1] * vowelFrac;

    // Clamp frequencies to safe range
    const float safeF1 = std::min (f1, static_cast<float> (currentSampleRate) * 0.45f);
    const float safeF2 = std::min (f2, static_cast<float> (currentSampleRate) * 0.45f);

    // Set bandpass filter parameters for both formants
    formant1L.setParameters (openpedals::BiquadFilter::Type::BandPass, safeF1, Q, 0.0, currentSampleRate);
    formant1R.setParameters (openpedals::BiquadFilter::Type::BandPass, safeF1, Q, 0.0, currentSampleRate);
    formant2L.setParameters (openpedals::BiquadFilter::Type::BandPass, safeF2, Q, 0.0, currentSampleRate);
    formant2R.setParameters (openpedals::BiquadFilter::Type::BandPass, safeF2, Q, 0.0, currentSampleRate);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        if (numChannels > 0)
        {
            float dry = buffer.getSample (0, i);
            float wet = formant1L.process (dry) + formant2L.process (dry);
            buffer.setSample (0, i, dry * (1.0f - mix) + wet * mix);
        }

        if (numChannels > 1)
        {
            float dry = buffer.getSample (1, i);
            float wet = formant1R.process (dry) + formant2R.process (dry);
            buffer.setSample (1, i, dry * (1.0f - mix) + wet * mix);
        }
    }
}

void HumanizerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void HumanizerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HumanizerProcessor();
}
