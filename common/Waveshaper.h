/*
 * Waveshaper.h — Nonlinear transfer functions for overdrive/distortion
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
namespace Waveshaper {

    // Soft clipping using hyperbolic tangent — warm, tube-like character
    inline float softClip (float x)
    {
        return std::tanh (x);
    }

    // Hard clipping at a threshold — aggressive, square-wave-like
    inline float hardClip (float x, float threshold = 1.0f)
    {
        return std::clamp (x, -threshold, threshold);
    }

    // Asymmetric soft clipping — different behavior for positive and negative excursions
    // Simulates the asymmetric clipping of a single-ended tube stage
    inline float tubeStyle (float x)
    {
        if (x >= 0.0f)
            return 1.0f - std::exp (-x);  // Soft saturation on positive
        else
            return -std::tanh (-x);       // Tanh on negative
    }

    // Adjustable sigmoid — drive parameter controls the sharpness of the curve
    // drive: 1.0 = gentle, higher = more aggressive clipping
    inline float sigmoid (float x, float drive)
    {
        return (2.0f / (1.0f + std::exp (-drive * x))) - 1.0f;
    }

} // namespace Waveshaper
} // namespace bosslike
