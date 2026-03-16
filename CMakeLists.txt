cmake_minimum_required(VERSION 3.20)
project(untitled4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 1. Point CMake to your specific Qt installation
set(CMAKE_PREFIX_PATH "C:/Qt/6.10.2/mingw_64")

option(ENABLE_QT_FRONTEND "Build Qt frontend" ON)

# Backend Library
add_library(imgproc_backend
        backend/src/Canny.cpp
        backend/src/Hough.cpp
        backend/src/ImageIO.cpp
)

target_include_directories(imgproc_backend
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/backend/include
)

# CLI Runner (Console App)
add_executable(imgproc_runner runner/src/main.cpp)
target_link_libraries(imgproc_runner PRIVATE imgproc_backend)

# GUI App (Qt)
if(ENABLE_QT_FRONTEND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    # Since you know you have Qt6, we can look for it directly
    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    add_executable(imgproc_qt
            frontend_qt/src/main.cpp
            frontend_qt/src/MainWindow.cpp
            frontend_qt/src/resources.qrc
            # Note: Usually you don't list .hpp in add_executable,
            # but it helps CLion "see" them in the project tree.
            frontend_qt/src/MainWindow.hpp
    )

    target_link_libraries(imgproc_qt PRIVATE imgproc_backend Qt6::Widgets)
endif()