#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QFrame>
#include <QStatusBar>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QProgressBar>
#include <memory>
#include "engine/ConversionEngine.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(
        std::shared_ptr<zc::ConversionEngine> engine,
        QWidget* parent = nullptr);
    ~MainWindow() override = default;

    // Called by main() when launched with a file path argument
    void addFile(const QString& path);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event)           override;

private:
    std::shared_ptr<zc::ConversionEngine> engine_;

    // Layout
    QWidget* centralWidget_;
    QHBoxLayout* rootLayout_;

    // Sidebar
    QFrame* sidebar_;
    QVBoxLayout* sidebarLayout_;
    QPushButton* btnConvert_;
    QPushButton* btnHistory_;
    QPushButton* btnSettings_;

    // Main area
    QFrame* mainArea_;
    QVBoxLayout* mainAreaLayout_;

    // Drop zone
    QFrame* dropZone_;
    QLabel* dropIcon_;
    QLabel* dropLabel_;
    QLabel* dropSubLabel_;

    // File list
    QListWidget* fileList_;

    // Progress
    QProgressBar* progressBar_;

    // Bottom bar
    QFrame* bottomBar_;
    QLabel* statusLabel_;
    QPushButton* btnClear_;
    QPushButton* btnStart_;

    void setupUI();
    void setupSidebar();
    void setupMainArea();
    void setupDropZone();
    void setupBottomBar();
    void applyStyles();

private slots:
    void onConvertClicked();
    void onHistoryClicked();
    void onSettingsClicked();
    void onClearClicked();
    void onStartClicked();
    void onJobStarted(int jobId, QString fileName);
    void onJobProgress(int jobId, float   progress);
    void onJobFinished(int jobId, bool    success, QString message);
    void onAllJobsDone();
};