#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Canny.hpp"
#include "Hough.hpp"
#include "Image.hpp"

namespace {

void printUsage() {
    std::cout << "Usage:\n"
              << "  imgproc_runner <input.pgm> <output_prefix> [options]\n\n"
              << "Options:\n"
              << "  --low <value>        Canny low threshold (default: 30)\n"
              << "  --high <value>       Canny high threshold (default: 80)\n"
              << "  --line-th <votes>    Hough line vote threshold (default: 100)\n";
}

void saveDetections(
    const std::string& path,
    const std::vector<backend::LineDetection>& lines) {

    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Failed to create detections file: " + path);
    }

    out << "LINES " << lines.size() << "\n";
    for (const auto& line : lines) {
        out << line.rho << " " << line.theta << " " << line.votes << "\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        printUsage();
        return 1;
    }

    const std::string inputPath = argv[1];
    const std::string outputPrefix = argv[2];

    backend::CannyParams cannyParams;
    int lineThreshold = 100;

    // Parse optional arguments
    for (int i = 3; i < argc; i += 2) {
        if (i + 1 >= argc) {
            throw std::runtime_error("Missing value for argument: " + std::string(argv[i]));
        }

        const std::string key = argv[i];
        const std::string value = argv[i + 1];

        if (key == "--low") {
            cannyParams.lowThreshold = std::stod(value);
        } else if (key == "--high") {
            cannyParams.highThreshold = std::stod(value);
        } else if (key == "--line-th") {
            lineThreshold = std::stoi(value);
        } else {
            throw std::runtime_error("Unknown argument: " + key);
        }
    }

    try {
        // Load image
        const backend::GrayImage image = backend::loadImageAsGray(inputPath);

        // Canny edge detection
        const backend::GrayImage edges = backend::cannyEdgeDetect(image, cannyParams);

        // Hough line detection
        const std::vector<backend::LineDetection> lines =
            backend::detectLinesHough(edges, lineThreshold, 20);

        // Save outputs
        backend::savePgm(edges, outputPrefix + "_edges.pgm");
        saveDetections(outputPrefix + "_detections.txt", lines);

        std::cout << "Saved: " << outputPrefix << "_edges.pgm\n";
        std::cout << "Saved: " << outputPrefix << "_detections.txt\n";
        std::cout << "Detected lines: " << lines.size() << "\n";

        return 0;

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 2;
    }
}