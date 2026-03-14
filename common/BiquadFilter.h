/*
 * BiquadFilter.h — Second-order IIR filter (Audio EQ Cookbook)
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

class BiquadFilter
{
public:
    enum class Type
    {
        LowPass,
        HighPass,
        BandPass,
        Peaking,
        AllPass,
        LowShelf,
        HighShelf
    };

    BiquadFilter() = default;

    void setParameters (Type type, double frequency, double Q, double gainDB, double sampleRate)
    {
        double w0 = 2.0 * M_PI * frequency / sampleRate;
        double cosw0 = std::cos (w0);
        double sinw0 = std::sin (w0);
        double alpha = sinw0 / (2.0 * Q);

        double a0 = 1.0, a1 = 0.0, a2 = 0.0;
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;

        switch (type)
        {
            case Type::LowPass:
                b0 = (1.0 - cosw0) / 2.0;
                b1 =  1.0 - cosw0;
                b2 = (1.0 - cosw0) / 2.0;
                a0 =  1.0 + alpha;
                a1 = -2.0 * cosw0;
                a2 =  1.0 - alpha;
                break;

            case Type::HighPass:
                b0 =  (1.0 + cosw0) / 2.0;
                b1 = -(1.0 + cosw0);
                b2 =  (1.0 + cosw0) / 2.0;
                a0 =  1.0 + alpha;
                a1 = -2.0 * cosw0;
                a2 =  1.0 - alpha;
                break;

            case Type::BandPass:
                b0 =  alpha;
                b1 =  0.0;
                b2 = -alpha;
                a0 =  1.0 + alpha;
                a1 = -2.0 * cosw0;
                a2 =  1.0 - alpha;
                break;

            case Type::Peaking:
            {
                double A = std::pow (10.0, gainDB / 40.0);
                b0 =  1.0 + alpha * A;
                b1 = -2.0 * cosw0;
                b2 =  1.0 - alpha * A;
                a0 =  1.0 + alpha / A;
                a1 = -2.0 * cosw0;
                a2 =  1.0 - alpha / A;
                break;
            }

            case Type::AllPass:
                b0 =  1.0 - alpha;
                b1 = -2.0 * cosw0;
                b2 =  1.0 + alpha;
                a0 =  1.0 + alpha;
                a1 = -2.0 * cosw0;
                a2 =  1.0 - alpha;
                break;

            case Type::LowShelf:
            {
                double A = std::pow (10.0, gainDB / 40.0);
                double sqrtA = std::sqrt (A);
                b0 =      A * ((A + 1.0) - (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha);
                b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosw0);
                b2 =      A * ((A + 1.0) - (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha);
                a0 =           (A + 1.0) + (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha;
                a1 =    -2.0 * ((A - 1.0) + (A + 1.0) * cosw0);
                a2 =           (A + 1.0) + (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha;
                break;
            }

            case Type::HighShelf:
            {
                double A = std::pow (10.0, gainDB / 40.0);
                double sqrtA = std::sqrt (A);
                b0 =      A * ((A + 1.0) + (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha);
                b1 =-2.0 * A * ((A - 1.0) + (A + 1.0) * cosw0);
                b2 =      A * ((A + 1.0) + (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha);
                a0 =           (A + 1.0) - (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha;
                a1 =     2.0 * ((A - 1.0) - (A + 1.0) * cosw0);
                a2 =           (A + 1.0) - (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha;
                break;
            }
        }

        // Normalize coefficients
        coeffB0 = static_cast<float> (b0 / a0);
        coeffB1 = static_cast<float> (b1 / a0);
        coeffB2 = static_cast<float> (b2 / a0);
        coeffA1 = static_cast<float> (a1 / a0);
        coeffA2 = static_cast<float> (a2 / a0);
    }

    // Process a single sample (Direct Form II Transposed)
    float process (float input)
    {
        float output = coeffB0 * input + z1;
        z1 = coeffB1 * input - coeffA1 * output + z2;
        z2 = coeffB2 * input - coeffA2 * output;
        return output;
    }

    void reset()
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }

private:
    float coeffB0 = 1.0f, coeffB1 = 0.0f, coeffB2 = 0.0f;
    float coeffA1 = 0.0f, coeffA2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;
};

} // namespace bosslike
