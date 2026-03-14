/*
 * OpenPedals Tape Delay — Tape echo simulation with saturation and flutter
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

TapeDelayProcessor::TapeDelayProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    timeParam       = apvts.getRawParameterValue ("time");
    feedbackParam   = apvts.getRawParameterValue ("feedback");
    mixParam        = apvts.getRawParameterValue ("mix");
    saturationParam = apvts.getRawParameterValue ("saturation");
    flutterParam    = apvts.getRawParameterValue ("flutter");

    setCurrentProgram (0);
}

void TapeDelayProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("time")->setValueNotifyingHost (apvts.getParameter ("time")->convertTo0to1 (p.time));
    apvts.getParameter ("feedback")->setValueNotifyingHost (apvts.getParameter ("feedback")->convertTo0to1 (p.feedback));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
    apvts.getParameter ("saturation")->setValueNotifyingHost (apvts.getParameter ("saturation")->convertTo0to1 (p.saturation));
    apvts.getParameter ("flutter")->setValueNotifyingHost (apvts.getParameter ("flutter")->convertTo0to1 (p.flutter));
}

juce::AudioProcessorValueTreeState::ParameterLayout TapeDelayProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "time", 1 }, "Time",
        juce::NormalisableRange<float> (50.0f, 1500.0f, 1.0f, 0.5f), 400.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + " ms"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "feedback", 1 }, "Feedback",
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.4f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.35f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "saturation", 1 }, "Saturation",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "flutter", 1 }, "Flutter",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void TapeDelayProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    const int maxDelaySamples = static_cast<int> (sampleRate * maxDelayMs / 1000.0) + 1;
    delayLineL.setMaxDelay (maxDelaySamples);
    delayLineR.setMaxDelay (maxDelaySamples);
    delayLineL.reset();
    delayLineR.reset();

    feedbackFilterL.reset();
    feedbackFilterR.reset();

    lfoL.setSampleRate (sampleRate);
    lfoR.setSampleRate (sampleRate);
    lfoL.setWaveform (openpedals::LFO::Waveform::Triangle);
    lfoR.setWaveform (openpedals::LFO::Waveform::Triangle);
    lfoL.reset();
    lfoR.reset();
    lfoR.setPhase (0.25); // 90° offset for stereo

    feedbackSampleL = 0.0f;
    feedbackSampleR = 0.0f;
}

void TapeDelayProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float delayMs    = timeParam->load();
    const float feedback   = feedbackParam->load();
    const float mix        = mixParam->load();
    const float saturation = saturationParam->load();
    const float flutter    = flutterParam->load();

    // Flutter LFO rate: 1-4Hz based on flutter parameter
    const float flutterRate = 1.0f + flutter * 3.0f;
    lfoL.setFrequency (static_cast<double> (flutterRate));
    lfoR.setFrequency (static_cast<double> (flutterRate));

    // Tape lowpass: progressive filtering, 2kHz-8kHz based on saturation
    // More saturation = darker (lower cutoff)
    const float tapeCutoff = 8000.0f - saturation * 6000.0f;
    feedbackFilterL.setParameters (openpedals::BiquadFilter::Type::LowPass, tapeCutoff, 0.707, 0.0, currentSampleRate);
    feedbackFilterR.setParameters (openpedals::BiquadFilter::Type::LowPass, tapeCutoff, 0.707, 0.0, currentSampleRate);

    const float baseDelaySamples = static_cast<float> (delayMs * currentSampleRate / 1000.0);

    // Flutter depth: up to ±2ms of wobble at max flutter
    const float maxFlutterDepthMs = 2.0f;
    const float flutterDepthSamples = flutter * maxFlutterDepthMs * static_cast<float> (currentSampleRate) / 1000.0f;

    // Saturation drive: maps 0-1 to 1.0-4.0 drive multiplier
    const float drive = 1.0f + saturation * 3.0f;

    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        float lfoValL = lfoL.tick();
        float lfoValR = lfoR.tick();

        float modulatedDelayL = baseDelaySamples + lfoValL * flutterDepthSamples;
        float modulatedDelayR = baseDelaySamples + lfoValR * flutterDepthSamples;

        modulatedDelayL = std::max (modulatedDelayL, 1.0f);
        modulatedDelayR = std::max (modulatedDelayR, 1.0f);

        // Left channel
        if (buffer.getNumChannels() > 0)
        {
            float dry = buffer.getSample (0, i);
            float delayedL = delayLineL.getInterpolated (modulatedDelayL);

            // Tape feedback path: softClip (saturation) → lowpass filter → delay
            float saturated = softClip (delayedL * drive) / drive;
            float filtered = feedbackFilterL.process (saturated);
            float toDelay = dry + filtered * feedback;
            delayLineL.push (toDelay);
            feedbackSampleL = delayedL;
            buffer.setSample (0, i, dry * (1.0f - mix) + delayedL * mix);
        }

        // Right channel
        if (buffer.getNumChannels() > 1)
        {
            float dry = buffer.getSample (1, i);
            float delayedR = delayLineR.getInterpolated (modulatedDelayR);

            // Tape feedback path: softClip (saturation) → lowpass filter → delay
            float saturated = softClip (delayedR * drive) / drive;
            float filtered = feedbackFilterR.process (saturated);
            float toDelay = dry + filtered * feedback;
            delayLineR.push (toDelay);
            feedbackSampleR = delayedR;
            buffer.setSample (1, i, dry * (1.0f - mix) + delayedR * mix);
        }
    }
}

void TapeDelayProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TapeDelayProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TapeDelayProcessor();
}
