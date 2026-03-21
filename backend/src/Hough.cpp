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













std::vector<CircleDetection> detectCirclesHough(
    const GrayImage& edgeImage,
    int minRadius, int maxRadius, int voteThreshold, int maxCircles) {
    
    if (!edgeImage.isValid()) throw std::invalid_argument("Edge image is invalid");

    const int width = edgeImage.width;
    const int height = edgeImage.height;
    const int rRange = maxRadius - minRadius + 1;

    // 3D Accumulator flattened to 1D: A[r][y][x]
    std::vector<int> accumulator(static_cast<size_t>(width * height * rRange), 0);

    // Pre-calculate edges to speed up looping
    std::vector<std::pair<int, int>> edges;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (isEdgePixel(edgeImage, x, y)) edges.push_back({x, y});
        }
    }

    // Voting
    for (const auto& [x, y] : edges) {
        for (int r = minRadius; r <= maxRadius; ++r) {
            int rIdx = r - minRadius;
            // Step by 3 degrees to speed up voting (tradeoff: requires slightly lower threshold)
            for (int t = 0; t < 360; t += 3) { 
                double rad = t * kPi / 180.0;
                int a = static_cast<int>(std::round(x - r * std::cos(rad)));
                int b = static_cast<int>(std::round(y - r * std::sin(rad)));

                if (a >= 0 && a < width && b >= 0 && b < height) {
                    accumulator[static_cast<size_t>(rIdx * width * height + b * width + a)]++;
                }
            }
        }
    }

    // Peak Detection
    std::vector<CircleDetection> circles;
    for (int r = minRadius; r <= maxRadius; ++r) {
        int rIdx = r - minRadius;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int votes = accumulator[static_cast<size_t>(rIdx * width * height + y * width + x)];
                if (votes >= voteThreshold) {
                    circles.push_back({x, y, r, votes});
                }
            }
        }
    }

    keepTopByVotes(circles, maxCircles);
    return circles;
}





std::vector<EllipseDetection> detectEllipsesHough(
    const GrayImage& edgeImage,
    int minA, int maxA, int minB, int maxB,
    int voteThreshold, int maxEllipses) {
    
    if (!edgeImage.isValid()) throw std::invalid_argument("Edge image is invalid");

    const int width = edgeImage.width;
    const int height = edgeImage.height;
    std::vector<int> acc(static_cast<size_t>(width * height), 0);
    std::vector<EllipseDetection> results;

    // FIX 1: Collect ALL edge points. Don't skip pixels yet.
    std::vector<std::pair<int, int>> edges;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (edgeImage.at(x, y) > 128.0f) edges.push_back({x, y});
        }
    }

    if (edges.empty()) return {};

    // FIX 2: Tighten the steps. 5 was too sparse; 2 is much safer for small images.
    for (int a = minA; a <= maxA; a += 2) {
        for (int b = minB; b <= maxB; b += 2) {
            // FIX 3: Add more angles. 45 degrees misses many tilted ellipses. 
            // Try 22.5 degree steps.
            for (int angleDeg = 0; angleDeg < 180; angleDeg += 22) {
                std::fill(acc.begin(), acc.end(), 0);
                double angle = angleDeg * 3.14159 / 180.0;
                double cosA = std::cos(angle);
                double sinA = std::sin(angle);

                for (const auto& [x, y] : edges) {
                    // FIX 4: Vote density. Step by 5 degrees instead of 15.
                    for (int t = 0; t < 360; t += 5) {
                        double rad = t * 3.14159 / 180.0;
                        double ct = std::cos(rad);
                        double st = std::sin(rad);

                        // FIX 5: Use std::round instead of static_cast!
                        // static_cast(29.9) = 29, but std::round(29.9) = 30.
                        int cx = static_cast<int>(std::round(x - (a * ct * cosA - b * st * sinA)));
                        int cy = static_cast<int>(std::round(y - (a * ct * sinA + b * st * cosA)));

                        if (cx >= 0 && cx < width && cy >= 0 && cy < height) {
                            acc[cy * width + cx]++;
                        }
                    }
                }
                
                for (int cy = 0; cy < height; ++cy) {
                    for (int cx = 0; cx < width; ++cx) {
                        int votes = acc[cy * width + cx];
                        if (votes >= voteThreshold) {
                            results.push_back({cx, cy, a, b, angle, votes});
                        }
                    }
                }
            }
        }
    }

    // FIX 6: Sort and filter to prevent overlapping detections of the same object
    std::sort(results.begin(), results.end(), [](const auto& l, const auto& r) {
        return l.votes > r.votes;
    });

    if (static_cast<int>(results.size()) > maxEllipses) {
        results.resize(static_cast<size_t>(maxEllipses));
    }
    return results;
}






// std::vector<EllipseDetection> detectEllipsesHough(
//     const GrayImage& edgeImage,
//     int minA, int maxA, int minB, int maxB,
//     int voteThreshold, int maxEllipses) {
    
//     if (!edgeImage.isValid()) throw std::invalid_argument("Edge image is invalid");

//     const int width = edgeImage.width;
//     const int height = edgeImage.height;
    
//     // Optimization: Move accumulator outside the loops to avoid re-allocation
//     std::vector<int> acc(static_cast<size_t>(width * height), 0);
//     std::vector<EllipseDetection> results;

//     // Optimization: Collect edge points once
//     std::vector<std::pair<int, int>> edges;
//     for (int y = 0; y < height; ++y) {
//         for (int x = 0; x < width; ++x) {
//             if (isEdgePixel(edgeImage, x, y)) edges.push_back({x, y});
//         }
//     }

//     // Optimization: If there are too many edges, process only 50% of them
//     int step = (edges.size() > 2000) ? 2 : 1;

//     for (int a = minA; a <= maxA; a += 5) {
//         for (int b = minB; b <= maxB; b += 5) {
//             for (int angleDeg = 0; angleDeg < 180; angleDeg += 45) {
//                 std::fill(acc.begin(), acc.end(), 0);
//                 double angle = angleDeg * kPi / 180.0;
//                 float cosA = static_cast<float>(std::cos(angle));
//                 float sinA = static_cast<float>(std::sin(angle));

//                 for (size_t i = 0; i < edges.size(); i += step) {
//                     const auto& [x, y] = edges[i];
//                     // Optimization: Step by 15 degrees for very fast initial search
//                     for (int t = 0; t < 360; t += 15) {
//                         float rad = t * 0.0174533f; // Pre-calculated PI/180
//                         float ct = std::cos(rad);
//                         float st = std::sin(rad);

//                         int cx = static_cast<int>(x - (a * ct * cosA - b * st * sinA));
//                         int cy = static_cast<int>(y - (a * ct * sinA + b * st * cosA));

//                         if (cx >= 0 && cx < width && cy >= 0 && cy < height) {
//                             acc[static_cast<size_t>(cy * width + cx)]++;
//                         }
//                     }
//                 }
                
//                 for (int cy = 0; cy < height; ++cy) {
//                     for (int cx = 0; cx < width; ++cx) {
//                         if (acc[static_cast<size_t>(cy * width + cx)] >= voteThreshold) {
//                             results.push_back({cx, cy, a, b, angle, acc[static_cast<size_t>(cy * width + cx)]});
//                         }
//                     }
//                 }
//             }
//         }
//     }

//     keepTopByVotes(results, maxEllipses);
//     return results;
// }

// std::vector<EllipseDetection> detectEllipsesHough(
//     const GrayImage& edgeImage,
//     int minA, int maxA, int minB, int maxB,
//     int voteThreshold, int maxEllipses) {
    
//     if (!edgeImage.isValid()) throw std::invalid_argument("Edge image is invalid");

//     const int width = edgeImage.width;
//     const int height = edgeImage.height;
    
//     // Memory-Safe Approach: Reuse a 2D accumulator for each parameter combination
//     std::vector<int> acc(static_cast<size_t>(width * height), 0);
//     std::vector<EllipseDetection> results;

//     std::vector<std::pair<int, int>> edges;
//     for (int y = 0; y < height; ++y) {
//         for (int x = 0; x < width; ++x) {
//             if (isEdgePixel(edgeImage, x, y)) edges.push_back({x, y});
//         }
//     }

//     // Stepping by 3 to save CPU cycles. 
//     for (int a = minA; a <= maxA; a += 3) {
//         for (int b = minB; b <= maxB; b += 3) {
//             // Check 0, 45, 90, 135 degree rotations
//             for (int angleDeg = 0; angleDeg < 180; angleDeg += 45) {
//                 std::fill(acc.begin(), acc.end(), 0);
//                 double angle = angleDeg * kPi / 180.0;
//                 double cosA = std::cos(angle);
//                 double sinA = std::sin(angle);

//                 for (const auto& [x, y] : edges) {
//                     for (int t = 0; t < 360; t += 5) {
//                         double rad = t * kPi / 180.0;
//                         double ct = std::cos(rad);
//                         double st = std::sin(rad);

//                         int cx = static_cast<int>(std::round(x - (a * ct * cosA - b * st * sinA)));
//                         int cy = static_cast<int>(std::round(y - (a * ct * sinA + b * st * cosA)));

//                         if (cx >= 0 && cx < width && cy >= 0 && cy < height) {
//                             acc[static_cast<size_t>(cy * width + cx)]++;
//                         }
//                     }
//                 }
                
//                 // Extract peaks for this specific (a, b, angle) combination
//                 for (int cy = 0; cy < height; ++cy) {
//                     for (int cx = 0; cx < width; ++cx) {
//                         int votes = acc[static_cast<size_t>(cy * width + cx)];
//                         if (votes >= voteThreshold) {
//                             results.push_back({cx, cy, a, b, angle, votes});
//                         }
//                     }
//                 }
//             }
//         }
//     }

//     keepTopByVotes(results, maxEllipses);
//     return results;
// }





} // namespace backend
