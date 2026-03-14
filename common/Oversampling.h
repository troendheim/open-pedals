/*
 * Oversampling.h — Anti-aliasing wrapper around JUCE's oversampling
 *
 * Part of the OpenPedals guitar effects plugin collection.
 * Copyright (C) 2026 Richard Troendheim
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <functional>

namespace openpedals {

class Oversampling
{
public:
    // factor: 0 = 1x (no oversampling), 1 = 2x, 2 = 4x, 3 = 8x
    explicit Oversampling (int numChannels = 2, int factor = 2)
        : oversamplingFactor (factor)
    {
        oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
            numChannels,
            factor,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
            true  // isMaxQuality
        );
    }

    void prepare (double sampleRate, int blockSize)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (blockSize);
        spec.numChannels = static_cast<juce::uint32> (oversampler->getNumChannels());
        oversampler->initProcessing (static_cast<size_t> (blockSize));
    }

    void reset()
    {
        oversampler->reset();
    }

    // Process a block through the oversampler with a callback for the nonlinear processing
    // The callback receives an AudioBlock at the oversampled rate
    void process (juce::AudioBuffer<float>& buffer,
                  std::function<void (juce::dsp::AudioBlock<float>&)> processingCallback)
    {
        juce::dsp::AudioBlock<float> block (buffer);

        // Upsample
        auto oversampledBlock = oversampler->processSamplesUp (block);

        // Apply nonlinear processing at the oversampled rate
        processingCallback (oversampledBlock);

        // Downsample back
        oversampler->processSamplesDown (block);
    }

    float getLatencyInSamples() const
    {
        return oversampler->getLatencyInSamples();
    }

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    int oversamplingFactor;
};

} // namespace openpedals
