#pragma once

#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QDockWidget>
#include <QSlider>
#include <QLabel>

#include "Image.hpp"

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onLoadImage();
    void onRunPipeline();
    void onDetectionTypeChanged(int index);

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    static QImage toQImage(const backend::GrayImage& image);
    void setStatusText(const QString& text);

    backend::GrayImage image_;
    QLabel* inputLabel_ = nullptr;
    QLabel* edgesLabel_ = nullptr;
    QLabel* fileLabel_ = nullptr;
    QPlainTextEdit* detectionsBox_ = nullptr;
    QComboBox* detectionTypeBox_ = nullptr;
    QDockWidget* controlsDock_ = nullptr;
    QSlider* cannyKernelSizeSlider_ = nullptr;
    QSlider* cannySigmaSlider_ = nullptr;
    QSlider* cannyLowThresholdSlider_ = nullptr;
    QSlider* cannyHighThresholdSlider_ = nullptr;
    QSlider* houghLinesThresholdSlider_ = nullptr;
    QWidget* houghLinesParamsWidget_ = nullptr;
};
