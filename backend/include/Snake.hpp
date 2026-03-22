#pragma once

#include "Image.hpp"

#include <array>
#include <vector>

namespace backend {

/// A contour is an ordered, closed list of 2-D points {x, y}.
using SnakeContour = std::vector<std::array<float, 2>>;

/// Parameters that drive both initialisation and evolution.
struct SnakeParams {
    float alpha                = 0.4f;   ///< Weight for the elasticity energy (Eelastic)
    float beta                 = 0.4f;   ///< Weight for the smoothness energy (Esmooth)
    int   windowSize           = 5;      ///< Half-size W of the search window (full = 2W+1)
    int   numPoints            = 60;     ///< Number of uniformly-sampled contour points
    int   maxIterations        = 200;    ///< Hard iteration cap
    float convergenceThreshold = 0.5f;  ///< Stop when total displacement < this (pixels)
};

/// Part A — Initialise a uniformly-sampled circular contour.
///
/// \param cx       Centre x (image coordinates)
/// \param cy       Centre y (image coordinates)
/// \param radius   Circle radius in pixels
/// \param numPoints Number of points to sample
/// \returns        Closed contour (first == last point NOT repeated)
SnakeContour initContour(float cx, float cy, float radius, int numPoints);

/// Part B — Greedy energy-minimisation evolution.
///
/// Uses the gradient magnitude already computed by the Canny pipeline
/// (do NOT recompute it here).
///
/// \param initial          Initialised contour from initContour()
/// \param gradientMagnitude Pre-computed Sobel magnitude map [0..∞]
/// \param params           Algorithm hyper-parameters
/// \returns                Final contour after convergence or maxIterations
SnakeContour evolveContour(const SnakeContour&  initial,
                           const GrayImage&      gradientMagnitude,
                           const SnakeParams&    params);

} // namespace backend
