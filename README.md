# Image Processing Project

This project is a C++ application with a Qt frontend that performs image processing tasks, including Canny edge detection and Hough transforms for line and circle detection.

## Features

- Load images in various formats (PNG, JPG, JPEG, PGM, PPM).
- Double-click on the image preview to load a new image.
- Canny edge detection with adjustable parameters.
- Hough transform for line and circle detection with adjustable parameters.
- Superimpose detected lines and circles on the original image.
- Selectable detection type (All, Edges, Lines, Circles).

## Building the Project

### Prerequisites

- C++ compiler (supporting C++20)
- CMake (version 3.16 or later)
- Qt (version 6 or later)

### Build Steps

1.  **Clone the repository:**
    ```bash
    git clone <repository-url>
    cd <repository-directory>
    ```

2.  **Configure the project with CMake:**
    ```bash
    cmake -B build -S .
    ```

3.  **Build the project:**
    ```bash
    cmake --build build
    ```

4.  **Run the application:**
    The executable `imgproc_qt` will be located in the `build` directory (or a subdirectory depending on the generator).

## Usage

1.  Launch the `imgproc_qt` application.
2.  Double-click the "click or drop to upload" area to load an image.
3.  Adjust the detection parameters in the "Detection Parameters" section.
4.  Select the desired detection type from the dropdown menu.
5.  Click the "Detect Edges / Lines / Circles" button to run the pipeline.
6.  The results will be displayed in the right-hand panel, and a summary will be shown in the text box below.

