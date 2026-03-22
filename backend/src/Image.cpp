#include "Image.hpp"
#include <fstream>
#include <stdexcept>
#include <string>

namespace backend {

GrayImage loadImageAsGray(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    std::string magic;
    in >> magic;

    if (magic != "P2") {
        throw std::runtime_error("Only ASCII PGM (P2) supported");
    }

    int width, height, maxVal;
    in >> width >> height >> maxVal;

    GrayImage img(width, height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int value;
            in >> value;
            img.at(x, y) = static_cast<float>(value);
        }
    }

    return img;
}

void savePgm(const GrayImage& img, const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Cannot write file: " + path);
    }

    out << "P2\n";
    out << img.width << " " << img.height << "\n255\n";

    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            out << static_cast<int>(img.at(x, y)) << " ";
        }
        out << "\n";
    }
}

} // namespace backend