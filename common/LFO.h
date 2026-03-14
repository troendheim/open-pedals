/*
 * LFO.h — Low-frequency oscillator with multiple waveforms
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

#include <cmath>

namespace bosslike {

class LFO
{
public:
    enum class Waveform
    {
        Sine,
        Triangle,
        Square,
        Saw
    };

    LFO() = default;

    void setSampleRate (double newSampleRate)
    {
        sampleRate = newSampleRate;
        updateIncrement();
    }

    void setFrequency (double newFrequency)
    {
        frequency = newFrequency;
        updateIncrement();
    }

    void setWaveform (Waveform newWaveform)
    {
        waveform = newWaveform;
    }

    void reset()
    {
        phase = 0.0;
    }

    // Returns value in range [-1, 1]
    float tick()
    {
        float value = 0.0f;

        switch (waveform)
        {
            case Waveform::Sine:
                value = static_cast<float> (std::sin (2.0 * M_PI * phase));
                break;

            case Waveform::Triangle:
                if (phase < 0.25)
                    value = static_cast<float> (phase * 4.0);
                else if (phase < 0.75)
                    value = static_cast<float> (2.0 - phase * 4.0);
                else
                    value = static_cast<float> (phase * 4.0 - 4.0);
                break;

            case Waveform::Square:
                value = phase < 0.5 ? 1.0f : -1.0f;
                break;

            case Waveform::Saw:
                value = static_cast<float> (2.0 * phase - 1.0);
                break;
        }

        phase += phaseIncrement;
        if (phase >= 1.0)
            phase -= 1.0;

        return value;
    }

private:
    void updateIncrement()
    {
        if (sampleRate > 0.0)
            phaseIncrement = frequency / sampleRate;
    }

    double phase = 0.0;
    double phaseIncrement = 0.0;
    double frequency = 1.0;
    double sampleRate = 44100.0;
    Waveform waveform = Waveform::Sine;
};

} // namespace bosslike
