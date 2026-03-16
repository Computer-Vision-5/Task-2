#include "ImageIO.hpp"
#include <cstdint> // This defines uint8_t

#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace backend {

namespace {

std::string readToken(std::istream& input) {
    std::string token;
    while (input >> token) {
        if (!token.empty() && token[0] == '#') {
            std::string restOfLine;
            std::getline(input, restOfLine);
            continue;
        }
        return token;
    }
    return {};
}

std::uint8_t clampToByte(float value) {
    const auto clamped = std::clamp(value, 0.0f, 255.0f);
    return static_cast<std::uint8_t>(clamped + 0.5f);
}

float scaleToByte(int sample, int maxValue) {
    return (static_cast<float>(sample) * 255.0f) / static_cast<float>(maxValue);
}

int readBinarySample(std::istream& input, int maxValue) {
    if (maxValue <= 255) {
        unsigned char byte = 0;
        input.read(reinterpret_cast<char*>(&byte), 1);
        if (!input) {
            throw std::runtime_error("Unexpected end of binary image data");
        }
        return static_cast<int>(byte);
    }

    unsigned char hi = 0;
    unsigned char lo = 0;
    input.read(reinterpret_cast<char*>(&hi), 1);
    input.read(reinterpret_cast<char*>(&lo), 1);
    if (!input) {
        throw std::runtime_error("Unexpected end of binary image data");
    }
    return (static_cast<int>(hi) << 8) | static_cast<int>(lo);
}

} // namespace

GrayImage loadImageAsGray(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Failed to open image file: " + path);
    }

    const std::string magic = readToken(input);
    const bool isAscii = (magic == "P2" || magic == "P3");
    const bool isGray = (magic == "P2" || magic == "P5");
    const bool isColor = (magic == "P3" || magic == "P6");

    if (!isGray && !isColor) {
        throw std::runtime_error("Unsupported Netpbm format (use P2/P3/P5/P6): " + magic);
    }

    const int width = std::stoi(readToken(input));
    const int height = std::stoi(readToken(input));
    const int maxValue = std::stoi(readToken(input));
    if (width <= 0 || height <= 0 || maxValue <= 0 || maxValue > 65535) {
        throw std::runtime_error("Invalid image header values");
    }

    GrayImage image(width, height, 0.0f);

    if (isAscii) {
        if (isGray) {
            for (int i = 0; i < width * height; ++i) {
                const std::string tok = readToken(input);
                if (tok.empty()) {
                    throw std::runtime_error("Unexpected end of P2 file");
                }
                image.pixels[static_cast<std::size_t>(i)] = scaleToByte(std::stoi(tok), maxValue);
            }
        } else {
            for (int i = 0; i < width * height; ++i) {
                const std::string rt = readToken(input);
                const std::string gt = readToken(input);
                const std::string bt = readToken(input);
                if (rt.empty() || gt.empty() || bt.empty()) {
                    throw std::runtime_error("Unexpected end of P3 file");
                }
                const float r = scaleToByte(std::stoi(rt), maxValue);
                const float g = scaleToByte(std::stoi(gt), maxValue);
                const float b = scaleToByte(std::stoi(bt), maxValue);
                image.pixels[static_cast<std::size_t>(i)] = 0.299f * r + 0.587f * g + 0.114f * b;
            }
        }
        return image;
    }

    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (isGray) {
        for (int i = 0; i < width * height; ++i) {
            image.pixels[static_cast<std::size_t>(i)] = scaleToByte(readBinarySample(input, maxValue), maxValue);
        }
    } else {
        for (int i = 0; i < width * height; ++i) {
            const float r = scaleToByte(readBinarySample(input, maxValue), maxValue);
            const float g = scaleToByte(readBinarySample(input, maxValue), maxValue);
            const float b = scaleToByte(readBinarySample(input, maxValue), maxValue);
            image.pixels[static_cast<std::size_t>(i)] = 0.299f * r + 0.587f * g + 0.114f * b;
        }
    }

    return image;
}

void savePgm(const GrayImage& image, const std::string& path) {
    if (!image.isValid()) {
        throw std::runtime_error("Invalid image while writing PGM: " + path);
    }

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Failed to create PGM file: " + path);
    }

    output << "P5\n" << image.width << " " << image.height << "\n255\n";
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(image.width * image.height));
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = clampToByte(image.pixels[i]);
    }
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

} // namespace backend

