#pragma once

#include <string>

#include "Image.hpp"

namespace backend {

// Loads PGM/PPM (P2/P3/P5/P6) and returns a grayscale image.
GrayImage loadImageAsGray(const std::string& path);
void savePgm(const GrayImage& image, const std::string& path);

} // namespace backend
