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
              << "  imgproc_runner <input.(pgm|ppm)> <output_prefix> [options]\n\n"
              << "Options:\n"
              << "  --low <value>          Canny low threshold (default: 30)\n"
              << "  --high <value>         Canny high threshold (default: 80)\n"
              << "  --line-th <votes>      Hough line vote threshold (default: 100)\n"
              << "  --circle-th <votes>    Hough circle vote threshold (default: 80)\n"
              << "  --min-r <radius>       Min circle radius (default: 10)\n"
              << "  --max-r <radius>       Max circle radius (default: 80)\n"
              << "  --ellipse-th <votes>   Hough ellipse vote threshold (default: 60)\n"
              << "  --min-a <axis>         Min ellipse semi-major axis (default: 15)\n"
              << "  --max-a <axis>         Max ellipse semi-major axis (default: 60)\n"
              << "  --min-b <axis>         Min ellipse semi-minor axis (default: 15)\n"
              << "  --max-b <axis>         Max ellipse semi-minor axis (default: 60)\n";
}

void saveDetections(
    const std::string& path,
    const std::vector<backend::LineDetection>& lines,
    const std::vector<backend::CircleDetection>& circles,
    const std::vector<backend::EllipseDetection>& ellipses) {
    
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Failed to create detections file: " + path);
    }

    out << "LINES " << lines.size() << "\n";
    for (const auto& line : lines) {
        out << line.rho << " " << line.theta << " " << line.votes << "\n";
    }

    out << "CIRCLES " << circles.size() << "\n";
    for (const auto& circle : circles) {
        out << circle.centerX << " " << circle.centerY << " " << circle.radius << " " << circle.votes << "\n";
    }

    out << "ELLIPSES " << ellipses.size() << "\n";
    for (const auto& ellipse : ellipses) {
        out << ellipse.centerX << " " << ellipse.centerY << " " 
            << ellipse.a << " " << ellipse.b << " " << ellipse.angle << " " << ellipse.votes << "\n";
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
    int circleThreshold = 80;
    int minRadius = 10;
    int maxRadius = 80;
    int ellipseThreshold = 60;
    int minA = 15;
    int maxA = 60;
    int minB = 15;
    int maxB = 60;

    for (int i = 3; i < argc; i += 2) {
        if (i + 1 >= argc) {
            throw std::runtime_error("Missing value for argument: " + std::string(argv[i]));
        }
        const std::string key = argv[i];
        const std::string value = argv[i + 1];

        if (key == "--low") cannyParams.lowThreshold = std::stod(value);
        else if (key == "--high") cannyParams.highThreshold = std::stod(value);
        else if (key == "--line-th") lineThreshold = std::stoi(value);
        else if (key == "--circle-th") circleThreshold = std::stoi(value);
        else if (key == "--min-r") minRadius = std::stoi(value);
        else if (key == "--max-r") maxRadius = std::stoi(value);
        else if (key == "--ellipse-th") ellipseThreshold = std::stoi(value);
        else if (key == "--min-a") minA = std::stoi(value);
        else if (key == "--max-a") maxA = std::stoi(value);
        else if (key == "--min-b") minB = std::stoi(value);
        else if (key == "--max-b") maxB = std::stoi(value);
        else throw std::runtime_error("Unknown argument: " + key);
    }

    try {
        // Run the processing pipeline
        const backend::GrayImage image = backend::loadImageAsGray(inputPath);
        const backend::GrayImage edges = backend::cannyEdgeDetect(image, cannyParams);
        
        // Execute Hough Transforms
        const auto lines = backend::detectLinesHough(edges, lineThreshold, 20);
        const auto circles = backend::detectCirclesHough(edges, minRadius, maxRadius, circleThreshold, 20);
        const auto ellipses = backend::detectEllipsesHough(edges, minA, maxA, minB, maxB, ellipseThreshold, 20);

        // Save output files
        backend::savePgm(edges, outputPrefix + "_edges.pgm");
        saveDetections(outputPrefix + "_detections.txt", lines, circles, ellipses);

        // Print summary to console
        std::cout << "Saved: " << outputPrefix << "_edges.pgm\n";
        std::cout << "Saved: " << outputPrefix << "_detections.txt\n";
        std::cout << "Detected lines: " << lines.size() << "\n";
        std::cout << "Detected circles: " << circles.size() << "\n";
        std::cout << "Detected ellipses: " << ellipses.size() << "\n";
        
        return 0;
        
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 2;
    }
}