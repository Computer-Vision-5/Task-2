#pragma once

#include <cstddef>
#include <vector>

namespace backend {

struct GrayImage {
    int width = 0;
    int height = 0;
    std::vector<float> pixels;

    GrayImage() = default;
    GrayImage(int w, int h, float value = 0.0f) : width(w), height(h), pixels(static_cast<std::size_t>(w * h), value) {}

    [[nodiscard]] bool isValid() const { return width > 0 && height > 0 && pixels.size() == static_cast<std::size_t>(width * height); }

    float& at(int x, int y) { return pixels.at(static_cast<std::size_t>(y * width + x)); }
    float at(int x, int y) const { return pixels.at(static_cast<std::size_t>(y * width + x)); }
};

} // namespace backend
