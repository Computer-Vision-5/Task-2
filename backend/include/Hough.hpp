#pragma once

#include <vector>
#include "Image.hpp"

namespace backend {

struct LineDetection {
    double rho = 0.0;
    double theta = 0.0;
    int votes = 0;
};

// --- NEW STRUCTS FOR PART 1 ---
struct CircleDetection {
    int centerX = 0;
    int centerY = 0;
    int radius = 0;
    int votes = 0;
};

struct EllipseDetection {
    int centerX = 0;
    int centerY = 0;
    int a = 0; // Semi-major axis
    int b = 0; // Semi-minor axis
    double angle = 0.0; // Rotation in radians
    int votes = 0;
};

std::vector<LineDetection> detectLinesHough(
    const GrayImage& edgeImage,
    int voteThreshold,
    int maxLines = 20,
    double thetaStepDegrees = 1.0,
    double rhoStep = 1.0);

// --- NEW DECLARATIONS FOR PART 1 ---
std::vector<CircleDetection> detectCirclesHough(
    const GrayImage& edgeImage,
    int minRadius, 
    int maxRadius, 
    int voteThreshold, 
    int maxCircles = 20);

std::vector<EllipseDetection> detectEllipsesHough(
    const GrayImage& edgeImage,
    int minA, int maxA, 
    int minB, int maxB,
    int voteThreshold, 
    int maxEllipses = 20);

} // namespace backend





















// #pragma once

// #include <vector>

// #include "Image.hpp"

// namespace backend {

// struct LineDetection {
//     double rho = 0.0;
//     double theta = 0.0;
//     int votes = 0;
// };

// std::vector<LineDetection> detectLinesHough(
//     const GrayImage& edgeImage,
//     int voteThreshold,
//     int maxLines = 20,
//     double thetaStepDegrees = 1.0,
//     double rhoStep = 1.0);


// } // namespace backend

