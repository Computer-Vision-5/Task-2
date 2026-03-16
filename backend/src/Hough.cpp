#include "Hough.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace backend {

namespace {

constexpr double kPi = 3.14159265358979323846;

bool isEdgePixel(const GrayImage& edgeImage, int x, int y) {
    return edgeImage.at(x, y) > 0.0f;
}

template <typename T>
void keepTopByVotes(std::vector<T>& detections, int maxCount) {
    std::sort(detections.begin(), detections.end(), [](const T& a, const T& b) {
        return a.votes > b.votes;
    });
    if (static_cast<int>(detections.size()) > maxCount) {
        detections.resize(static_cast<size_t>(maxCount));
    }
}

} // namespace

std::vector<LineDetection> detectLinesHough(
    const GrayImage& edgeImage,
    int voteThreshold,
    int maxLines,
    double thetaStepDegrees,
    double rhoStep) {
    if (!edgeImage.isValid()) {
        throw std::invalid_argument("Edge image is invalid");
    }
    if (voteThreshold <= 0 || maxLines <= 0 || thetaStepDegrees <= 0.0 || rhoStep <= 0.0) {
        throw std::invalid_argument("Invalid Hough line parameters");
    }

    const int width = edgeImage.width;
    const int height = edgeImage.height;
    const double diag = std::hypot(static_cast<double>(width), static_cast<double>(height));
    const int rhoBins = static_cast<int>(std::ceil((2.0 * diag) / rhoStep)) + 1;
    const int thetaBins = static_cast<int>(std::ceil(180.0 / thetaStepDegrees));

    std::vector<int> accumulator(static_cast<size_t>(rhoBins * thetaBins), 0);

    std::vector<double> cosTable(static_cast<size_t>(thetaBins), 0.0);
    std::vector<double> sinTable(static_cast<size_t>(thetaBins), 0.0);
    for (int t = 0; t < thetaBins; ++t) {
        const double theta = (static_cast<double>(t) * thetaStepDegrees) * kPi / 180.0;
        cosTable[static_cast<size_t>(t)] = std::cos(theta);
        sinTable[static_cast<size_t>(t)] = std::sin(theta);
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!isEdgePixel(edgeImage, x, y)) {
                continue;
            }
            for (int t = 0; t < thetaBins; ++t) {
                const double rho = x * cosTable[static_cast<size_t>(t)] + y * sinTable[static_cast<size_t>(t)];
                const int r = static_cast<int>(std::round((rho + diag) / rhoStep));
                if (r >= 0 && r < rhoBins) {
                    ++accumulator[static_cast<size_t>(t * rhoBins + r)];
                }
            }
        }
    }

    std::vector<LineDetection> lines;
    for (int t = 0; t < thetaBins; ++t) {
        for (int r = 0; r < rhoBins; ++r) {
            const int votes = accumulator[static_cast<size_t>(t * rhoBins + r)];
            if (votes < voteThreshold) {
                continue;
            }
            const double theta = (static_cast<double>(t) * thetaStepDegrees) * kPi / 180.0;
            const double rho = (static_cast<double>(r) * rhoStep) - diag;
            lines.push_back(LineDetection{rho, theta, votes});
        }
    }

    keepTopByVotes(lines, maxLines);
    return lines;
}

std::vector<CircleDetection> detectCirclesHough(
    const GrayImage& edgeImage,
    int minRadius,
    int maxRadius,
    int voteThreshold,
    int maxCircles,
    int angleStepDegrees) {
    if (!edgeImage.isValid()) {
        throw std::invalid_argument("Edge image is invalid");
    }
    if (minRadius <= 0 || maxRadius < minRadius || voteThreshold <= 0 || maxCircles <= 0 || angleStepDegrees <= 0) {
        throw std::invalid_argument("Invalid Hough circle parameters");
    }

    const int width = edgeImage.width;
    const int height = edgeImage.height;

    std::vector<CircleDetection> circles;

    std::vector<double> cosTable;
    std::vector<double> sinTable;
    for (int angle = 0; angle < 360; angle += angleStepDegrees) {
        const double rad = static_cast<double>(angle) * kPi / 180.0;
        cosTable.push_back(std::cos(rad));
        sinTable.push_back(std::sin(rad));
    }

    for (int radius = minRadius; radius <= maxRadius; ++radius) {
        std::vector<int> accumulator(static_cast<size_t>(width * height), 0);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (!isEdgePixel(edgeImage, x, y)) {
                    continue;
                }
                for (size_t i = 0; i < cosTable.size(); ++i) {
                    const int cx = static_cast<int>(std::round(x - radius * cosTable[i]));
                    const int cy = static_cast<int>(std::round(y - radius * sinTable[i]));
                    if (cx >= 0 && cy >= 0 && cx < width && cy < height) {
                        ++accumulator[static_cast<size_t>(cy * width + cx)];
                    }
                }
            }
        }

        for (int cy = 0; cy < height; ++cy) {
            for (int cx = 0; cx < width; ++cx) {
                const int votes = accumulator[static_cast<size_t>(cy * width + cx)];
                if (votes >= voteThreshold) {
                    circles.push_back(CircleDetection{cx, cy, radius, votes});
                }
            }
        }
    }

    keepTopByVotes(circles, maxCircles);
    return circles;
}

} // namespace backend

