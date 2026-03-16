#pragma once

#include <vector>

#include "Image.hpp"

namespace backend {

struct LineDetection {
    double rho = 0.0;
    double theta = 0.0;
    int votes = 0;
};

std::vector<LineDetection> detectLinesHough(
    const GrayImage& edgeImage,
    int voteThreshold,
    int maxLines = 20,
    double thetaStepDegrees = 1.0,
    double rhoStep = 1.0);


} // namespace backend

