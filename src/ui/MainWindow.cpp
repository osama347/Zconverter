#include "MainWindow.h"
#include <QMimeData>
#include <QFileInfo>
#include <QUrl>
#include <QList>
#include <QDebug>

MainWindow::MainWindow(
    std::shared_ptr<zc::ConversionEngine> engine,
    QWidget* parent)
    : QMainWindow(parent)
    , engine_(std::move(engine))
{
    setWindowTitle("zconverter");
    setMinimumSize(900, 600);
    resize(1000, 650);
    setAcceptDrops(true);

    setupUI();
    applyStyles();

    connect(engine_.get(), &zc::ConversionEngine::jobStarted,
        this, &MainWindow::onJobStarted,
        Qt::QueuedConnection);
    connect(engine_.get(), &zc::ConversionEngine::jobProgress,
        this, &MainWindow::onJobProgress,
        Qt::QueuedConnection);
    connect(engine_.get(), &zc::ConversionEngine::jobFinished,
        this, &MainWindow::onJobFinished,
        Qt::QueuedConnection);
    connect(engine_.get(), &zc::ConversionEngine::allJobsDone,
        this, &MainWindow::onAllJobsDone,
        Qt::QueuedConnection);
}

void MainWindow::setupUI()
{
    centralWidget_ = new QWidget(this);
    rootLayout_ = new QHBoxLayout(centralWidget_);
    rootLayout_->setContentsMargins(0, 0, 0, 0);
    rootLayout_->setSpacing(0);

    setupSidebar();
    setupMainArea();

    rootLayout_->addWidget(sidebar_);
    rootLayout_->addWidget(mainArea_, 1);

    setCentralWidget(centralWidget_);
    statusBar()->showMessage("Ready — drop image files to convert to PDF");
}

void MainWindow::setupSidebar()
{
    sidebar_ = new QFrame();
    sidebar_->setObjectName("sidebar");
    sidebar_->setFixedWidth(200);

    sidebarLayout_ = new QVBoxLayout(sidebar_);
    sidebarLayout_->setContentsMargins(16, 24, 16, 24);
    sidebarLayout_->setSpacing(8);

    QLabel* appName = new QLabel("zconverter");
    appName->setObjectName("appName");
    appName->setAlignment(Qt::AlignCenter);

    QLabel* appVersion = new QLabel("v1.0.0");
    appVersion->setObjectName("appVersion");
    appVersion->setAlignment(Qt::AlignCenter);

    QFrame* divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setObjectName("divider");

    btnConvert_ = new QPushButton("  Convert to PDF");
    btnHistory_ = new QPushButton("  History");
    btnSettings_ = new QPushButton("  Settings");

    btnConvert_->setCheckable(true);
    btnHistory_->setCheckable(true);
    btnSettings_->setCheckable(true);
    btnConvert_->setChecked(true);

    btnConvert_->setObjectName("navButton");
    btnHistory_->setObjectName("navButton");
    btnSettings_->setObjectName("navButton");

    btnConvert_->setCursor(Qt::PointingHandCursor);
    btnHistory_->setCursor(Qt::PointingHandCursor);
    btnSettings_->setCursor(Qt::PointingHandCursor);

    sidebarLayout_->addWidget(appName);
    sidebarLayout_->addWidget(appVersion);
    sidebarLayout_->addSpacing(8);
    sidebarLayout_->addWidget(divider);
    sidebarLayout_->addSpacing(8);
    sidebarLayout_->addWidget(btnConvert_);
    sidebarLayout_->addWidget(btnHistory_);
    sidebarLayout_->addWidget(btnSettings_);
    sidebarLayout_->addStretch();

    QLabel* tagline = new QLabel("Offline · Private · Fast");
    tagline->setObjectName("appVersion");
    tagline->setAlignment(Qt::AlignCenter);
    tagline->setWordWrap(true);
    sidebarLayout_->addWidget(tagline);

    connect(btnConvert_, &QPushButton::clicked,
        this, &MainWindow::onConvertClicked);
    connect(btnHistory_, &QPushButton::clicked,
        this, &MainWindow::onHistoryClicked);
    connect(btnSettings_, &QPushButton::clicked,
        this, &MainWindow::onSettingsClicked);
}

void MainWindow::setupMainArea()
{
    mainArea_ = new QFrame();
    mainArea_->setObjectName("mainArea");

    mainAreaLayout_ = new QVBoxLayout(mainArea_);
    mainAreaLayout_->setContentsMargins(32, 32, 32, 16);
    mainAreaLayout_->setSpacing(16);

    QLabel* pageTitle = new QLabel("Convert to PDF");
    pageTitle->setObjectName("pageTitle");

    QLabel* pageSubtitle = new QLabel(
        "Drop image files below (JPG, PNG, BMP, TIFF, WEBP). "
        "Each file will be converted to a separate PDF. "
        "100% offline — nothing leaves your machine."
    );
    pageSubtitle->setObjectName("pageSubtitle");
    pageSubtitle->setWordWrap(true);

    mainAreaLayout_->addWidget(pageTitle);
    mainAreaLayout_->addWidget(pageSubtitle);

    setupDropZone();

    fileList_ = new QListWidget();
    fileList_->setObjectName("fileList");
    fileList_->setAlternatingRowColors(true);

    progressBar_ = new QProgressBar();
    progressBar_->setObjectName("progressBar");
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setVisible(false);
    progressBar_->setFixedHeight(6);
    progressBar_->setTextVisible(false);

    mainAreaLayout_->addWidget(dropZone_);
    mainAreaLayout_->addWidget(fileList_, 1);
    mainAreaLayout_->addWidget(progressBar_);

    setupBottomBar();
    mainAreaLayout_->addWidget(bottomBar_);
}

void MainWindow::setupDropZone()
{
    dropZone_ = new QFrame();
    dropZone_->setObjectName("dropZone");
    dropZone_->setFixedHeight(160);

    QVBoxLayout* dz = new QVBoxLayout(dropZone_);
    dz->setAlignment(Qt::AlignCenter);
    dz->setSpacing(8);

    dropIcon_ = new QLabel("📄");
    dropIcon_->setObjectName("dropIcon");
    dropIcon_->setAlignment(Qt::AlignCenter);

    dropLabel_ = new QLabel("Drop image files here");
    dropLabel_->setObjectName("dropLabel");
    dropLabel_->setAlignment(Qt::AlignCenter);

    dropSubLabel_ = new QLabel(
        "JPG  PNG  BMP  TIFF  WEBP\n"
        "DOCX  TXT  RTF  PPTX  XLSX  HTML  → PDF"
    );
    dropSubLabel_->setObjectName("dropSubLabel");
    dropSubLabel_->setAlignment(Qt::AlignCenter);

    dz->addWidget(dropIcon_);
    dz->addWidget(dropLabel_);
    dz->addWidget(dropSubLabel_);
}

void MainWindow::setupBottomBar()
{
    bottomBar_ = new QFrame();

    QHBoxLayout* bb = new QHBoxLayout(bottomBar_);
    bb->setContentsMargins(0, 8, 0, 0);

    statusLabel_ = new QLabel("0 files added");
    statusLabel_->setObjectName("statusLabel");

    btnClear_ = new QPushButton("Clear");
    btnClear_->setObjectName("btnSecondary");
    btnClear_->setCursor(Qt::PointingHandCursor);
    btnClear_->setFixedWidth(100);

    btnStart_ = new QPushButton("Convert to PDF");
    btnStart_->setObjectName("btnPrimary");
    btnStart_->setCursor(Qt::PointingHandCursor);
    btnStart_->setFixedWidth(150);

    bb->addWidget(statusLabel_);
    bb->addStretch();
    bb->addWidget(btnClear_);
    bb->addSpacing(8);
    bb->addWidget(btnStart_);

    connect(btnClear_, &QPushButton::clicked,
        this, &MainWindow::onClearClicked);
    connect(btnStart_, &QPushButton::clicked,
        this, &MainWindow::onStartClicked);
}

void MainWindow::applyStyles()
{
    setStyleSheet(R"(
        QMainWindow {
            background-color: #f5f5f5;
        }
        #sidebar {
            background-color: #ffffff;
            border-right: 1px solid #e0e0e0;
        }
        #appName {
            font-size: 20px;
            font-weight: bold;
            color: #1a1a1a;
        }
        #appVersion {
            font-size: 11px;
            color: #999999;
        }
        QPushButton#navButton {
            text-align: left;
            padding: 10px 16px;
            border: none;
            border-radius: 8px;
            font-size: 13px;
            color: #444444;
            background-color: transparent;
        }
        QPushButton#navButton:hover {
            background-color: #f0f0f0;
        }
        QPushButton#navButton:checked {
            background-color: #e8f0fe;
            color: #1a73e8;
            font-weight: bold;
        }
        #mainArea {
            background-color: #f5f5f5;
        }
        #pageTitle {
            font-size: 22px;
            font-weight: bold;
            color: #1a1a1a;
        }
        #pageSubtitle {
            font-size: 13px;
            color: #666666;
        }
        #dropZone {
            background-color: #ffffff;
            border: 2px dashed #d0d0d0;
            border-radius: 12px;
        }
        #dropZone:hover {
            border-color: #1a73e8;
            background-color: #f0f6ff;
        }
        #dropIcon {
            font-size: 36px;
        }
        #dropLabel {
            font-size: 15px;
            font-weight: bold;
            color: #333333;
        }
        #dropSubLabel {
            font-size: 12px;
            color: #999999;
            letter-spacing: 1px;
        }
        #fileList {
            background-color: #ffffff;
            border: 1px solid #e0e0e0;
            border-radius: 8px;
            font-size: 13px;
            color: #333333;
            padding: 4px;
        }
        #fileList::item {
            padding: 8px 12px;
            border-radius: 6px;
        }
        #fileList::item:hover {
            background-color: #f0f6ff;
        }
        #fileList::item:selected {
            background-color: #e8f0fe;
            color: #1a73e8;
        }
        #btnPrimary {
            background-color: #1a73e8;
            color: #ffffff;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: bold;
        }
        #btnPrimary:hover    { background-color: #1558b0; }
        #btnPrimary:pressed  { background-color: #0f3f7a; }
        #btnPrimary:disabled { background-color: #a0c0f0; }
        #btnSecondary {
            background-color: #ffffff;
            color: #444444;
            border: 1px solid #d0d0d0;
            border-radius: 8px;
            padding: 10px 20px;
            font-size: 14px;
        }
        #btnSecondary:hover { background-color: #f5f5f5; }
        #statusLabel {
            font-size: 13px;
            color: #888888;
        }
        QProgressBar#progressBar {
            background-color: #e0e0e0;
            border: none;
            border-radius: 3px;
        }
        QProgressBar#progressBar::chunk {
            background-color: #1a73e8;
            border-radius: 3px;
        }
        QStatusBar {
            background-color: #ffffff;
            border-top: 1px solid #e0e0e0;
            font-size: 12px;
            color: #888888;
        }
    )");
}

// ── Drag & Drop ──────────────────────────────────────────

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QList<QUrl> urls = event->mimeData()->urls();

    QStringList allowed = {
        "jpg","jpeg","png","bmp","tiff","tif","webp",
        "docx","doc","odt","rtf","txt",
        "pptx","ppt","odp",
        "xlsx","xls","ods",
        "html","htm"
    };

    int added = 0;
    for (const QUrl& url : urls) {
        QString path = url.toLocalFile();
        if (path.isEmpty()) continue;

        QFileInfo info(path);
        QString   ext = info.suffix().toLower();

        if (!allowed.contains(ext)) {
            statusBar()->showMessage(
                QString("Skipped %1 — not a supported format")
                .arg(info.fileName()));
            continue;
        }

        QString display = QString("%1   %2   %3 KB")
            .arg(info.fileName(), -40)
            .arg(ext.toUpper(), -6)
            .arg(info.size() / 1024);

        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, path);
        fileList_->addItem(item);
        added++;
    }

    int count = fileList_->count();
    statusLabel_->setText(QString("%1 file%2 added")
        .arg(count).arg(count == 1 ? "" : "s"));

    if (added > 0)
        statusBar()->showMessage(
            QString("Added %1 file%2 — click Convert to PDF")
            .arg(added).arg(added == 1 ? "" : "s"));
}

// ── Engine Slots ─────────────────────────────────────────

void MainWindow::onJobStarted(int jobId, QString fileName)
{
    Q_UNUSED(jobId)
        statusBar()->showMessage(
            QString("Converting %1...").arg(fileName));
    progressBar_->setVisible(true);
}

void MainWindow::onJobProgress(int jobId, float progress)
{
    Q_UNUSED(jobId)
        progressBar_->setValue(static_cast<int>(progress * 100));
}

void MainWindow::onJobFinished(int jobId, bool success,
    QString message)
{
    Q_UNUSED(jobId)
        if (!success)
            statusBar()->showMessage(
                QString("Failed: %1").arg(message));
}

void MainWindow::onAllJobsDone()
{
    progressBar_->setValue(100);
    progressBar_->setVisible(false);
    btnStart_->setEnabled(true);
    btnStart_->setText("Convert to PDF");
    statusBar()->showMessage(
        "All done! PDF files saved next to original files.");
    statusLabel_->setText(
        QString("%1 file%2 converted")
        .arg(fileList_->count())
        .arg(fileList_->count() == 1 ? "" : "s"));
}

// ── Button Slots ─────────────────────────────────────────

void MainWindow::onConvertClicked()
{
    btnConvert_->setChecked(true);
    btnHistory_->setChecked(false);
    btnSettings_->setChecked(false);
}

void MainWindow::onHistoryClicked()
{
    btnConvert_->setChecked(false);
    btnHistory_->setChecked(true);
    btnSettings_->setChecked(false);
    statusBar()->showMessage("History — coming soon");
}

void MainWindow::onSettingsClicked()
{
    btnConvert_->setChecked(false);
    btnHistory_->setChecked(false);
    btnSettings_->setChecked(true);
    statusBar()->showMessage("Settings — coming soon");
}

void MainWindow::onClearClicked()
{
    fileList_->clear();
    progressBar_->setValue(0);
    progressBar_->setVisible(false);
    statusLabel_->setText("0 files added");
    statusBar()->showMessage(
        "Ready — drop image files to convert to PDF");
}

void MainWindow::onStartClicked()
{
    if (fileList_->count() == 0) {
        statusBar()->showMessage(
            "No files added — drop some files first");
        return;
    }

    if (engine_->isRunning()) {
        statusBar()->showMessage(
            "Already converting — please wait");
        return;
    }

    btnStart_->setEnabled(false);
    btnStart_->setText("Converting...");
    progressBar_->setValue(0);
    progressBar_->setVisible(true);

    for (int i = 0; i < fileList_->count(); ++i) {
        auto* item = fileList_->item(i);
        QString path = item->data(Qt::UserRole).toString();

        if (path.isEmpty()) continue;

        zc::ConversionJob job;
        job.inputPath = path;
        job.targetFormat = "pdf";
        job.quality = 95;

        engine_->submitJob(job);
    }
}

// ── addFile (called from main when launched with a path) ─

void MainWindow::addFile(const QString& path)
{
    QFileInfo info(path);
    if (!info.exists()) return;

    QString ext = info.suffix().toLower();
    QStringList allowed = {
        "jpg","jpeg","png","bmp","tiff","tif","webp",
        "docx","doc","odt","rtf","txt",
        "pptx","ppt","odp",
        "xlsx","xls","ods",
        "html","htm"
    };
    if (!allowed.contains(ext)) return;

    QString display = QString("%1   %2   %3 KB")
        .arg(info.fileName(), -40)
        .arg(ext.toUpper(), -6)
        .arg(info.size() / 1024);

    auto* item = new QListWidgetItem(display);
    item->setData(Qt::UserRole, path);
    fileList_->addItem(item);

    int count = fileList_->count();
    statusLabel_->setText(QString("%1 file%2 added")
        .arg(count).arg(count == 1 ? "" : "s"));
}