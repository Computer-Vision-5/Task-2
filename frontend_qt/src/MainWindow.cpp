// #include "MainWindow.hpp"
//
// #include <QFile>
// #include <QFileDialog>
// #include <QFrame>
// #include <QGroupBox>
// #include <QHBoxLayout>
// #include <QMessageBox>
// #include <QMouseEvent>
// #include <QPainter>
// #include <QPushButton>
// #include <QScrollArea>
// #include <QVBoxLayout>
// #include <QWidget>
//
// #include <algorithm>
// #include <sstream>
//
// #include "Canny.hpp"
// #include "Hough.hpp"
// #include "ImageIO.hpp"
//
// // ── Helper: builds a labeled slider block (name / value on one row, slider below) ──
// static QWidget* makeSliderBlock(QWidget* parent,
//                                  const QString& name,
//                                  QSlider*& sliderOut,
//                                  QLabel*&  valueOut,
//                                  int min, int max, int initial,
//                                  std::function<QString(int)> formatter = nullptr)
// {
//     auto* block  = new QWidget(parent);
//     auto* layout = new QVBoxLayout(block);
//     layout->setContentsMargins(0, 4, 0, 4);
//     layout->setSpacing(4);
//
//     // Row: param name  +  current value (right-aligned)
//     auto* row    = new QHBoxLayout();
//     auto* lName  = new QLabel(name, block);
//     auto* lValue = new QLabel(block);
//     lName ->setObjectName("paramName");
//     lValue->setObjectName("paramValue");
//     lValue->setMinimumWidth(40);
//     lValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//     row->addWidget(lName);
//     row->addStretch();
//     row->addWidget(lValue);
//     layout->addLayout(row);
//
//     // Slider below
//     auto* slider = new QSlider(Qt::Horizontal, block);
//     slider->setRange(min, max);
//     slider->setValue(initial);
//     layout->addWidget(slider);
//
//     // Set initial value text
//     auto fmt = formatter ? formatter : [](int v){ return QString::number(v); };
//     lValue->setText(fmt(initial));
//
//     // Connect
//     QObject::connect(slider, &QSlider::valueChanged, lValue, [lValue, fmt](int v){
//         lValue->setText(fmt(v));
//     });
//
//     sliderOut = slider;
//     valueOut  = lValue;
//     return block;
// }
//
// // ── Helper: thin horizontal separator ──
// static QFrame* makeSeparator(QWidget* parent)
// {
//     auto* sep = new QFrame(parent);
//     sep->setObjectName("separator");
//     sep->setFrameShape(QFrame::HLine);
//     sep->setFrameShadow(QFrame::Plain);
//     sep->setFixedHeight(1);
//     return sep;
// }
//
// // ── Helper: section header label ──
// static QLabel* makeSectionLabel(const QString& text, QWidget* parent)
// {
//     auto* lbl = new QLabel(text, parent);
//     lbl->setObjectName("sectionLabel");
//     return lbl;
// }
//
// // ─────────────────────────────────────────────────────────────
// MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
// {
//     // ── Load stylesheet from external file ──
//     QFile qssFile(":/styles.qss");          // resource path if using .qrc
//     if (!qssFile.open(QFile::ReadOnly)) {
//         qssFile.setFileName("styles.qss");  // fallback: same directory as binary
//         qssFile.open(QFile::ReadOnly);
//     }
//     if (qssFile.isOpen()) {
//         setStyleSheet(QString::fromUtf8(qssFile.readAll()));
//     }
//
//     // ═══════════════════════════════════════
//     //  SIDEBAR / DOCK
//     // ═══════════════════════════════════════
//     controlsDock_ = new QDockWidget("Controls", this);
//     controlsDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
//     controlsDock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
//
//     // Scrollable inner container so it works at any window height
//     auto* scrollArea   = new QScrollArea(controlsDock_);
//     auto* scrollWidget = new QWidget(scrollArea);
//     auto* sideLayout   = new QVBoxLayout(scrollWidget);
//     sideLayout->setContentsMargins(14, 14, 14, 14);
//     sideLayout->setSpacing(6);
//
//     // ── Detection type ──
//     sideLayout->addWidget(makeSectionLabel("Detection Mode", scrollWidget));
//     detectionTypeBox_ = new QComboBox(scrollWidget);
//     detectionTypeBox_->addItems({"All", "Edges", "Lines", "Circles"});
//     sideLayout->addWidget(detectionTypeBox_);
//
//     sideLayout->addSpacing(8);
//     sideLayout->addWidget(makeSeparator(scrollWidget));
//
//     // ── Canny group ──
//     auto* cannyGroup  = new QGroupBox("Canny Edge Detection", scrollWidget);
//     auto* cannyLayout = new QVBoxLayout(cannyGroup);
//     cannyLayout->setSpacing(6);
//
//     QLabel* dummy1 = nullptr; QLabel* dummy2 = nullptr;
//     QLabel* dummy3 = nullptr; QLabel* dummy4 = nullptr;
//
//     cannyLayout->addWidget(
//         makeSliderBlock(cannyGroup, "Kernel Size", cannyKernelSizeSlider_, dummy1,
//                         3, 21, 5,
//                         [](int v){ return QString::number(v % 2 == 0 ? v + 1 : v); }));
//
//     cannyLayout->addWidget(
//         makeSliderBlock(cannyGroup, "Sigma", cannySigmaSlider_, dummy2,
//                         1, 100, 14,
//                         [](int v){ return QString::number(v / 10.0, 'f', 1); }));
//
//     cannyLayout->addWidget(
//         makeSliderBlock(cannyGroup, "Low Threshold", cannyLowThresholdSlider_, dummy3,
//                         0, 1000, 50));
//
//     cannyLayout->addWidget(
//         makeSliderBlock(cannyGroup, "High Threshold", cannyHighThresholdSlider_, dummy4,
//                         0, 1000, 100));
//
//     sideLayout->addWidget(cannyGroup);
//     sideLayout->addSpacing(4);
//
//     // ── Hough Lines group ──
//     houghLinesParamsWidget_ = new QGroupBox("Hough Lines", scrollWidget);
//     auto* linesLayout = new QVBoxLayout(houghLinesParamsWidget_);
//     linesLayout->setSpacing(6);
//     QLabel* dummy5 = nullptr;
//     linesLayout->addWidget(
//         makeSliderBlock(static_cast<QGroupBox*>(houghLinesParamsWidget_),
//                         "Threshold", houghLinesThresholdSlider_, dummy5,
//                         1, 1000, 100));
//     sideLayout->addWidget(houghLinesParamsWidget_);
//     sideLayout->addSpacing(4);
//
//     // ── Hough Circles group ──
//     houghCirclesParamsWidget_ = new QGroupBox("Hough Circles", scrollWidget);
//     auto* circlesLayout = new QVBoxLayout(houghCirclesParamsWidget_);
//     circlesLayout->setSpacing(6);
//     QLabel* dummy6 = nullptr;
//     circlesLayout->addWidget(
//         makeSliderBlock(static_cast<QGroupBox*>(houghCirclesParamsWidget_),
//                         "Threshold", houghCirclesThresholdSlider_, dummy6,
//                         1, 1000, 20));
//     sideLayout->addWidget(houghCirclesParamsWidget_);
//
//     sideLayout->addStretch();
//     sideLayout->addWidget(makeSeparator(scrollWidget));
//     sideLayout->addSpacing(8);
//
//     // ── Run button (bottom of sidebar) ──
//     auto* runButton = new QPushButton("⚡  Process Image", scrollWidget);
//     runButton->setObjectName("actionButton");
//     sideLayout->addWidget(runButton);
//
//     scrollWidget->setLayout(sideLayout);
//     scrollArea->setWidget(scrollWidget);
//     scrollArea->setWidgetResizable(true);
//     scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//     scrollArea->setFrameShape(QFrame::NoFrame);
//     scrollArea->setMinimumWidth(260);
//
//     controlsDock_->setWidget(scrollArea);
//     addDockWidget(Qt::LeftDockWidgetArea, controlsDock_);
//
//     connect(detectionTypeBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
//             this, &MainWindow::onDetectionTypeChanged);
//     onDetectionTypeChanged(0);
//
//     // ═══════════════════════════════════════
//     //  CENTRAL AREA
//     // ═══════════════════════════════════════
//     auto* central    = new QWidget(this);
//     auto* mainLayout = new QVBoxLayout(central);
//     mainLayout->setContentsMargins(16, 12, 16, 12);
//     mainLayout->setSpacing(10);
//
//     auto* title = new QLabel("Image Detection UI", central);
//     title->setObjectName("titleLabel");
//     mainLayout->addWidget(title);
//
//     fileLabel_ = new QLabel("No image loaded — double-click a panel to open", central);
//     fileLabel_->setObjectName("fileLabel");
//     mainLayout->addWidget(fileLabel_);
//
//
//     // Image panels
//     auto* imageLayout = new QHBoxLayout();
//     imageLayout->setSpacing(10);
//
//     inputLabel_ = new QLabel("Double-click to upload", central);
//     edgesLabel_ = new QLabel("Output will appear here", central);
//     inputLabel_->setObjectName("imagePanel");
//     edgesLabel_->setObjectName("imagePanel");
//     inputLabel_->setMinimumSize(440, 340);
//     edgesLabel_->setMinimumSize(440, 340);
//     inputLabel_->setAlignment(Qt::AlignCenter);
//     edgesLabel_->setAlignment(Qt::AlignCenter);
//     imageLayout->addWidget(inputLabel_);
//     imageLayout->addWidget(edgesLabel_);
//     mainLayout->addLayout(imageLayout);
//
//     detectionsBox_ = new QPlainTextEdit(central);
//     detectionsBox_->setReadOnly(true);
//     detectionsBox_->setPlaceholderText("Detection results will appear here after processing…");
//     detectionsBox_->setObjectName("reportBox");
//     detectionsBox_->setMaximumHeight(160);
//     mainLayout->addWidget(detectionsBox_);
//
//     setCentralWidget(central);
//     setWindowTitle("Image Detection UI");
//     resize(1100, 780);
//
//     connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunPipeline);
// }
//
// // ─────────────────────────────────────────────────────────────
// QImage MainWindow::toQImage(const backend::GrayImage& image)
// {
//     QImage out(image.width, image.height, QImage::Format_Grayscale8);
//     for (int y = 0; y < image.height; ++y)
//         for (int x = 0; x < image.width; ++x) {
//             const int v = static_cast<int>(std::clamp(image.at(x, y), 0.0f, 255.0f));
//             out.setPixel(x, y, qRgb(v, v, v));
//         }
//     return out;
// }
//
// void MainWindow::setStatusText(const QString& text)
// {
//     detectionsBox_->setPlainText(text);
// }
//
// // ─────────────────────────────────────────────────────────────
// void MainWindow::onLoadImage()
// {
//     const QString path = QFileDialog::getOpenFileName(
//         this, "Open image", {},
//         "Images (*.png *.jpg *.jpeg *.pgm *.ppm)");
//     if (path.isEmpty()) return;
//
//     QImage qImage;
//     if (!qImage.load(path)) {
//         QMessageBox::critical(this, "Load failed", "Failed to load image file.");
//         return;
//     }
//
//     image_ = backend::GrayImage(qImage.width(), qImage.height());
//     for (int y = 0; y < qImage.height(); ++y)
//         for (int x = 0; x < qImage.width(); ++x)
//             image_.at(x, y) = qGray(qImage.pixel(x, y));
//
//     const QPixmap pix = QPixmap::fromImage(qImage)
//         .scaled(inputLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
//     inputLabel_->setPixmap(pix);
//     edgesLabel_->setText("Output will appear here");
//     fileLabel_->setText("Loaded: " + path);
//     setStatusText("Image loaded. Press 'Process Image' to run the pipeline.");
// }
//
// // ─────────────────────────────────────────────────────────────
// void MainWindow::onRunPipeline()
// {
//     if (!image_.isValid()) {
//         QMessageBox::information(this, "No image", "Please upload an image first.");
//         return;
//     }
//
//     try {
//         backend::CannyParams cannyParams;
//         int kernelSize = cannyKernelSizeSlider_->value();
//         if (kernelSize % 2 == 0) kernelSize++;
//         cannyParams.gaussianKernelSize = kernelSize;
//         cannyParams.gaussianSigma      = cannySigmaSlider_->value() / 10.0;
//         cannyParams.lowThreshold       = cannyLowThresholdSlider_->value();
//         cannyParams.highThreshold      = cannyHighThresholdSlider_->value();
//
//         const backend::GrayImage edges = backend::cannyEdgeDetect(image_, cannyParams);
//         const QString detectionType    = detectionTypeBox_->currentText();
//
//         if (detectionType == "Edges") {
//             edgesLabel_->setPixmap(
//                 QPixmap::fromImage(toQImage(edges))
//                     .scaled(edgesLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
//             setStatusText("Canny edge detection complete.");
//             return;
//         }
//
//         const auto lines   = backend::detectLinesHough(edges, houghLinesThresholdSlider_->value(), 20);
//         const auto circles = backend::detectCirclesHough(edges, 10, 80, 80, houghCirclesThresholdSlider_->value());
//
//         QImage resultImage = toQImage(image_).convertToFormat(QImage::Format_ARGB32);
//         QPainter painter(&resultImage);
//         painter.setRenderHint(QPainter::Antialiasing);
//
//         std::ostringstream report;
//         report << "Detection Summary\n-----------------\n";
//
//         if (detectionType == "All" || detectionType == "Lines") {
//             report << "Lines: " << lines.size() << "\n";
//             painter.setPen(QPen(QColor(255, 80, 80), 2));
//             for (std::size_t i = 0; i < lines.size(); ++i) {
//                 report << "  L" << i + 1 << ": rho=" << lines[i].rho
//                        << ", theta=" << lines[i].theta << ", votes=" << lines[i].votes << "\n";
//                 const double rho = lines[i].rho, theta = lines[i].theta;
//                 const double a = std::cos(theta), b = std::sin(theta);
//                 const double x0 = a * rho, y0 = b * rho;
//                 painter.drawLine(
//                     static_cast<int>(x0 + 1000 * (-b)), static_cast<int>(y0 + 1000 * a),
//                     static_cast<int>(x0 - 1000 * (-b)), static_cast<int>(y0 - 1000 * a));
//             }
//         }
//
//         if (detectionType == "All" || detectionType == "Circles") {
//             report << "\nCircles: " << circles.size() << "\n";
//             painter.setPen(QPen(QColor(80, 180, 255), 2));
//             for (std::size_t i = 0; i < circles.size(); ++i) {
//                 report << "  C" << i + 1 << ": cx=" << circles[i].centerX
//                        << ", cy=" << circles[i].centerY
//                        << ", r=" << circles[i].radius
//                        << ", votes=" << circles[i].votes << "\n";
//                 painter.drawEllipse(
//                     QPoint(circles[i].centerX, circles[i].centerY),
//                     circles[i].radius, circles[i].radius);
//             }
//         }
//
//         edgesLabel_->setPixmap(
//             QPixmap::fromImage(resultImage)
//                 .scaled(edgesLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
//         setStatusText(QString::fromStdString(report.str()));
//
//     } catch (const std::exception& ex) {
//         QMessageBox::critical(this, "Processing failed", ex.what());
//     }
// }
//
// // ─────────────────────────────────────────────────────────────
// void MainWindow::mouseDoubleClickEvent(QMouseEvent* event)
// {
//     if (event->button() == Qt::LeftButton && inputLabel_->underMouse())
//         onLoadImage();
// }
//
// void MainWindow::onDetectionTypeChanged(int index)
// {
//     const QString t = detectionTypeBox_->itemText(index);
//     houghLinesParamsWidget_  ->setVisible(t == "All" || t == "Lines");
//     houghCirclesParamsWidget_->setVisible(t == "All" || t == "Circles");
// }

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
#include <sstream>

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

    // Row: param name  +  current value (right-aligned)
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

    // Slider below
    auto* slider = new QSlider(Qt::Horizontal, block);
    slider->setRange(min, max);
    slider->setValue(initial);
    layout->addWidget(slider);

    // Set initial value text
    auto fmt = formatter ? formatter : [](int v){ return QString::number(v); };
    lValue->setText(fmt(initial));

    // Connect
    QObject::connect(slider, &QSlider::valueChanged, lValue, [lValue, fmt](int v){
        lValue->setText(fmt(v));
    });

    sliderOut = slider;
    valueOut  = lValue;
    return block;
}

// ── Helper: thin horizontal separator ──
static QFrame* makeSeparator(QWidget* parent)
{
    auto* sep = new QFrame(parent);
    sep->setObjectName("separator");
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Plain);
    sep->setFixedHeight(1);
    return sep;
}

// ── Helper: section header label ──
static QLabel* makeSectionLabel(const QString& text, QWidget* parent)
{
    auto* lbl = new QLabel(text, parent);
    lbl->setObjectName("sectionLabel");
    return lbl;
}

// ─────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    // ── Load stylesheet from external file ──
    QFile qssFile(":/styles.qss");          // resource path if using .qrc
    if (qssFile.open(QFile::ReadOnly)) {
        setStyleSheet(QString::fromUtf8(qssFile.readAll()));
        qssFile.close();
    } else {
        qssFile.setFileName("styles.qss");  // fallback: same directory as binary
        if (qssFile.open(QFile::ReadOnly)) {
            setStyleSheet(QString::fromUtf8(qssFile.readAll()));
            qssFile.close();
        }
    }

    // ═══════════════════════════════════════
    //  SIDEBAR / DOCK
    // ═══════════════════════════════════════
    controlsDock_ = new QDockWidget("Controls", this);
    controlsDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    controlsDock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    // Scrollable inner container so it works at any window height
    auto* scrollArea   = new QScrollArea(controlsDock_);
    auto* scrollWidget = new QWidget(scrollArea);
    auto* sideLayout   = new QVBoxLayout(scrollWidget);
    sideLayout->setContentsMargins(14, 14, 14, 14);
    sideLayout->setSpacing(6);

    // ── Detection type ──
    sideLayout->addWidget(makeSectionLabel("Detection Mode", scrollWidget));
    detectionTypeBox_ = new QComboBox(scrollWidget);
    detectionTypeBox_->addItems({"All", "Edges", "Lines"});
    sideLayout->addWidget(detectionTypeBox_);

    sideLayout->addSpacing(8);
    sideLayout->addWidget(makeSeparator(scrollWidget));

    // ── Canny group ──
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

    // ── Hough Lines group ──
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

    // ── Run button (bottom of sidebar) ──
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

    // ═══════════════════════════════════════
    //  CENTRAL AREA
    // ═══════════════════════════════════════
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

    // Image panels
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
void MainWindow::onRunPipeline()
{
    if (!image_.isValid()) {
        QMessageBox::information(this, "No image", "Please upload an image first.");
        return;
    }

    try {
        backend::CannyParams cannyParams;
        int kernelSize = cannyKernelSizeSlider_->value();
        if (kernelSize % 2 == 0) kernelSize++;
        cannyParams.gaussianKernelSize = kernelSize;
        cannyParams.gaussianSigma      = cannySigmaSlider_->value() / 10.0;
        cannyParams.lowThreshold       = cannyLowThresholdSlider_->value();
        cannyParams.highThreshold      = cannyHighThresholdSlider_->value();

        const backend::GrayImage edges = backend::cannyEdgeDetect(image_, cannyParams);
        const QString detectionType    = detectionTypeBox_->currentText();

        if (detectionType == "Edges") {
            edgesLabel_->setPixmap(
                QPixmap::fromImage(toQImage(edges))
                    .scaled(edgesLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            setStatusText("Canny edge detection complete.");
            return;
        }

        const auto lines   = backend::detectLinesHough(edges, houghLinesThresholdSlider_->value(), 20);

        QImage resultImage = toQImage(image_).convertToFormat(QImage::Format_ARGB32);
        QPainter painter(&resultImage);
        painter.setRenderHint(QPainter::Antialiasing);

        std::ostringstream report;
        report << "Detection Summary\n-----------------\n";

        if (detectionType == "All" || detectionType == "Lines") {
            report << "Lines: " << lines.size() << "\n";
            painter.setPen(QPen(QColor(255, 80, 80), 2));
            for (std::size_t i = 0; i < lines.size(); ++i) {
                report << "  L" << i + 1 << ": rho=" << lines[i].rho
                       << ", theta=" << lines[i].theta << ", votes=" << lines[i].votes << "\n";
                const double rho = lines[i].rho, theta = lines[i].theta;
                const double a = std::cos(theta), b = std::sin(theta);
                const double x0 = a * rho, y0 = b * rho;
                painter.drawLine(
                    static_cast<int>(x0 + 1000 * (-b)), static_cast<int>(y0 + 1000 * a),
                    static_cast<int>(x0 - 1000 * (-b)), static_cast<int>(y0 - 1000 * a));
            }
        }


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


