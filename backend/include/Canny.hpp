#pragma once

#include "Image.hpp"

namespace backend {

struct CannyParams {
    int gaussianKernelSize = 5;
    double gaussianSigma = 1.2;
    double lowThreshold = 30.0;
    double highThreshold = 80.0;
};

GrayImage cannyEdgeDetect(const GrayImage& input, const CannyParams& params = {});

} // namespace backend

