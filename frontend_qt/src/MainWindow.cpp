#include "MainWindow.hpp"

#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

#include "Canny.hpp"
#include "Hough.hpp"
#include "ImageIO.hpp"

// ── Helper: builds a labeled slider block (name / value on one row, slider below) ──
static QWidget* makeSliderBlock(QWidget* parent,
                                 const QString& name,
                                 QSlider*& sliderOut,
                                 QLabel*&  valueOut,
                                 int min, int max, int initial,
                                 std::function<QString(int)> formatter = nullptr)
{
    auto* block  = new QWidget(parent);
    auto* layout = new QVBoxLayout(block);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(4);

    auto* row    = new QHBoxLayout();
    auto* lName  = new QLabel(name, block);
    auto* lValue = new QLabel(block);
    lName ->setObjectName("paramName");
    lValue->setObjectName("paramValue");
    lValue->setMinimumWidth(40);
    lValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    row->addWidget(lName);
    row->addStretch();
    row->addWidget(lValue);
    layout->addLayout(row);

    auto* slider = new QSlider(Qt::Horizontal, block);
    slider->setRange(min, max);
    slider->setValue(initial);
    layout->addWidget(slider);

    auto fmt = formatter ? formatter : [](int v){ return QString::number(v); };
    lValue->setText(fmt(initial));

    QObject::connect(slider, &QSlider::valueChanged, lValue, [lValue, fmt](int v){
        lValue->setText(fmt(v));
    });

    sliderOut = slider;
    valueOut  = lValue;
    return block;
}

static QFrame* makeSeparator(QWidget* parent)
{
    auto* sep = new QFrame(parent);
    sep->setObjectName("separator");
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Plain);
    sep->setFixedHeight(1);
    return sep;
}

static QLabel* makeSectionLabel(const QString& text, QWidget* parent)
{
    auto* lbl = new QLabel(text, parent);
    lbl->setObjectName("sectionLabel");
    return lbl;
}

// ─────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    QFile qssFile(":/styles.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        setStyleSheet(QString::fromUtf8(qssFile.readAll()));
        qssFile.close();
    } else {
        qssFile.setFileName("styles.qss");
        if (qssFile.open(QFile::ReadOnly)) {
            setStyleSheet(QString::fromUtf8(qssFile.readAll()));
            qssFile.close();
        }
    }

    controlsDock_ = new QDockWidget("Controls", this);
    controlsDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    controlsDock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    auto* scrollArea   = new QScrollArea(controlsDock_);
    auto* scrollWidget = new QWidget(scrollArea);
    auto* sideLayout   = new QVBoxLayout(scrollWidget);
    sideLayout->setContentsMargins(14, 14, 14, 14);
    sideLayout->setSpacing(6);

    sideLayout->addWidget(makeSectionLabel("Detection Mode", scrollWidget));
    detectionTypeBox_ = new QComboBox(scrollWidget);
    detectionTypeBox_->addItems({"All", "Edges", "Lines"});
    sideLayout->addWidget(detectionTypeBox_);

    sideLayout->addSpacing(8);
    sideLayout->addWidget(makeSeparator(scrollWidget));

    auto* cannyGroup  = new QGroupBox("Canny Edge Detection", scrollWidget);
    auto* cannyLayout = new QVBoxLayout(cannyGroup);
    cannyLayout->setSpacing(6);

    QLabel* dummy1 = nullptr; QLabel* dummy2 = nullptr;
    QLabel* dummy3 = nullptr; QLabel* dummy4 = nullptr;

    cannyLayout->addWidget(
        makeSliderBlock(cannyGroup, "Kernel Size", cannyKernelSizeSlider_, dummy1,
                        3, 21, 5,
                        [](int v){ return QString::number(v % 2 == 0 ? v + 1 : v); }));

    cannyLayout->addWidget(
        makeSliderBlock(cannyGroup, "Sigma", cannySigmaSlider_, dummy2,
                        1, 100, 14,
                        [](int v){ return QString::number(v / 10.0, 'f', 1); }));

    cannyLayout->addWidget(
        makeSliderBlock(cannyGroup, "Low Threshold", cannyLowThresholdSlider_, dummy3,
                        0, 1000, 50));

    cannyLayout->addWidget(
        makeSliderBlock(cannyGroup, "High Threshold", cannyHighThresholdSlider_, dummy4,
                        0, 1000, 100));

    sideLayout->addWidget(cannyGroup);
    sideLayout->addSpacing(4);

    houghLinesParamsWidget_ = new QGroupBox("Hough Lines", scrollWidget);
    auto* linesLayout = new QVBoxLayout(houghLinesParamsWidget_);
    linesLayout->setSpacing(6);
    QLabel* dummy5 = nullptr;
    linesLayout->addWidget(
        makeSliderBlock(static_cast<QGroupBox*>(houghLinesParamsWidget_),
                        "Threshold", houghLinesThresholdSlider_, dummy5,
                        1, 1000, 100));
    sideLayout->addWidget(houghLinesParamsWidget_);
    sideLayout->addSpacing(4);

    sideLayout->addStretch();
    sideLayout->addWidget(makeSeparator(scrollWidget));
    sideLayout->addSpacing(8);

    auto* runButton = new QPushButton("⚡  Process Image", scrollWidget);
    runButton->setObjectName("actionButton");
    sideLayout->addWidget(runButton);

    scrollWidget->setLayout(sideLayout);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setMinimumWidth(260);

    controlsDock_->setWidget(scrollArea);
    addDockWidget(Qt::LeftDockWidgetArea, controlsDock_);

    connect(detectionTypeBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onDetectionTypeChanged);
    onDetectionTypeChanged(0);

    auto* central    = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(16, 8, 16, 10);
    mainLayout->setSpacing(6);

    auto* title = new QLabel("Image Detection UI", central);
    title->setObjectName("titleLabel");
    mainLayout->addWidget(title);

    fileLabel_ = new QLabel("No image loaded — double-click a panel to open", central);
    fileLabel_->setObjectName("fileLabel");
    fileLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(fileLabel_);

    auto* imageLayout = new QHBoxLayout();
    imageLayout->setSpacing(10);

    inputLabel_ = new QLabel("Double-click to upload", central);
    edgesLabel_ = new QLabel("Output will appear here", central);
    inputLabel_->setObjectName("imagePanel");
    edgesLabel_->setObjectName("imagePanel");
    inputLabel_->setMinimumSize(440, 480);
    edgesLabel_->setMinimumSize(440, 480);
    inputLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    edgesLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    inputLabel_->setAlignment(Qt::AlignCenter);
    edgesLabel_->setAlignment(Qt::AlignCenter);
    imageLayout->addWidget(inputLabel_);
    imageLayout->addWidget(edgesLabel_);
    mainLayout->addLayout(imageLayout);

    detectionsBox_ = new QPlainTextEdit(central);
    detectionsBox_->setReadOnly(true);
    detectionsBox_->setPlaceholderText("Detection results will appear here after processing…");
    detectionsBox_->setObjectName("reportBox");
    detectionsBox_->setMaximumHeight(160);
    mainLayout->addWidget(detectionsBox_);

    setCentralWidget(central);
    setWindowTitle("Image Detection UI");
    resize(1100, 780);

    connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunPipeline);
}

// ─────────────────────────────────────────────────────────────
QImage MainWindow::toQImage(const backend::GrayImage& image)
{
    QImage out(image.width, image.height, QImage::Format_Grayscale8);
    for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x) {
            const int v = static_cast<int>(std::clamp(image.at(x, y), 0.0f, 255.0f));
            out.setPixel(x, y, qRgb(v, v, v));
        }
    return out;
}

void MainWindow::setStatusText(const QString& text)
{
    detectionsBox_->setPlainText(text);
}

// ─────────────────────────────────────────────────────────────
void MainWindow::onLoadImage()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Open image", {},
        "Images (*.png *.jpg *.jpeg *.pgm *.ppm)");
    if (path.isEmpty()) return;

    QImage qImage;
    if (!qImage.load(path)) {
        QMessageBox::critical(this, "Load failed", "Failed to load image file.");
        return;
    }

    image_ = backend::GrayImage(qImage.width(), qImage.height());
    for (int y = 0; y < qImage.height(); ++y)
        for (int x = 0; x < qImage.width(); ++x)
            image_.at(x, y) = qGray(qImage.pixel(x, y));

    const QPixmap pix = QPixmap::fromImage(qImage)
        .scaled(inputLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    inputLabel_->setPixmap(pix);
    edgesLabel_->setText("Output will appear here");
    fileLabel_->setText("Loaded: " + path);
    setStatusText("Image loaded. Press 'Process Image' to run the pipeline.");
}

// ─────────────────────────────────────────────────────────────
// Extracts true line segments from a Hough-detected infinite line.
//
// Algorithm:
//   1. Parameterise the infinite line by arc-length t along its direction
//      vector (perpDir rotated 90°).  Every edge pixel close to the line
//      gets projected onto that 1-D axis → a scalar t value.
//   2. Sort those t-values and scan for *contiguous runs* (gap ≤ maxGap).
//   3. Only keep runs whose projected length ≥ minSegmentLen pixels.
//   4. Convert each run's (t_start, t_end) back to image (x,y) coordinates.
//
// This guarantees that each drawn segment corresponds exactly to where edge
// pixels actually exist — no "bridging" across large blank regions.
// ─────────────────────────────────────────────────────────────
static std::vector<std::pair<QPoint, QPoint>> extractLineSegments(
    const backend::GrayImage& edges,
    double rho,
    double theta,
    double distTolerance = 1.5,  // pixels away from ideal line
    double maxGap        = 8.0,  // max blank gap still considered same segment
    double minSegmentLen = 15.0) // minimum segment length to draw
{
    // Direction along the line (unit vector tangent to it)
    const double dx =  std::sin(theta);   // perpendicular to normal → tangent
    const double dy = -std::cos(theta);

    // A point on the ideal infinite line
    const double ox = rho * std::cos(theta);
    const double oy = rho * std::sin(theta);

    // Collect the 1-D projection (t) of every qualifying edge pixel
    std::vector<double> tValues;
    tValues.reserve(512);

    for (int y = 0; y < edges.height; ++y) {
        for (int x = 0; x < edges.width; ++x) {
            if (edges.at(x, y) < 128.0f) continue;   // not an edge pixel

            // Signed distance from pixel to the ideal line: n·p - rho
            const double dist = std::cos(theta) * x + std::sin(theta) * y - rho;
            if (std::abs(dist) > distTolerance) continue;

            // Project onto tangent to get arc-length position
            const double t = dx * (x - ox) + dy * (y - oy);
            tValues.push_back(t);
        }
    }

    if (tValues.empty())
        return {};

    std::sort(tValues.begin(), tValues.end());

    // Scan for contiguous runs separated by at most maxGap
    std::vector<std::pair<QPoint, QPoint>> segments;
    double segStart = tValues.front();
    double segEnd   = tValues.front();

    auto tToPoint = [&](double t) -> QPoint {
        return QPoint(
            static_cast<int>(std::round(ox + t * dx)),
            static_cast<int>(std::round(oy + t * dy)));
    };

    for (std::size_t i = 1; i < tValues.size(); ++i) {
        const double gap = tValues[i] - tValues[i - 1];
        if (gap <= maxGap) {
            // Extend current segment
            segEnd = tValues[i];
        } else {
            // Gap too large → close current segment, start a new one
            if (segEnd - segStart >= minSegmentLen)
                segments.emplace_back(tToPoint(segStart), tToPoint(segEnd));
            segStart = tValues[i];
            segEnd   = tValues[i];
        }
    }
    // Don't forget the last run
    if (segEnd - segStart >= minSegmentLen)
        segments.emplace_back(tToPoint(segStart), tToPoint(segEnd));

    return segments;
}

// ─────────────────────────────────────────────────────────────
void MainWindow::onRunPipeline()
{
    if (!image_.isValid()) {
        QMessageBox::information(this, "No image", "Please upload an image first.");
        return;
    }

    try {
        // ── Canny ──────────────────────────────────────────────
        backend::CannyParams cannyParams;
        int kernelSize = cannyKernelSizeSlider_->value();
        if (kernelSize % 2 == 0) ++kernelSize;          // must be odd
        cannyParams.gaussianKernelSize = kernelSize;
        cannyParams.gaussianSigma      = cannySigmaSlider_->value() / 10.0;
        cannyParams.lowThreshold       = cannyLowThresholdSlider_->value();
        cannyParams.highThreshold      = cannyHighThresholdSlider_->value();

        const backend::GrayImage edges = backend::cannyEdgeDetect(image_, cannyParams);

        const QString detectionType = detectionTypeBox_->currentText();

        // ── Edges-only mode ────────────────────────────────────
        if (detectionType == "Edges") {
            edgesLabel_->setPixmap(
                QPixmap::fromImage(toQImage(edges))
                    .scaled(edgesLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            setStatusText("Canny edge detection complete.");
            return;
        }

        // ── Hough Lines ────────────────────────────────────────
        const auto lines = backend::detectLinesHough(
            edges,
            houghLinesThresholdSlider_->value(),
            /*maxLines=*/20);

        // Start from a colour copy of the original image
        QImage resultImage = toQImage(image_).convertToFormat(QImage::Format_ARGB32);
        QPainter painter(&resultImage);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(255, 50, 50), 2));

        std::ostringstream report;
        report << "Detection Summary\n-----------------\n";
        report << "Lines found: " << lines.size() << "\n\n";

        int totalSegments = 0;

        if (detectionType == "All" || detectionType == "Lines") {
            for (std::size_t i = 0; i < lines.size(); ++i) {
                const double rho   = lines[i].rho;
                const double theta = lines[i].theta;

                report << "  L" << (i + 1)
                       << ": rho=" << static_cast<int>(rho)
                       << "  theta=" << static_cast<int>(theta * 180.0 / M_PI) << "°"
                       << "  votes=" << lines[i].votes << "\n";

                // ── Key fix: extract actual segments, not the infinite line ──
                const auto segments = extractLineSegments(edges, rho, theta);
                totalSegments += static_cast<int>(segments.size());

                for (const auto& [p1, p2] : segments)
                    painter.drawLine(p1, p2);
            }
        }

        report << "\nTotal drawn segments: " << totalSegments;

        edgesLabel_->setPixmap(
            QPixmap::fromImage(resultImage)
                .scaled(edgesLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        setStatusText(QString::fromStdString(report.str()));

    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Processing failed", ex.what());
    }
}

// ─────────────────────────────────────────────────────────────
void MainWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && inputLabel_->underMouse())
        onLoadImage();
}

void MainWindow::onDetectionTypeChanged(int index)
{
    const QString t = detectionTypeBox_->itemText(index);
    houghLinesParamsWidget_->setVisible(t == "All" || t == "Lines");
}



