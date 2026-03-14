/*
 * OpenPedals Pitch Shifter — Granular pitch shifter with two crossfaded grains
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
#include <cmath>

PitchShifterProcessor::PitchShifterProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    semitonesParam = apvts.getRawParameterValue ("semitones");
    fineParam      = apvts.getRawParameterValue ("fine");
    mixParam       = apvts.getRawParameterValue ("mix");

    setCurrentProgram (0);
}

void PitchShifterProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("semitones")->setValueNotifyingHost (apvts.getParameter ("semitones")->convertTo0to1 (p.semitones));
    apvts.getParameter ("fine")->setValueNotifyingHost (apvts.getParameter ("fine")->convertTo0to1 (p.fine));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
}

juce::AudioProcessorValueTreeState::ParameterLayout PitchShifterProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "semitones", 1 }, "Semitones",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 1.0f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " st"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "fine", 1 }, "Fine",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 1.0f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " ct"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void PitchShifterProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    // Grain size ~50ms, adjusted to sample rate
    grainSize = static_cast<int> (sampleRate * 0.05);
    if (grainSize < 256) grainSize = 256;

    const int bufferSize = grainSize * 4;

    for (auto& ch : channelState)
    {
        ch.delayLine.setMaxDelay (bufferSize);
        ch.delayLine.reset();
        ch.writePos  = 0;
        ch.grainPos0 = 0.0f;
        ch.grainPos1 = static_cast<float> (grainSize / 2);  // offset by half grain size
    }
}

float PitchShifterProcessor::processGrainPitchShift (ChannelState& state, float input, float pitchRatio)
{
    // Write input into the delay line
    state.delayLine.push (input);
    state.writePos++;

    // Read pointer increment: how much faster/slower we read relative to write
    float readIncrement = 1.0f - pitchRatio;

    // Advance both grain read positions
    state.grainPos0 += readIncrement;
    state.grainPos1 += readIncrement;

    float gs = static_cast<float> (grainSize);

    // Wrap grain positions to stay within [0, grainSize*2) range
    if (state.grainPos0 >= gs * 2.0f)
        state.grainPos0 -= gs * 2.0f;
    if (state.grainPos0 < 0.0f)
        state.grainPos0 += gs * 2.0f;
    if (state.grainPos1 >= gs * 2.0f)
        state.grainPos1 -= gs * 2.0f;
    if (state.grainPos1 < 0.0f)
        state.grainPos1 += gs * 2.0f;

    // Convert grain positions to delay times (samples back from write head)
    float delay0 = state.grainPos0;
    float delay1 = state.grainPos1;

    // Ensure delays are positive and within buffer
    delay0 = std::max (delay0, 1.0f);
    delay1 = std::max (delay1, 1.0f);

    // Read from delay line at each grain's position
    float grain0 = state.delayLine.getInterpolated (delay0);
    float grain1 = state.delayLine.getInterpolated (delay1);

    // Crossfade using raised cosine window based on position within grain cycle
    // Each grain fades in and out over its grain size period
    float phase0 = state.grainPos0 / gs;  // 0..2 range
    if (phase0 > 1.0f) phase0 -= 1.0f;    // wrap to 0..1
    float phase1 = state.grainPos1 / gs;
    if (phase1 > 1.0f) phase1 -= 1.0f;

    // Raised cosine window: 0.5 - 0.5*cos(2*pi*phase) — smooth fade
    float window0 = 0.5f - 0.5f * std::cos (2.0f * static_cast<float> (M_PI) * phase0);
    float window1 = 0.5f - 0.5f * std::cos (2.0f * static_cast<float> (M_PI) * phase1);

    return grain0 * window0 + grain1 * window1;
}

void PitchShifterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float semitones = semitonesParam->load();
    const float fine      = fineParam->load();
    const float mix       = mixParam->load();

    const float pitchRatio = std::pow (2.0f, (semitones + fine / 100.0f) / 12.0f);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < std::min (numChannels, 2); ++ch)
    {
        auto& state = channelState[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            float dry = buffer.getSample (ch, i);
            float wet = processGrainPitchShift (state, dry, pitchRatio);
            buffer.setSample (ch, i, dry * (1.0f - mix) + wet * mix);
        }
    }
}

void PitchShifterProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PitchShifterProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PitchShifterProcessor();
}
