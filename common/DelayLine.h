/*
 * DelayLine.h — Circular buffer delay line with linear interpolation
 *
 * Part of the Bosslike guitar effects plugin collection.
 * Copyright (C) 2026 Richard Troendheim
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cassert>

namespace bosslike {

template <typename SampleType = float>
class DelayLine
{
public:
    DelayLine() = default;

    explicit DelayLine (int maxDelayInSamples)
    {
        setMaxDelay (maxDelayInSamples);
    }

    void setMaxDelay (int maxDelayInSamples)
    {
        assert (maxDelayInSamples > 0);
        maxDelay = maxDelayInSamples;
        buffer.resize (static_cast<size_t> (maxDelay + 1), SampleType (0));
        if (delay > maxDelay)
            delay = static_cast<SampleType> (maxDelay);
    }

    void setDelay (SampleType delayInSamples)
    {
        delay = std::clamp (delayInSamples, SampleType (0), static_cast<SampleType> (maxDelay));
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), SampleType (0));
        writePos = 0;
    }

    void push (SampleType sample)
    {
        buffer[static_cast<size_t> (writePos)] = sample;
        writePos = (writePos + 1) % static_cast<int> (buffer.size());
    }

    // Read from the delay line at the current delay setting (with linear interpolation)
    SampleType get() const
    {
        return getInterpolated (delay);
    }

    // Read at an arbitrary delay time with linear interpolation
    SampleType getInterpolated (SampleType delayInSamples) const
    {
        auto bufSize = static_cast<int> (buffer.size());
        SampleType readPos = static_cast<SampleType> (writePos) - delayInSamples;
        if (readPos < SampleType (0))
            readPos += static_cast<SampleType> (bufSize);

        int index0 = static_cast<int> (readPos);
        int index1 = (index0 + 1) % bufSize;
        SampleType frac = readPos - static_cast<SampleType> (index0);

        return buffer[static_cast<size_t> (index0)] * (SampleType (1) - frac)
             + buffer[static_cast<size_t> (index1)] * frac;
    }

    // Process one sample: push input, return delayed output
    SampleType process (SampleType input)
    {
        push (input);
        return get();
    }

private:
    std::vector<SampleType> buffer;
    int writePos = 0;
    int maxDelay = 0;
    SampleType delay = SampleType (0);
};

} // namespace bosslike
