#include "MainWindow.hpp"

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <sstream>

#include "Canny.hpp"
#include "Hough.hpp"
#include "ImageIO.hpp"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);
    auto* controlsLayout = new QHBoxLayout();
    auto* buttonLayout = new QHBoxLayout();
    auto* imageLayout = new QHBoxLayout();

    auto* title = new QLabel("Image Detection UI", central);
    title->setObjectName("titleLabel");

    auto* runButton = new QPushButton("Detect Edges / Lines / Circles", central);
    runButton->setObjectName("actionButton");

    fileLabel_ = new QLabel("No image loaded", central);
    fileLabel_->setObjectName("fileLabel");

    buttonLayout->addWidget(runButton);

    detectionTypeBox_ = new QComboBox(central);
    detectionTypeBox_->addItems({"All", "Edges", "Lines", "Circles"});

    auto* paramsGroup = new QGroupBox("Detection Parameters", central);
    auto* paramsLayout = new QFormLayout(paramsGroup);

    cannyKernelSizeSpin_ = new QSpinBox(central);
    cannyKernelSizeSpin_->setRange(3, 21);
    cannyKernelSizeSpin_->setSingleStep(2);
    cannyKernelSizeSpin_->setValue(5);
    paramsLayout->addRow("Canny Kernel Size:", cannyKernelSizeSpin_);

    cannySigmaSpin_ = new QDoubleSpinBox(central);
    cannySigmaSpin_->setRange(0.1, 10.0);
    cannySigmaSpin_->setSingleStep(0.1);
    cannySigmaSpin_->setValue(1.4);
    paramsLayout->addRow("Canny Sigma:", cannySigmaSpin_);

    cannyLowThresholdSpin_ = new QDoubleSpinBox(central);
    cannyLowThresholdSpin_->setRange(0.0, 1000.0);
    cannyLowThresholdSpin_->setValue(50.0);
    paramsLayout->addRow("Canny Low Threshold:", cannyLowThresholdSpin_);

    cannyHighThresholdSpin_ = new QDoubleSpinBox(central);
    cannyHighThresholdSpin_->setRange(0.0, 1000.0);
    cannyHighThresholdSpin_->setValue(100.0);
    paramsLayout->addRow("Canny High Threshold:", cannyHighThresholdSpin_);

    houghLinesThresholdSpin_ = new QSpinBox(central);
    houghLinesThresholdSpin_->setRange(1, 1000);
    houghLinesThresholdSpin_->setValue(100);
    paramsLayout->addRow("Hough Lines Threshold:", houghLinesThresholdSpin_);

    houghCirclesThresholdSpin_ = new QSpinBox(central);
    houghCirclesThresholdSpin_->setRange(1, 1000);
    houghCirclesThresholdSpin_->setValue(20);
    paramsLayout->addRow("Hough Circles Threshold:", houghCirclesThresholdSpin_);

    controlsLayout->addWidget(detectionTypeBox_);
    controlsLayout->addWidget(paramsGroup);

    inputLabel_ = new QLabel("click or drop to upload", central);
    edgesLabel_ = new QLabel("Edge output", central);
    inputLabel_->setObjectName("imagePanel");
    edgesLabel_->setObjectName("imagePanel");
    inputLabel_->setMinimumSize(420, 320);
    edgesLabel_->setMinimumSize(420, 320);
    inputLabel_->setAlignment(Qt::AlignCenter);
    edgesLabel_->setAlignment(Qt::AlignCenter);
    imageLayout->addWidget(inputLabel_);
    imageLayout->addWidget(edgesLabel_);

    detectionsBox_ = new QPlainTextEdit(central);
    detectionsBox_->setReadOnly(true);
    detectionsBox_->setPlaceholderText("Detection results will appear here.");
    detectionsBox_->setObjectName("reportBox");

    mainLayout->addWidget(title);
    mainLayout->addWidget(fileLabel_);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(controlsLayout);
    mainLayout->addLayout(imageLayout);
    mainLayout->addWidget(detectionsBox_);
    setCentralWidget(central);

    setWindowTitle("Qt Frontend + C++ Backend");
    resize(1000, 760);

    setStyleSheet(
        "QWidget { background-color: #f2f5f7; color: #2d3b45; }"
        "QLabel#titleLabel { font-size: 22px; font-weight: 600; color: #3f6274; padding: 6px 2px; }"
        "QLabel#fileLabel { background-color: #e8eef2; border: 1px solid #d1dde5; border-radius: 8px; padding: 8px 10px; }"
        "QLabel#imagePanel { background-color: #fcfdfe; border: 1px solid #cfdae3; border-radius: 10px; }"
        "QPushButton#actionButton { background-color: #d9e6ef; border: 1px solid #c2d3df; border-radius: 8px; padding: 8px 14px; }"
        "QPushButton#actionButton:hover { background-color: #cadde9; }"
        "QPlainTextEdit#reportBox { background-color: #fcfdfe; border: 1px solid #cfdae3; border-radius: 8px; padding: 8px; }"
        "QGroupBox { border: 1px solid #cfdae3; border-radius: 8px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 3px; }"
    );

    connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunPipeline);
}

QImage MainWindow::toQImage(const backend::GrayImage& image) {
    QImage out(image.width, image.height, QImage::Format_Grayscale8);
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const int value = static_cast<int>(std::clamp(image.at(x, y), 0.0f, 255.0f));
            out.setPixel(x, y, qRgb(value, value, value));
        }
    }
    return out;
}

void MainWindow::setStatusText(const QString& text) {
    detectionsBox_->setPlainText(text);
}

void MainWindow::onLoadImage() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Open image",
        QString(),
        "Images (*.png *.jpg *.jpeg *.pgm *.ppm)");

    if (path.isEmpty()) {
        return;
    }

    QImage qImage;
    if (!qImage.load(path)) {
        QMessageBox::critical(this, "Load failed", "Failed to load image file.");
        return;
    }

    image_ = backend::GrayImage(qImage.width(), qImage.height());
    for (int y = 0; y < qImage.height(); ++y) {
        for (int x = 0; x < qImage.width(); ++x) {
            image_.at(x, y) = qGray(qImage.pixel(x, y));
        }
    }

    const QPixmap pix = QPixmap::fromImage(qImage).scaled(inputLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    inputLabel_->setPixmap(pix);
    edgesLabel_->setText("Edge output");
    fileLabel_->setText("Loaded: " + path);
    setStatusText("Image loaded successfully. Press 'Detect Edges / Lines / Circles'.");
}

void MainWindow::onRunPipeline() {
    if (!image_.isValid()) {
        QMessageBox::information(this, "No image", "Please upload an image first.");
        return;
    }

    try {
        backend::CannyParams cannyParams;
        cannyParams.gaussianKernelSize = cannyKernelSizeSpin_->value();
        cannyParams.gaussianSigma = cannySigmaSpin_->value();
        cannyParams.lowThreshold = cannyLowThresholdSpin_->value();
        cannyParams.highThreshold = cannyHighThresholdSpin_->value();

        const backend::GrayImage edges = backend::cannyEdgeDetect(image_, cannyParams);

        const QString detectionType = detectionTypeBox_->currentText();
        if (detectionType == "Edges") {
            const QPixmap edgesPix = QPixmap::fromImage(toQImage(edges)).scaled(edgesLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            edgesLabel_->setPixmap(edgesPix);
            setStatusText("Canny edge detection complete.");
            return;
        }

        const auto lines = backend::detectLinesHough(edges, houghLinesThresholdSpin_->value(), 20);
        const auto circles = backend::detectCirclesHough(edges, 10, 80, 80, houghCirclesThresholdSpin_->value());

        QImage resultImage = toQImage(image_);
        resultImage = resultImage.convertToFormat(QImage::Format_ARGB32);
        QPainter painter(&resultImage);
        painter.setRenderHint(QPainter::Antialiasing);

        std::ostringstream report;
        report << "Detection Summary\n";
        report << "-----------------\n";

        if (detectionType == "All" || detectionType == "Lines") {
            report << "Lines: " << lines.size() << "\n";
            painter.setPen(QPen(Qt::red, 2));
            for (std::size_t i = 0; i < lines.size(); ++i) {
                report << "  L" << i + 1 << ": rho=" << lines[i].rho << ", theta=" << lines[i].theta << ", votes=" << lines[i].votes << "\n";
                const double rho = lines[i].rho;
                const double theta = lines[i].theta;
                const double a = std::cos(theta);
                const double b = std::sin(theta);
                const double x0 = a * rho;
                const double y0 = b * rho;
                const int x1 = static_cast<int>(x0 + 1000 * (-b));
                const int y1 = static_cast<int>(y0 + 1000 * (a));
                const int x2 = static_cast<int>(x0 - 1000 * (-b));
                const int y2 = static_cast<int>(y0 - 1000 * (a));
                painter.drawLine(x1, y1, x2, y2);
            }
        }

        if (detectionType == "All" || detectionType == "Circles") {
            report << "\nCircles: " << circles.size() << "\n";
            painter.setPen(QPen(Qt::blue, 2));
            for (std::size_t i = 0; i < circles.size(); ++i) {
                report << "  C" << i + 1 << ": cx=" << circles[i].centerX << ", cy=" << circles[i].centerY
                       << ", r=" << circles[i].radius << ", votes=" << circles[i].votes << "\n";
                painter.drawEllipse(QPoint(circles[i].centerX, circles[i].centerY), circles[i].radius, circles[i].radius);
            }
        }

        const QPixmap resultPix = QPixmap::fromImage(resultImage).scaled(edgesLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        edgesLabel_->setPixmap(resultPix);
        setStatusText(QString::fromStdString(report.str()));

    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Processing failed", ex.what());
    }
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && inputLabel_->underMouse()) {
        onLoadImage();
    }
}
