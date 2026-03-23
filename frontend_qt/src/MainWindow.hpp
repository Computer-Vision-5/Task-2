#pragma once

#include <QFutureWatcher>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QDockWidget>
#include <QPushButton>
#include <QSlider>
#include <QPointF>

#include "Image.hpp"
#include "Snake.hpp"

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private slots:
    void onLoadImage();
    void onRunPipeline();
    void onDetectionTypeChanged(int index);
    void onProcessingFinished();

private:
    static QImage toQImage(const backend::GrayImage& image);
    void setStatusText(const QString& text);

    // Core image state
    backend::GrayImage image_;
    backend::GrayImage gradientMag_;          ///< Sobel magnitude -- reused by snake

    // Snake runtime state
    backend::SnakeContour snakeContour_;      ///< Latest evolved contour
    QPointF snakeCenter_;                     ///< Click position in image coordinates
    float   snakeRadius_  = 0.0f;            ///< Drag radius in image pixels
    QPointF snakePressPos_;                   ///< Raw widget position at mouse-press
    bool    snakePlacing_ = false;            ///< True while user is dragging a circle
    QPixmap snakeOverlayPixmap_;              ///< Clean copy of input pixmap for overlay

    // Pipeline result -- carries all computed output across the thread boundary
    // so the worker thread never touches Qt widgets directly.
    struct PipelineResult {
        QImage  outputImage;
        QString statusText;
        backend::SnakeContour snakeContour;   ///< filled only in Snake mode
        // ── Snake analysis results (Parts C / D / E) ──────────────────
        std::vector<int> chainCodes;          ///< Freeman 8-direction codes
        float            contourArea      = 0.0f;  ///< px²
        float            contourPerimeter = 0.0f;  ///< px
    };

    // Main UI widgets
    QLabel*         inputLabel_       = nullptr;
    QLabel*         edgesLabel_       = nullptr;
    QLabel*         fileLabel_        = nullptr;
    QPlainTextEdit* detectionsBox_    = nullptr;
    QComboBox*      detectionTypeBox_ = nullptr;
    QDockWidget*    controlsDock_     = nullptr;
    QPushButton*    processButton_    = nullptr;
    QFutureWatcher<PipelineResult>* watcher_ = nullptr;

    // Canny sliders
    QSlider* cannyKernelSizeSlider_    = nullptr;
    QSlider* cannySigmaSlider_         = nullptr;
    QSlider* cannyLowThresholdSlider_  = nullptr;
    QSlider* cannyHighThresholdSlider_ = nullptr;

    // Hough Lines params
    QWidget* houghLinesParamsWidget_    = nullptr;
    QSlider* houghLinesThresholdSlider_ = nullptr;

    // Snake params
    QWidget* snakeParamsWidget_    = nullptr;
    QSlider* snakeAlphaSlider_     = nullptr;
    QSlider* snakeBetaSlider_      = nullptr;
    QSlider* snakeWindowSlider_    = nullptr;
    QSlider* snakeNumPointsSlider_ = nullptr;
    QSlider* snakeMaxIterSlider_   = nullptr;
    QSlider* snakeThresholdSlider_ = nullptr;

    // Hough Circles params
    QWidget* houghCirclesParamsWidget_  = nullptr;
    QSlider* houghCirclesThresholdSlider_ = nullptr;
    QSlider* houghCirclesMinRSlider_    = nullptr;
    QSlider* houghCirclesMaxRSlider_    = nullptr;

    // Hough Ellipses params
    QWidget* houghEllipsesParamsWidget_ = nullptr;
    QSlider* houghEllipsesThresholdSlider_ = nullptr;
    QSlider* houghEllipsesMinASlider_   = nullptr;
    QSlider* houghEllipsesMaxASlider_   = nullptr;
};
