#include "Hough.hpp"
#include <algorithm>
#include <cmath>
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

    if (!edgeImage.isValid()) throw std::invalid_argument("Edge image is invalid");

    const int width = edgeImage.width;
    const int height = edgeImage.height;
    const double diag = std::hypot(static_cast<double>(width), static_cast<double>(height));
    const int rhoBins = static_cast<int>(std::ceil((2.0 * diag) / rhoStep)) + 1;
    const int thetaBins = static_cast<int>(std::ceil(180.0 / thetaStepDegrees));

    std::vector<int> accumulator(static_cast<size_t>(rhoBins * thetaBins), 0);

    // Precompute Trig Tables
    std::vector<double> cosTable(thetaBins), sinTable(thetaBins);
    for (int t = 0; t < thetaBins; ++t) {
        double theta = (static_cast<double>(t) * thetaStepDegrees) * kPi / 180.0;
        cosTable[t] = std::cos(theta);
        sinTable[t] = std::sin(theta);
    }

    // Voting
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!isEdgePixel(edgeImage, x, y)) continue;
            for (int t = 0; t < thetaBins; ++t) {
                double rho = x * cosTable[t] + y * sinTable[t];
                int r = static_cast<int>(std::round((rho + diag) / rhoStep));
                if (r >= 0 && r < rhoBins) {
                    ++accumulator[static_cast<size_t>(t * rhoBins + r)];
                }
            }
        }
    }

    // --- Peak Detection (Non-Maximum Suppression) ---
    std::vector<LineDetection> lines;
    for (int t = 0; t < thetaBins; ++t) {
        for (int r = 0; r < rhoBins; ++r) {
            int idx = t * rhoBins + r;
            int votes = accumulator[idx];
            if (votes < voteThreshold) continue;

            bool isPeak = true;
            for (int dt = -1; dt <= 1; ++dt) {
                for (int dr = -1; dr <= 1; ++dr) {
                    if (dt == 0 && dr == 0) continue;
                    int nt = t + dt, nr = r + dr;
                    if (nt < 0) nt = thetaBins - 1; // Wrap theta
                    if (nt >= thetaBins) nt = 0;
                    if (nr < 0 || nr >= rhoBins) continue;

                    if (accumulator[nt * rhoBins + nr] > votes) {
                        isPeak = false; break;
                    }
                }
                if (!isPeak) break;
            }

            if (isPeak) {
                double theta = (t * thetaStepDegrees) * kPi / 180.0;
                double rho = (r * rhoStep) - diag;
                lines.push_back({rho, theta, votes});
            }
        }
    }

    keepTopByVotes(lines, maxLines);
    return lines;
}

} // namespace backend
