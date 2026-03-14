/*
 * ToneStack.h — 3-band tone stack (bass/mid/treble) using biquad filters
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

#include "BiquadFilter.h"

namespace openpedals {

// 3-band parametric tone stack using shelving + peaking EQ
class ToneStack
{
public:
    ToneStack() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        update();
    }

    // bass, mid, treble: -12 to +12 dB each
    void setParameters (float bassDB, float midDB, float trebleDB)
    {
        bass = bassDB;
        mid = midDB;
        treble = trebleDB;
        update();
    }

    float process (float input)
    {
        float s = bassFilter.process (input);
        s = midFilter.process (s);
        s = trebleFilter.process (s);
        return s;
    }

    void reset()
    {
        bassFilter.reset();
        midFilter.reset();
        trebleFilter.reset();
    }

private:
    void update()
    {
        bassFilter.setParameters (BiquadFilter::Type::LowShelf, 250.0, 0.707, bass, sr);
        midFilter.setParameters (BiquadFilter::Type::Peaking, 800.0, 1.0, mid, sr);
        trebleFilter.setParameters (BiquadFilter::Type::HighShelf, 3500.0, 0.707, treble, sr);
    }

    BiquadFilter bassFilter, midFilter, trebleFilter;
    double sr = 44100.0;
    float bass = 0.0f, mid = 0.0f, treble = 0.0f;
};

} // namespace openpedals
