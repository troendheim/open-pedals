/*
 * EnvelopeFollower.h — Amplitude envelope detector with attack/release
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
#include <algorithm>

namespace bosslike {

class EnvelopeFollower
{
public:
    EnvelopeFollower() = default;

    void setSampleRate (double newSampleRate)
    {
        sampleRate = newSampleRate;
        updateCoefficients();
    }

    void setAttack (double attackMs)
    {
        attackTime = attackMs;
        updateCoefficients();
    }

    void setRelease (double releaseMs)
    {
        releaseTime = releaseMs;
        updateCoefficients();
    }

    void reset()
    {
        envelope = 0.0f;
    }

    // Process a single sample, returns the current envelope value (always >= 0)
    float process (float input)
    {
        float absInput = std::abs (input);

        if (absInput > envelope)
            envelope = attackCoeff * (envelope - absInput) + absInput;
        else
            envelope = releaseCoeff * (envelope - absInput) + absInput;

        return envelope;
    }

    float getEnvelope() const { return envelope; }

private:
    void updateCoefficients()
    {
        if (sampleRate <= 0.0)
            return;

        // Time constant: coefficient = exp(-1 / (time_in_seconds * sampleRate))
        if (attackTime > 0.0)
            attackCoeff = static_cast<float> (std::exp (-1.0 / (attackTime * 0.001 * sampleRate)));
        else
            attackCoeff = 0.0f;

        if (releaseTime > 0.0)
            releaseCoeff = static_cast<float> (std::exp (-1.0 / (releaseTime * 0.001 * sampleRate)));
        else
            releaseCoeff = 0.0f;
    }

    double sampleRate = 44100.0;
    double attackTime = 10.0;   // ms
    double releaseTime = 100.0; // ms
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float envelope = 0.0f;
};

} // namespace bosslike
