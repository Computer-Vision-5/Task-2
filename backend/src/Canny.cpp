#include "Canny.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace backend {

namespace {

int clampCoord(int value, int low, int high) {
    return std::max(low, std::min(value, high));
}

std::vector<float> makeGaussianKernel1D(int size, double sigma) {
    if (size < 3 || size % 2 == 0 || sigma <= 0.0) {
        throw std::invalid_argument("Invalid Gaussian kernel parameters");
    }

    std::vector<float> kernel(static_cast<size_t>(size), 0.0f);
    const int radius = size / 2;
    const double invTwoSigmaSq = 1.0 / (2.0 * sigma * sigma);
    double sum = 0.0;

    for (int i = -radius; i <= radius; ++i) {
        const double value = std::exp(-(static_cast<double>(i * i)) * invTwoSigmaSq);
        kernel[static_cast<size_t>(i + radius)] = static_cast<float>(value);
        sum += value;
    }

    for (float& v : kernel) {
        v = static_cast<float>(v / sum);
    }
    return kernel;
}

GrayImage gaussianBlurSeparable(const GrayImage& input, int kernelSize, double sigma) {
    const std::vector<float> kernel = makeGaussianKernel1D(kernelSize, sigma);
    const int radius = kernelSize / 2;

    GrayImage temp(input.width, input.height, 0.0f);
    GrayImage output(input.width, input.height, 0.0f);

    for (int y = 0; y < input.height; ++y) {
        for (int x = 0; x < input.width; ++x) {
            float sum = 0.0f;
            for (int k = -radius; k <= radius; ++k) {
                const int sx = clampCoord(x + k, 0, input.width - 1);
                sum += input.at(sx, y) * kernel[static_cast<size_t>(k + radius)];
            }
            temp.at(x, y) = sum;
        }
    }

    for (int y = 0; y < input.height; ++y) {
        for (int x = 0; x < input.width; ++x) {
            float sum = 0.0f;
            for (int k = -radius; k <= radius; ++k) {
                const int sy = clampCoord(y + k, 0, input.height - 1);
                sum += temp.at(x, sy) * kernel[static_cast<size_t>(k + radius)];
            }
            output.at(x, y) = sum;
        }
    }

    return output;
}

struct Gradients {
    GrayImage magnitude;
    GrayImage direction;
};

Gradients sobelGradients(const GrayImage& input) {
    static constexpr std::array<int, 9> kx = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
    static constexpr std::array<int, 9> ky = {1, 2, 1, 0, 0, 0, -1, -2, -1};

    Gradients g{GrayImage(input.width, input.height, 0.0f), GrayImage(input.width, input.height, 0.0f)};

    for (int y = 0; y < input.height; ++y) {
        for (int x = 0; x < input.width; ++x) {
            float gx = 0.0f;
            float gy = 0.0f;
            int idx = 0;
            for (int j = -1; j <= 1; ++j) {
                for (int i = -1; i <= 1; ++i) {
                    const int sx = clampCoord(x + i, 0, input.width - 1);
                    const int sy = clampCoord(y + j, 0, input.height - 1);
                    const float px = input.at(sx, sy);
                    gx += static_cast<float>(kx[static_cast<size_t>(idx)]) * px;
                    gy += static_cast<float>(ky[static_cast<size_t>(idx)]) * px;
                    ++idx;
                }
            }
            g.magnitude.at(x, y) = std::hypot(gx, gy);
            g.direction.at(x, y) = std::atan2(gy, gx);
        }
    }

    return g;
}

int quantizeDirection(float angleRadians) {
    double angle = angleRadians * 180.0 / 3.14159265358979323846;
    if (angle < 0.0) {
        angle += 180.0;
    }

    if ((angle >= 0.0 && angle < 22.5) || (angle >= 157.5 && angle <= 180.0)) {
        return 0;
    }
    if (angle >= 22.5 && angle < 67.5) {
        return 45;
    }
    if (angle >= 67.5 && angle < 112.5) {
        return 90;
    }
    return 135;
}

GrayImage nonMaximumSuppression(const GrayImage& magnitude, const GrayImage& direction) {
    GrayImage out(magnitude.width, magnitude.height, 0.0f);

    for (int y = 1; y < magnitude.height - 1; ++y) {
        for (int x = 1; x < magnitude.width - 1; ++x) {
            const float center = magnitude.at(x, y);
            const int dir = quantizeDirection(direction.at(x, y));

            float n1 = 0.0f;
            float n2 = 0.0f;

            if (dir == 0) {
                n1 = magnitude.at(x - 1, y);
                n2 = magnitude.at(x + 1, y);
            } else if (dir == 45) {
                n1 = magnitude.at(x - 1, y + 1);
                n2 = magnitude.at(x + 1, y - 1);
            } else if (dir == 90) {
                n1 = magnitude.at(x, y - 1);
                n2 = magnitude.at(x, y + 1);
            } else {
                n1 = magnitude.at(x - 1, y - 1);
                n2 = magnitude.at(x + 1, y + 1);
            }

            out.at(x, y) = (center >= n1 && center >= n2) ? center : 0.0f;
        }
    }

    return out;
}

GrayImage hysteresis(const GrayImage& suppressed, double lowThreshold, double highThreshold) {
    GrayImage edge(suppressed.width, suppressed.height, 0.0f);
    std::vector<std::pair<int, int>> stack;

    for (int y = 1; y < suppressed.height - 1; ++y) {
        for (int x = 1; x < suppressed.width - 1; ++x) {
            if (suppressed.at(x, y) >= highThreshold) {
                edge.at(x, y) = 255.0f;
                stack.emplace_back(x, y);
            }
        }
    }

    while (!stack.empty()) {
        const auto [x, y] = stack.back();
        stack.pop_back();

        for (int j = -1; j <= 1; ++j) {
            for (int i = -1; i <= 1; ++i) {
                if (i == 0 && j == 0) {
                    continue;
                }
                const int nx = x + i;
                const int ny = y + j;
                if (nx <= 0 || ny <= 0 || nx >= suppressed.width - 1 || ny >= suppressed.height - 1) {
                    continue;
                }
                if (edge.at(nx, ny) > 0.0f) {
                    continue;
                }
                if (suppressed.at(nx, ny) >= lowThreshold) {
                    edge.at(nx, ny) = 255.0f;
                    stack.emplace_back(nx, ny);
                }
            }
        }
    }

    return edge;
}

} // namespace

GrayImage cannyEdgeDetect(const GrayImage& input, const CannyParams& params) {
    if (!input.isValid()) {
        throw std::invalid_argument("Input image is invalid");
    }
    if (params.lowThreshold < 0.0 || params.highThreshold <= 0.0 || params.lowThreshold > params.highThreshold) {
        throw std::invalid_argument("Invalid Canny thresholds");
    }

    const GrayImage blurred = gaussianBlurSeparable(input, params.gaussianKernelSize, params.gaussianSigma);
    const Gradients gradients = sobelGradients(blurred);
    const GrayImage suppressed = nonMaximumSuppression(gradients.magnitude, gradients.direction);
    return hysteresis(suppressed, params.lowThreshold, params.highThreshold);
}

} // namespace backend

