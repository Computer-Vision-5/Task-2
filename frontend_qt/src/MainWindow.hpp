#pragma once

#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>

#include "Image.hpp"

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onLoadImage();
    void onRunPipeline();

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
    QSpinBox* cannyKernelSizeSpin_ = nullptr;
    QDoubleSpinBox* cannySigmaSpin_ = nullptr;
    QDoubleSpinBox* cannyLowThresholdSpin_ = nullptr;
    QDoubleSpinBox* cannyHighThresholdSpin_ = nullptr;
    QSpinBox* houghLinesThresholdSpin_ = nullptr;
    QSpinBox* houghCirclesThresholdSpin_ = nullptr;
};
