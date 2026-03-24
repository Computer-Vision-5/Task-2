#include "MainWindow.hpp"

#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

#include "Canny.hpp"
#include "Hough.hpp"
#include "Snake.hpp"

// ── Helper: builds a labeled slider block ────────────────────────────────────
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

// =============================================================================
// Constructor
// =============================================================================
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

    watcher_ = new QFutureWatcher<PipelineResult>(this);
    connect(watcher_, &QFutureWatcher<PipelineResult>::finished,
            this,     &MainWindow::onProcessingFinished);

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
    detectionTypeBox_->addItems({"All", "Edges", "Lines", "Circles", "Ellipses", "Snake"});
    sideLayout->addWidget(detectionTypeBox_);

    sideLayout->addSpacing(8);
    sideLayout->addWidget(makeSeparator(scrollWidget));

    auto* cannyGroup  = new QGroupBox("Canny Edge Detection", scrollWidget);
    auto* cannyLayout = new QVBoxLayout(cannyGroup);
    cannyLayout->setSpacing(6);

    QLabel* dummy1 = nullptr; QLabel* dummy2 = nullptr;
    QLabel* dummy3 = nullptr; QLabel* dummy4 = nullptr;

    cannyLayout->addWidget(makeSliderBlock(cannyGroup, "Kernel Size", cannyKernelSizeSlider_, dummy1, 3, 21, 5, [](int v){ return QString::number(v % 2 == 0 ? v + 1 : v); }));
    cannyLayout->addWidget(makeSliderBlock(cannyGroup, "Sigma", cannySigmaSlider_, dummy2, 1, 100, 14, [](int v){ return QString::number(v / 10.0, 'f', 1); }));
    cannyLayout->addWidget(makeSliderBlock(cannyGroup, "Low Threshold", cannyLowThresholdSlider_, dummy3, 0, 1000, 50));
    cannyLayout->addWidget(makeSliderBlock(cannyGroup, "High Threshold", cannyHighThresholdSlider_, dummy4, 0, 1000, 100));

    sideLayout->addWidget(cannyGroup);
    sideLayout->addSpacing(4);

    houghLinesParamsWidget_ = new QGroupBox("Hough Lines", scrollWidget);
    auto* linesLayout = new QVBoxLayout(houghLinesParamsWidget_);
    linesLayout->setSpacing(6);
    QLabel* dummy5 = nullptr;
    linesLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(houghLinesParamsWidget_), "Threshold", houghLinesThresholdSlider_, dummy5, 1, 1000, 100));
    sideLayout->addWidget(houghLinesParamsWidget_);
    sideLayout->addSpacing(4);

    houghCirclesParamsWidget_ = new QGroupBox("Hough Circles", scrollWidget);
    auto* circlesLayout = new QVBoxLayout(houghCirclesParamsWidget_);
    circlesLayout->setSpacing(6);
    QLabel* dC1 = nullptr; QLabel* dC2 = nullptr; QLabel* dC3 = nullptr;
    circlesLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(houghCirclesParamsWidget_), "Threshold", houghCirclesThresholdSlider_, dC1, 1, 500, 15));
    circlesLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(houghCirclesParamsWidget_), "Min Radius", houghCirclesMinRSlider_, dC2, 5, 200, 15));
    circlesLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(houghCirclesParamsWidget_), "Max Radius", houghCirclesMaxRSlider_, dC3, 10, 250, 150));
    sideLayout->addWidget(houghCirclesParamsWidget_);
    sideLayout->addSpacing(4);

    houghEllipsesParamsWidget_ = new QGroupBox("Hough Ellipses", scrollWidget);
    auto* ellipsesLayout = new QVBoxLayout(houghEllipsesParamsWidget_);
    ellipsesLayout->setSpacing(6);
    QLabel* dE1 = nullptr; QLabel* dE2 = nullptr; QLabel* dE3 = nullptr;
    ellipsesLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(houghEllipsesParamsWidget_), "Threshold", houghEllipsesThresholdSlider_, dE1, 1, 500, 60));
    ellipsesLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(houghEllipsesParamsWidget_), "Min Axis", houghEllipsesMinASlider_, dE2, 10, 100, 20));
    ellipsesLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(houghEllipsesParamsWidget_), "Max Axis", houghEllipsesMaxASlider_, dE3, 20, 400, 80));
    sideLayout->addWidget(houghEllipsesParamsWidget_);
    sideLayout->addSpacing(4);

    snakeParamsWidget_ = new QGroupBox("Active Contour (Snake)", scrollWidget);
    auto* snakeLayout  = new QVBoxLayout(snakeParamsWidget_);
    snakeLayout->setSpacing(6);

    QLabel *ds1=nullptr, *ds2=nullptr, *ds3=nullptr, *ds4=nullptr, *ds5=nullptr, *ds6=nullptr;

    snakeLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(snakeParamsWidget_), "a  Elasticity", snakeAlphaSlider_, ds1, 0, 100, 40, [](int v){ return QString::number(v / 100.0, 'f', 2); }));
    snakeLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(snakeParamsWidget_), "b  Smoothness", snakeBetaSlider_, ds2, 0, 100, 40, [](int v){ return QString::number(v / 100.0, 'f', 2); }));
    snakeLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(snakeParamsWidget_), "Window W", snakeWindowSlider_, ds3, 1, 20, 5));
    snakeLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(snakeParamsWidget_), "Contour Points", snakeNumPointsSlider_, ds4, 10, 200, 60));
    snakeLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(snakeParamsWidget_), "Max Iterations", snakeMaxIterSlider_, ds5, 10, 500, 200));
    snakeLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(snakeParamsWidget_), "Convergence x0.01", snakeThresholdSlider_, ds6, 1, 200, 50, [](int v){ return QString::number(v / 100.0, 'f', 2); }));

    QLabel* ds7 = nullptr;
    snakeLayout->addWidget(makeSliderBlock(static_cast<QGroupBox*>(snakeParamsWidget_), "Grad Blur Passes", snakeGradBlurSlider_, ds7, 1, 15, 3));

    auto* hintLabel = new QLabel("<i>Click &amp; drag on the left image<br>to place the initial circle.</i>", snakeParamsWidget_);
    hintLabel->setWordWrap(true);
    hintLabel->setObjectName("paramName");
    snakeLayout->addWidget(hintLabel);

    sideLayout->addWidget(snakeParamsWidget_);
    sideLayout->addSpacing(4);

    sideLayout->addStretch();
    sideLayout->addWidget(makeSeparator(scrollWidget));
    sideLayout->addSpacing(8);

    processButton_ = new QPushButton("  Process Image", scrollWidget);
    processButton_->setObjectName("actionButton");
    sideLayout->addWidget(processButton_);

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

    fileLabel_ = new QLabel("No image loaded -- double-click a panel to open", central);
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
    detectionsBox_->setPlaceholderText("Detection results will appear here after processing...");
    detectionsBox_->setObjectName("reportBox");
    detectionsBox_->setMaximumHeight(160);
    mainLayout->addWidget(detectionsBox_);

    setCentralWidget(central);
    setWindowTitle("Image Detection UI");
    resize(1100, 780);

    connect(processButton_, &QPushButton::clicked, this, &MainWindow::onRunPipeline);
    inputLabel_->installEventFilter(this);
    edgesLabel_->installEventFilter(this);
}

// =============================================================================
// Helpers
// =============================================================================
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

void MainWindow::onLoadImage()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Open image", {},
        "Images (*.png *.jpg *.jpeg)");
    if (path.isEmpty()) return;

    QImage qImage;
    if (!qImage.load(path)) {
        QMessageBox::critical(this, "Load failed", "Failed to load image file.");
        return;
    }
    if (qImage.width() > 800 || qImage.height() > 800) {
            qImage = qImage.scaled(800, 800, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    image_ = backend::GrayImage(qImage.width(), qImage.height());
    for (int y = 0; y < qImage.height(); ++y)
        for (int x = 0; x < qImage.width(); ++x)
            image_.at(x, y) = qGray(qImage.pixel(x, y));

    snakeRadius_ = 0.0f;
    snakeContour_.clear();
    gradientMag_ = backend::GrayImage{};
    inputZoom_   = 1.0;
    outputZoom_  = 1.0;
    outputBasePixmap_ = QPixmap{};

    // Store full-size pixmap first; then show scaled version
    inputBasePixmap_ = QPixmap::fromImage(qImage);
    const QPixmap pix = inputBasePixmap_
        .scaled(inputLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    inputLabel_->setPixmap(pix);
    snakeOverlayPixmap_ = pix;

    edgesLabel_->setText("Output will appear here");
    fileLabel_->setText("Loaded: " + path);
    setStatusText("Image loaded. Press 'Process Image' to run the pipeline.");
}

// ── Helper: draw the placed snake circle onto the current overlay ────────────
void MainWindow::redrawSnakeCircle()
{
    if (snakeRadius_ < 1.0f) return;

    const QPixmap& pm = inputLabel_->pixmap();
    const QSize    ls = inputLabel_->size();
    const double   ox = (ls.width()  - pm.width())  / 2.0;
    const double   oy = (ls.height() - pm.height()) / 2.0;

    // Convert image-space centre → pixmap space
    const double scaleX = static_cast<double>(pm.width())  / image_.width;
    const double scaleY = static_cast<double>(pm.height()) / image_.height;
    const double cx = snakeCenter_.x() * scaleX;
    const double cy = snakeCenter_.y() * scaleY;
    const double r  = static_cast<double>(snakeRadius_) * scaleX;

    QPixmap overlay = snakeOverlayPixmap_;
    QPainter p(&overlay);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(0, 220, 255), 2));
    p.drawEllipse(QPointF(cx, cy), r, r);
    (void)ox; (void)oy;
    inputLabel_->setPixmap(overlay);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    // ── Scroll-wheel zoom for both image panels ────────────────────────────
    if (event->type() == QEvent::Wheel &&
        (watched == inputLabel_ || watched == edgesLabel_))
    {
        auto* we = static_cast<QWheelEvent*>(event);
        const double delta  = we->angleDelta().y();
        const double factor = (delta > 0) ? 1.15 : (1.0 / 1.15);

        if (watched == inputLabel_ && !inputBasePixmap_.isNull()) {
            inputZoom_ = std::clamp(inputZoom_ * factor, 0.25, 4.0);
            // Build scaled pixmap from the base and paint the circle overlay
            const QSize baseSize = inputLabel_->size();
            const QPixmap scaledBase = inputBasePixmap_
                .scaled(baseSize * inputZoom_, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            snakeOverlayPixmap_ = scaledBase;
            if (snakeRadius_ >= 1.0f)
                redrawSnakeCircle();
            else
                inputLabel_->setPixmap(scaledBase);
        } else if (watched == edgesLabel_ && !outputBasePixmap_.isNull()) {
            outputZoom_ = std::clamp(outputZoom_ * factor, 0.25, 4.0);
            const QSize baseSize = edgesLabel_->size();
            edgesLabel_->setPixmap(
                outputBasePixmap_.scaled(baseSize * outputZoom_,
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation));
        }
        return true;
    }

    // Only snake-circle interactions live on the input label
    if (watched != inputLabel_) return QMainWindow::eventFilter(watched, event);
    if (detectionTypeBox_->currentText() != "Snake") return QMainWindow::eventFilter(watched, event);
    if (!image_.isValid()) return QMainWindow::eventFilter(watched, event);

    // ── Coordinate helpers ────────────────────────────────────────────────
    auto pixmapOffset = [this]() -> QPointF {
        const QPixmap& pm = inputLabel_->pixmap();
        const QSize    ls = inputLabel_->size();
        return { static_cast<double>((ls.width()  - pm.width())  / 2),
                 static_cast<double>((ls.height() - pm.height()) / 2) };
    };

    auto labelToPixmap = [&](const QPointF& lp) -> QPointF {
        const QPointF off = pixmapOffset();
        return { lp.x() - off.x(), lp.y() - off.y() };
    };

    auto labelToImage = [&](const QPointF& lp) -> QPointF {
        const QPixmap& pm = inputLabel_->pixmap();
        const QPointF  pp = labelToPixmap(lp);
        const double scaleX = static_cast<double>(image_.width)  / pm.width();
        const double scaleY = static_cast<double>(image_.height) / pm.height();
        return { pp.x() * scaleX, pp.y() * scaleY };
    };

    auto labelRadiusToImage = [&](double r) -> double {
        const QPixmap& pm = inputLabel_->pixmap();
        const double scaleX = static_cast<double>(image_.width) / pm.width();
        return r * scaleX;
    };

    // Convert image-space centre back to pixmap-space (needed for hit-test)
    auto imageCenterToLabel = [&]() -> QPointF {
        const QPixmap& pm  = inputLabel_->pixmap();
        const QPointF  off = pixmapOffset();
        const double scaleX = static_cast<double>(pm.width())  / image_.width;
        const double scaleY = static_cast<double>(pm.height()) / image_.height;
        return { snakeCenter_.x() * scaleX + off.x(),
                 snakeCenter_.y() * scaleY + off.y() };
    };

    auto imageRadiusToLabel = [&]() -> double {
        const QPixmap& pm  = inputLabel_->pixmap();
        const double scaleX = static_cast<double>(pm.width()) / image_.width;
        return static_cast<double>(snakeRadius_) * scaleX;
    };

    // ── Mouse press ──────────────────────────────────────────────────────
    if (event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            const QPointF pos = me->position();

            // If a circle already exists, check if the user clicked inside it
            if (snakeRadius_ >= 1.0f) {
                const QPointF cLabel = imageCenterToLabel();
                const double  rLabel = imageRadiusToLabel();
                const double  dx     = pos.x() - cLabel.x();
                const double  dy     = pos.y() - cLabel.y();
                if (std::sqrt(dx*dx + dy*dy) <= rLabel + 8.0) {
                    // Start moving the existing circle
                    snakeMoving_     = true;
                    snakeMoveOffset_ = QPointF(pos.x() - cLabel.x(), pos.y() - cLabel.y());
                    return true;
                }
            }

            // Otherwise start drawing a new circle
            snakePressPos_ = pos;
            snakePlacing_  = true;
            return true;
        }
    }

    // ── Mouse move – drawing new circle ─────────────────────────────────
    if (event->type() == QEvent::MouseMove && snakePlacing_) {
        auto* me = static_cast<QMouseEvent*>(event);
        const QPointF cur = me->position();
        const double  dx  = cur.x() - snakePressPos_.x();
        const double  dy  = cur.y() - snakePressPos_.y();
        const double  r   = std::sqrt(dx*dx + dy*dy);

        const QPointF pmCenter = labelToPixmap(snakePressPos_);

        QPixmap overlay = snakeOverlayPixmap_;
        QPainter p(&overlay);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(0, 220, 255), 2, Qt::DashLine));
        p.drawEllipse(pmCenter, r, r);
        inputLabel_->setPixmap(overlay);
        return true;
    }

    // ── Mouse move – moving existing circle ──────────────────────────────
    if (event->type() == QEvent::MouseMove && snakeMoving_) {
        auto* me = static_cast<QMouseEvent*>(event);
        const QPointF pos = me->position();
        // New label-space centre
        const QPointF newCLabel(
            pos.x() - snakeMoveOffset_.x(),
            pos.y() - snakeMoveOffset_.y());
        // Convert to image space
        snakeCenter_ = labelToImage(newCLabel);

        // Redraw overlay with the circle at the new position
        const QPointF pmCenter = labelToPixmap(newCLabel);
        const double  rLabel   = imageRadiusToLabel();

        QPixmap overlay = snakeOverlayPixmap_;
        QPainter p(&overlay);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(0, 220, 255), 2, Qt::DashLine));
        p.drawEllipse(pmCenter, rLabel, rLabel);
        inputLabel_->setPixmap(overlay);
        return true;
    }

    // ── Mouse release – finish drawing new circle ────────────────────────
    if (event->type() == QEvent::MouseButtonRelease && snakePlacing_) {
        auto* me = static_cast<QMouseEvent*>(event);
        const QPointF cur = me->position();
        const double  dx  = cur.x() - snakePressPos_.x();
        const double  dy  = cur.y() - snakePressPos_.y();
        const double  r   = std::sqrt(dx*dx + dy*dy);

        snakePlacing_ = false;

        if (r < 3.0) return true;

        snakeCenter_ = labelToImage(snakePressPos_);
        snakeRadius_ = static_cast<float>(labelRadiusToImage(r));

        const QPointF pmCenter = labelToPixmap(snakePressPos_);
        QPixmap overlay = snakeOverlayPixmap_;
        QPainter p(&overlay);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(0, 220, 255), 2));
        p.drawEllipse(pmCenter, r, r);
        inputLabel_->setPixmap(overlay);

        setStatusText(
            QString("Snake circle set -- centre (%.0f, %.0f), radius %.0f px in image space. "
                    "Press 'Process Image' to evolve.")
                .arg(snakeCenter_.x())
                .arg(snakeCenter_.y())
                .arg(snakeRadius_));
        return true;
    }

    // ── Mouse release – finish moving existing circle ────────────────────
    if (event->type() == QEvent::MouseButtonRelease && snakeMoving_) {
        snakeMoving_ = false;
        // Solidify the circle with a solid line
        redrawSnakeCircle();
        setStatusText(
            QString("Snake circle moved -- centre (%.0f, %.0f), radius %.0f px. "
                    "Press 'Process Image' to evolve.")
                .arg(snakeCenter_.x())
                .arg(snakeCenter_.y())
                .arg(snakeRadius_));
        return true;
    }

    return QMainWindow::eventFilter(watched, event);
}

static std::vector<std::pair<QPoint, QPoint>> extractLineSegments(
    const backend::GrayImage& edges,
    double rho, double theta,
    double distTolerance = 1.5,
    double maxGap        = 8.0,
    double minSegmentLen = 15.0)
{
    const double dx =  std::sin(theta);
    const double dy = -std::cos(theta);
    const double ox = rho * std::cos(theta);
    const double oy = rho * std::sin(theta);

    std::vector<double> tValues;
    tValues.reserve(512);

    for (int y = 0; y < edges.height; ++y) {
        for (int x = 0; x < edges.width; ++x) {
            if (edges.at(x, y) < 128.0f) continue;
            const double dist = std::cos(theta) * x + std::sin(theta) * y - rho;
            if (std::abs(dist) > distTolerance) continue;
            const double t = dx * (x - ox) + dy * (y - oy);
            tValues.push_back(t);
        }
    }

    if (tValues.empty()) return {};
    std::sort(tValues.begin(), tValues.end());

    std::vector<std::pair<QPoint, QPoint>> segments;
    double segStart = tValues.front();
    double segEnd   = tValues.front();

    auto tToPoint = [&](double t) -> QPoint {
        return QPoint(static_cast<int>(std::round(ox + t * dx)),
                      static_cast<int>(std::round(oy + t * dy)));
    };

    for (std::size_t i = 1; i < tValues.size(); ++i) {
        const double gap = tValues[i] - tValues[i - 1];
        if (gap <= maxGap) {
            segEnd = tValues[i];
        } else {
            if (segEnd - segStart >= minSegmentLen)
                segments.emplace_back(tToPoint(segStart), tToPoint(segEnd));
            segStart = tValues[i];
            segEnd   = tValues[i];
        }
    }
    if (segEnd - segStart >= minSegmentLen)
        segments.emplace_back(tToPoint(segStart), tToPoint(segEnd));

    return segments;
}

void MainWindow::onRunPipeline()
{
    if (!image_.isValid()) {
        QMessageBox::information(this, "No image", "Please upload an image first.");
        return;
    }
    if (watcher_->isRunning()) return;

    const QString detectionType = detectionTypeBox_->currentText();

    backend::CannyParams cannyParams;
    {
        int kernelSize = cannyKernelSizeSlider_->value();
        if (kernelSize % 2 == 0) ++kernelSize;
        cannyParams.gaussianKernelSize = kernelSize;
        cannyParams.gaussianSigma      = cannySigmaSlider_->value() / 10.0;
        cannyParams.lowThreshold       = cannyLowThresholdSlider_->value();
        cannyParams.highThreshold      = cannyHighThresholdSlider_->value();
    }

    const int    houghThreshold = houghLinesThresholdSlider_->value();
    const int    circleThreshold = houghCirclesThresholdSlider_->value();
    const int    minRadius = houghCirclesMinRSlider_->value();
    const int    maxRadius = houghCirclesMaxRSlider_->value();
    const int    ellipseThreshold = houghEllipsesThresholdSlider_->value();
    const int    minA = houghEllipsesMinASlider_->value();
    const int    maxA = houghEllipsesMaxASlider_->value();

    backend::SnakeParams snakeParams;
    snakeParams.alpha                = snakeAlphaSlider_->value()     / 100.0f;
    snakeParams.beta                 = snakeBetaSlider_->value()      / 100.0f;
    snakeParams.windowSize           = snakeWindowSlider_->value();
    snakeParams.numPoints            = snakeNumPointsSlider_->value();
    snakeParams.maxIterations        = snakeMaxIterSlider_->value();
    snakeParams.convergenceThreshold = snakeThresholdSlider_->value() / 100.0f;
    snakeParams.gradientBlurPasses   = snakeGradBlurSlider_->value();

    const backend::GrayImage  imageCopy       = image_;
    const float               snakeRadius     = snakeRadius_;
    const QPointF             snakeCenter     = snakeCenter_;

    if (detectionType == "Snake" && snakeRadius < 1.0f) {
        QMessageBox::information(this, "Snake",
            "Please click and drag on the left image to set the initial circle first.");
        return;
    }

    processButton_->setEnabled(false);
    processButton_->setText("Processing...");
    setStatusText("Processing...");

    QFuture<PipelineResult> future = QtConcurrent::run(
        [imageCopy, cannyParams, houghThreshold,
         detectionType, snakeParams, snakeRadius, snakeCenter,
         circleThreshold, minRadius, maxRadius,
         ellipseThreshold, minA, maxA]() -> PipelineResult
    {
        PipelineResult result;
        auto clampC = [](int v, int bound){ return v < 0 ? 0 : (v >= bound ? bound-1 : v); };

        const backend::GrayImage edges = backend::cannyEdgeDetect(imageCopy, cannyParams);

        backend::GrayImage gradMag(imageCopy.width, imageCopy.height, 0.0f);
        {
            static constexpr int kx[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
            static constexpr int ky[9] = { 1, 2, 1,  0, 0, 0, -1,-2,-1};
            for (int y = 0; y < imageCopy.height; ++y)
                for (int x = 0; x < imageCopy.width; ++x) {
                    float gx = 0.f, gy = 0.f;
                    int idx = 0;
                    for (int j = -1; j <= 1; ++j)
                        for (int i = -1; i <= 1; ++i, ++idx) {
                            const float px = imageCopy.at(clampC(x+i, imageCopy.width),
                                                           clampC(y+j, imageCopy.height));
                            gx += kx[idx] * px;
                            gy += ky[idx] * px;
                        }
                    gradMag.at(x, y) = std::hypot(gx, gy);
                }
        }

        // ── Solution 4: Normalise gradient map to [0,255] globally ──────────
        // This ensures consistent edge strength regardless of image brightness.
        {
            float maxG = 0.f;
            for (const float& px : gradMag.pixels) maxG = std::max(maxG, px);
            if (maxG > 0.f)
                for (float& px : gradMag.pixels) px = px / maxG * 255.f;
        }

        if (detectionType == "Edges") {
            QImage out(imageCopy.width, imageCopy.height, QImage::Format_Grayscale8);
            for (int y = 0; y < imageCopy.height; ++y)
                for (int x = 0; x < imageCopy.width; ++x) {
                    const int v = static_cast<int>(std::clamp(edges.at(x,y), 0.f, 255.f));
                    out.setPixel(x, y, qRgb(v,v,v));
                }
            result.outputImage = out;
            result.statusText  = "Canny edge detection complete.";
            return result;
        }

        if (detectionType == "Snake") {
            const backend::SnakeContour initial =
                backend::initContour(
                    static_cast<float>(snakeCenter.x()),
                    static_cast<float>(snakeCenter.y()),
                    snakeRadius,
                    snakeParams.numPoints);

            const backend::SnakeContour evolved =
                backend::evolveContour(initial, gradMag, snakeParams);

            QImage img(imageCopy.width, imageCopy.height, QImage::Format_ARGB32);
            for (int y = 0; y < imageCopy.height; ++y)
                for (int x = 0; x < imageCopy.width; ++x) {
                    const int v = static_cast<int>(std::clamp(imageCopy.at(x,y), 0.f, 255.f));
                    img.setPixel(x, y, qRgb(v,v,v));
                }

            QPainter painter(&img);
            painter.setRenderHint(QPainter::Antialiasing);

            painter.setPen(QPen(QColor(0, 220, 255), 2, Qt::DashLine));
            painter.drawEllipse(snakeCenter,
                                static_cast<double>(snakeRadius),
                                static_cast<double>(snakeRadius));

            painter.setPen(QPen(QColor(255, 220, 0), 2));
            const int N = static_cast<int>(evolved.size());
            for (int i = 0; i < N; ++i) {
                const auto& a = evolved[static_cast<size_t>(i)];
                const auto& b = evolved[static_cast<size_t>((i+1)%N)];
                painter.drawLine(QPointF(a[0],a[1]), QPointF(b[0],b[1]));
            }
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 80, 80));
            for (const auto& pt : evolved)
                painter.drawEllipse(QPointF(pt[0],pt[1]), 2.0, 2.0);
            painter.end();

            result.chainCodes        = backend::contourChainCode(evolved);
            result.contourArea       = backend::contourArea(evolved);
            result.contourPerimeter  = backend::contourPerimeter(evolved);

            result.outputImage  = img;
            result.snakeContour = evolved;
            result.statusText   =
                QString("Active Contour complete.\n"
                        "Points: %1 | \u03b1=%2 | \u03b2=%3 | W=%4 | MaxIter=%5\n"
                        "Cyan = initial circle | Yellow = evolved contour | Red dots = contour points")
                    .arg(N)
                    .arg(snakeParams.alpha,  0, 'f', 2)
                    .arg(snakeParams.beta,   0, 'f', 2)
                    .arg(snakeParams.windowSize)
                    .arg(snakeParams.maxIterations);
            return result;
        }

        const auto lines = backend::detectLinesHough(edges, houghThreshold, 20);
        
        std::vector<backend::CircleDetection> circles;
        if (detectionType == "All" || detectionType == "Circles") {
             circles = backend::detectCirclesHough(edges, minRadius, maxRadius, circleThreshold, 20);
        }
        
        std::vector<backend::EllipseDetection> ellipses;
        if (detectionType == "All" || detectionType == "Ellipses") {
            ellipses = backend::detectEllipsesHough(edges, minA, maxA, minA, maxA, ellipseThreshold, 20);
        }

        QImage img(imageCopy.width, imageCopy.height, QImage::Format_ARGB32);
        for (int y = 0; y < imageCopy.height; ++y)
            for (int x = 0; x < imageCopy.width; ++x) {
                const int v = static_cast<int>(std::clamp(imageCopy.at(x,y), 0.f, 255.f));
                img.setPixel(x, y, qRgb(v,v,v));
            }

        QPainter painter(&img);
        painter.setRenderHint(QPainter::Antialiasing);

        std::ostringstream report;
        report << "Detection Summary\n-----------------\n";
        int totalSegments = 0;

        if (detectionType == "All" || detectionType == "Lines") {
            painter.setPen(QPen(QColor(255, 50, 50), 2));
            report << "Lines found: " << lines.size() << "\n\n";

            for (std::size_t i = 0; i < lines.size(); ++i) {
                const double rho   = lines[i].rho;
                const double theta = lines[i].theta;
                report << "  L" << (i+1)
                       << ": rho=" << static_cast<int>(rho)
                       << "  theta=" << static_cast<int>(theta * 180.0 / M_PI) << "deg"
                       << "  votes=" << lines[i].votes << "\n";

                {
                    const double dx =  std::sin(theta);
                    const double dy = -std::cos(theta);
                    const double ox = rho * std::cos(theta);
                    const double oy = rho * std::sin(theta);
                    std::vector<double> tv;
                    for (int ey = 0; ey < edges.height; ++ey)
                        for (int ex = 0; ex < edges.width; ++ex) {
                            if (edges.at(ex,ey) < 128.f) continue;
                            const double dist = std::cos(theta)*ex + std::sin(theta)*ey - rho;
                            if (std::abs(dist) > 1.5) continue;
                            tv.push_back(dx*(ex-ox)+dy*(ey-oy));
                        }
                    std::sort(tv.begin(), tv.end());
                    if (!tv.empty()) {
                        double ss = tv.front(), se = tv.front();
                        auto toP = [&](double t){ return QPointF(ox+t*dx, oy+t*dy); };
                        for (size_t k = 1; k < tv.size(); ++k) {
                            if (tv[k]-tv[k-1] <= 8.0) { se = tv[k]; }
                            else {
                                if (se-ss >= 15.0) { painter.drawLine(toP(ss),toP(se)); ++totalSegments; }
                                ss = se = tv[k];
                            }
                        }
                        if (se-ss >= 15.0) { painter.drawLine(toP(ss),toP(se)); ++totalSegments; }
                    }
                }
            }
            report << "Total line segments: " << totalSegments << "\n";
        }

        if (detectionType == "All" || detectionType == "Circles") {
            painter.setPen(QPen(QColor(100, 255, 100), 2));
            painter.setBrush(Qt::NoBrush);
            report << "Circles found: " << circles.size() << "\n\n";

            for (std::size_t i = 0; i < circles.size(); ++i) {
                const auto& c = circles[i];
                if (c.centerX < 0 || c.centerX >= imageCopy.width ||
                    c.centerY < 0 || c.centerY >= imageCopy.height ||
                    c.radius <= 0) {
                    continue;
                }
                
                report << "  C" << (i+1)
                       << ": x=" << c.centerX
                       << "  y=" << c.centerY
                       << "  r=" << c.radius
                       << "  votes=" << c.votes << "\n";
                painter.drawEllipse(QPointF(c.centerX, c.centerY), c.radius, c.radius);
            }
            report << "\n";
        }

        if (detectionType == "All" || detectionType == "Ellipses") {
            painter.setPen(QPen(QColor(100, 100, 255), 2));
            painter.setBrush(Qt::NoBrush);
            report << "Ellipses found: " << ellipses.size() << "\n\n";

            for (std::size_t i = 0; i < ellipses.size(); ++i) {
                const auto& e = ellipses[i];
                if (e.centerX < 0 || e.centerX >= imageCopy.width ||
                    e.centerY < 0 || e.centerY >= imageCopy.height ||
                    e.a <= 0 || e.b <= 0) {
                    continue;
                }
                
                report << "  E" << (i+1)
                       << ": x=" << e.centerX
                       << "  y=" << e.centerY
                       << "  a=" << e.a
                       << "  b=" << e.b
                       << "  angle=" << static_cast<int>(e.angle * 180.0 / M_PI) << "deg"
                       << "  votes=" << e.votes << "\n";

                painter.save();
                painter.translate(e.centerX, e.centerY);
                painter.rotate(e.angle * 180.0 / M_PI);
                painter.drawEllipse(QPointF(0, 0), e.a, e.b);
                painter.restore();
            }
            report << "\n";
        }

        painter.end();

        result.outputImage = img;
        result.statusText  = QString::fromStdString(report.str());
        return result;
    });

    watcher_->setFuture(future);
}

void MainWindow::onProcessingFinished()
{
    PipelineResult result = watcher_->result();
    snakeContour_ = result.snakeContour;

    // Store full-res output pixmap so zoom can resample it
    outputBasePixmap_ = QPixmap::fromImage(result.outputImage);
    edgesLabel_->setPixmap(
        outputBasePixmap_.scaled(
            edgesLabel_->size() * outputZoom_,
            Qt::KeepAspectRatio, Qt::SmoothTransformation));

    QString fullText = result.statusText;
    if (!result.chainCodes.empty()) {
        fullText += QString("\n\n── Contour Analysis ──\n");
        fullText += QString("Area      : %1 px\u00b2\n").arg(static_cast<double>(result.contourArea), 0, 'f', 1);
        fullText += QString("Perimeter : %1 px\n").arg(static_cast<double>(result.contourPerimeter), 0, 'f', 1);
        fullText += QString("Chain Code (%1 codes):\n").arg(result.chainCodes.size());
        QString codeLine;
        for (int ci = 0; ci < static_cast<int>(result.chainCodes.size()); ++ci) {
            codeLine += QString::number(result.chainCodes[static_cast<size_t>(ci)]);
            if (ci + 1 < static_cast<int>(result.chainCodes.size())) {
                codeLine += (((ci + 1) % 20 == 0) ? "\n" : " ");
            }
        }
        fullText += codeLine;
    }
    setStatusText(fullText);

    processButton_->setEnabled(true);
    processButton_->setText("  Process Image");
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && inputLabel_->underMouse())
        onLoadImage();
}

void MainWindow::onDetectionTypeChanged(int index)
{
    const QString t = detectionTypeBox_->itemText(index);
    houghLinesParamsWidget_->setVisible(t == "All" || t == "Lines");
    houghCirclesParamsWidget_->setVisible(t == "All" || t == "Circles");
    houghEllipsesParamsWidget_->setVisible(t == "All" || t == "Ellipses");
    snakeParamsWidget_->setVisible(t == "Snake");
}
