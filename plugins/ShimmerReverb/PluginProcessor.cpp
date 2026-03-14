/*
 * OpenPedals Shimmer Reverb — Reverb with pitch-shifted feedback for ethereal pads
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

ShimmerReverbProcessor::ShimmerReverbProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    decayParam   = apvts.getRawParameterValue ("decay");
    shimmerParam = apvts.getRawParameterValue ("shimmer");
    mixParam     = apvts.getRawParameterValue ("mix");
    toneParam    = apvts.getRawParameterValue ("tone");

    setCurrentProgram (0);
}

void ShimmerReverbProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& p = presets[index];
    apvts.getParameter ("decay")->setValueNotifyingHost (apvts.getParameter ("decay")->convertTo0to1 (p.decay));
    apvts.getParameter ("shimmer")->setValueNotifyingHost (apvts.getParameter ("shimmer")->convertTo0to1 (p.shimmer));
    apvts.getParameter ("mix")->setValueNotifyingHost (apvts.getParameter ("mix")->convertTo0to1 (p.mix));
    apvts.getParameter ("tone")->setValueNotifyingHost (apvts.getParameter ("tone")->convertTo0to1 (p.tone));
}

juce::AudioProcessorValueTreeState::ParameterLayout ShimmerReverbProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "decay", 1 }, "Decay",
        juce::NormalisableRange<float> (0.5f, 10.0f, 0.01f, 0.5f), 4.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " s"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "shimmer", 1 }, "Shimmer",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.4f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 }, "Tone",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value * 100.0f)) + "%"; }));

    return { params.begin(), params.end() };
}

void ShimmerReverbProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 2;
    reverb.prepare (spec);

    // Grain size: ~50ms window for pitch shifting
    grainSize = static_cast<int> (sampleRate * 0.05);
    const int grainDelaySamples = grainSize * 2 + 1;

    grainDelayL1.setMaxDelay (grainDelaySamples);
    grainDelayL2.setMaxDelay (grainDelaySamples);
    grainDelayR1.setMaxDelay (grainDelaySamples);
    grainDelayR2.setMaxDelay (grainDelaySamples);
    grainDelayL1.reset();
    grainDelayL2.reset();
    grainDelayR1.reset();
    grainDelayR2.reset();

    grainPhase = 0.0;
    shimmerFeedbackL = 0.0f;
    shimmerFeedbackR = 0.0f;

    // Tone filter on shimmer path
    toneFilterL.setParameters (openpedals::BiquadFilter::Type::LowPass, 8000.0, 0.707, 0.0, sampleRate);
    toneFilterR.setParameters (openpedals::BiquadFilter::Type::LowPass, 8000.0, 0.707, 0.0, sampleRate);
    toneFilterL.reset();
    toneFilterR.reset();
}

void ShimmerReverbProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float decay   = decayParam->load();
    const float shimmer = shimmerParam->load();
    const float mix     = mixParam->load();
    const float tone    = toneParam->load();

    // Map decay to room size
    const float roomSize = std::min (decay / 10.0f, 1.0f);

    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize   = roomSize;
    reverbParams.damping    = 0.3f;
    reverbParams.wetLevel   = 1.0f;     // We handle wet/dry mix manually
    reverbParams.dryLevel   = 0.0f;
    reverbParams.width      = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters (reverbParams);

    // Update tone filter: map 0-1 to 2kHz-12kHz lowpass on shimmer path
    const float toneFreq = 2000.0f + tone * 10000.0f;
    toneFilterL.setParameters (openpedals::BiquadFilter::Type::LowPass, toneFreq, 0.707, 0.0, currentSampleRate);
    toneFilterR.setParameters (openpedals::BiquadFilter::Type::LowPass, toneFreq, 0.707, 0.0, currentSampleRate);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Store dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf (buffer);

    // Inject shimmer feedback into the input before reverb processing
    // This creates the regenerative pitch-shifted reverb tail
    for (int i = 0; i < numSamples; ++i)
    {
        if (numChannels > 0)
            buffer.setSample (0, i, buffer.getSample (0, i) + shimmerFeedbackL * shimmer);
        if (numChannels > 1)
            buffer.setSample (1, i, buffer.getSample (1, i) + shimmerFeedbackR * shimmer);
    }

    // Process through reverb (wet only)
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    reverb.process (context);

    // Pitch-shift the reverb output up by one octave using two overlapping grains.
    // Each grain reads from the delay line at half the write speed (octave up).
    // A triangle window crossfades between the two grains offset by half a period.
    const double grainPhaseDelta = 1.0 / static_cast<double> (grainSize);

    for (int i = 0; i < numSamples; ++i)
    {
        // Compute triangle crossfade window for grain 1 and grain 2
        // Grain 1 window peaks at phase=0.5, grain 2 offset by 0.5
        float window1 = static_cast<float> (grainPhase < 0.5 ? grainPhase * 2.0 : 2.0 - grainPhase * 2.0);
        double grain2Phase = grainPhase + 0.5;
        if (grain2Phase >= 1.0)
            grain2Phase -= 1.0;
        float window2 = static_cast<float> (grain2Phase < 0.5 ? grain2Phase * 2.0 : 2.0 - grain2Phase * 2.0);

        // Read positions: for octave-up, read at half-speed.
        // Grain 1 delay offset ramps from 0 to grainSize as phase goes 0->1
        // Grain 2 is offset by half a grain period
        float delay1 = static_cast<float> (grainPhase * grainSize);
        float delay2 = static_cast<float> (grain2Phase * grainSize);

        // Ensure minimum delay of 1 sample
        delay1 = std::max (delay1, 1.0f);
        delay2 = std::max (delay2, 1.0f);

        if (numChannels > 0)
        {
            float wet = buffer.getSample (0, i);
            float dry = dryBuffer.getSample (0, i);

            // Push wet signal into both grain delay lines
            grainDelayL1.push (wet);
            grainDelayL2.push (wet);

            // Read at different offsets for octave-up effect
            float grain1 = grainDelayL1.getInterpolated (delay1);
            float grain2 = grainDelayL2.getInterpolated (delay2);

            // Crossfade between grains
            float pitched = grain1 * window1 + grain2 * window2;

            // Apply tone filter to the pitched signal
            pitched = toneFilterL.process (pitched);

            // Store feedback for next block (with limiting to prevent runaway)
            shimmerFeedbackL = std::clamp (pitched * 0.4f, -1.0f, 1.0f);

            // Final output: mix of dry and reverb wet (which includes shimmer feedback)
            buffer.setSample (0, i, dry * (1.0f - mix) + wet * mix);
        }

        if (numChannels > 1)
        {
            float wet = buffer.getSample (1, i);
            float dry = dryBuffer.getSample (1, i);

            grainDelayR1.push (wet);
            grainDelayR2.push (wet);

            float grain1 = grainDelayR1.getInterpolated (delay1);
            float grain2 = grainDelayR2.getInterpolated (delay2);

            float pitched = grain1 * window1 + grain2 * window2;

            pitched = toneFilterR.process (pitched);

            shimmerFeedbackR = std::clamp (pitched * 0.4f, -1.0f, 1.0f);

            buffer.setSample (1, i, dry * (1.0f - mix) + wet * mix);
        }

        // Advance grain phase
        grainPhase += grainPhaseDelta;
        if (grainPhase >= 1.0)
            grainPhase -= 1.0;
    }
}

void ShimmerReverbProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ShimmerReverbProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShimmerReverbProcessor();
}
