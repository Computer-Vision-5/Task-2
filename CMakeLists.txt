cmake_minimum_required(VERSION 3.20)
project(untitled4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optional Qt root. Pass with:
#   -DQT_ROOT_DIR="C:/msys64/ucrt64"
# or point directly to a Qt SDK root that contains lib/cmake/Qt6.
set(QT_ROOT_DIR "" CACHE PATH "Qt root directory")
if(QT_ROOT_DIR)
        list(PREPEND CMAKE_PREFIX_PATH "${QT_ROOT_DIR}")

        # Ensure Qt host tools (rcc/moc/uic) load matching runtime DLLs.
        if(WIN32)
                set(ENV{PATH} "${QT_ROOT_DIR}/bin;${QT_ROOT_DIR}/share/qt6/bin;$ENV{PATH}")
        endif()
endif()

option(ENABLE_QT_FRONTEND "Build Qt frontend" ON)

# Backend Library
add_library(imgproc_backend
        backend/src/Canny.cpp
        backend/src/Hough.cpp
        backend/src/Image.cpp
        backend/src/Snake.cpp
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

    find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

    add_executable(imgproc_qt
            frontend_qt/src/main.cpp
            frontend_qt/src/MainWindow.cpp
            frontend_qt/src/resources.qrc
            frontend_qt/src/MainWindow.hpp
    )

    target_link_libraries(imgproc_qt PRIVATE imgproc_backend Qt6::Widgets Qt6::Concurrent)
endif()