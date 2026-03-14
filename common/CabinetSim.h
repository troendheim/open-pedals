/*
 * CabinetSim.h — Simple cabinet/speaker simulation using IIR filters
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

// Simple speaker cabinet simulation using cascaded filters
// Models the frequency response of a guitar speaker cabinet:
// - Highpass around 80-100Hz (speaker roll-off)
// - Resonant peak around 2-4kHz (speaker cone breakup)
// - Steep lowpass around 5-6kHz (no tweeter in guitar cabs)
class CabinetSim
{
public:
    CabinetSim() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        update();
    }

    // brightness: 0-1, controls the lowpass cutoff (3kHz-7kHz)
    // resonance: 0-1, controls the mid-peak (0-6dB at ~2.5kHz)
    void setParameters (float brightness, float resonance)
    {
        bright = brightness;
        reso = resonance;
        update();
    }

    float process (float input)
    {
        float s = highpass.process (input);
        s = midPeak.process (s);
        s = lowpass.process (s);
        return s;
    }

    void reset()
    {
        highpass.reset();
        midPeak.reset();
        lowpass.reset();
    }

private:
    void update()
    {
        // Highpass: speaker roll-off below ~80Hz
        highpass.setParameters (BiquadFilter::Type::HighPass, 80.0, 0.707, 0.0, sr);

        // Mid-peak: speaker resonance around 2.5kHz
        double peakGain = reso * 6.0;  // 0-6dB peak
        midPeak.setParameters (BiquadFilter::Type::Peaking, 2500.0, 1.5, peakGain, sr);

        // Lowpass: guitar cab has no tweeter, rolls off steeply
        double lpFreq = 3000.0 + bright * 4000.0;  // 3kHz-7kHz
        lowpass.setParameters (BiquadFilter::Type::LowPass, lpFreq, 0.707, 0.0, sr);
    }

    BiquadFilter highpass, midPeak, lowpass;
    double sr = 44100.0;
    float bright = 0.5f;
    float reso = 0.5f;
};

} // namespace openpedals
