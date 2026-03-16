#pragma once

#include <vector>

#include "Image.hpp"

namespace backend {

struct LineDetection {
    double rho = 0.0;
    double theta = 0.0;
    int votes = 0;
};

struct CircleDetection {
    int centerX = 0;
    int centerY = 0;
    int radius = 0;
    int votes = 0;
};

std::vector<LineDetection> detectLinesHough(
    const GrayImage& edgeImage,
    int voteThreshold,
    int maxLines = 20,
    double thetaStepDegrees = 1.0,
    double rhoStep = 1.0);

std::vector<CircleDetection> detectCirclesHough(
    const GrayImage& edgeImage,
    int minRadius,
    int maxRadius,
    int voteThreshold,
    int maxCircles = 20,
    int angleStepDegrees = 5);

} // namespace backend

