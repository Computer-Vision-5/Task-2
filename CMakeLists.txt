cmake_minimum_required(VERSION 3.20)
project(untitled4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# 1. Point CMake to your MSYS2 MinGW64 Qt installation
set(CMAKE_PREFIX_PATH "C:/msys64/mingw64")

option(ENABLE_QT_FRONTEND "Build Qt frontend" ON)

# Backend Library
add_library(imgproc_backend
        backend/src/Canny.cpp
        backend/src/Hough.cpp
)

target_include_directories(imgproc_backend
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/backend/include
)

# CLI Runner (Commented out for now so it doesn't block the UI build)
# add_executable(imgproc_runner runner/src/main.cpp)
# target_link_libraries(imgproc_runner PRIVATE imgproc_backend)

# GUI App (Qt)
if(ENABLE_QT_FRONTEND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    add_executable(imgproc_qt
            frontend_qt/src/main.cpp
            frontend_qt/src/MainWindow.cpp
            frontend_qt/src/resources.qrc
            frontend_qt/src/MainWindow.hpp
    )

    target_link_libraries(imgproc_qt PRIVATE imgproc_backend Qt6::Widgets)
endif()