#include <QApplication>
#include <QCoreApplication>
#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QStringList>
#include <memory>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

#include "ui/MainWindow.h"

#include "engine/ConversionEngine.h"
#include "engine/ConversionJob.h"

#include "converters/ImageToPdfConverter.h"
#include "converters/OfficeToPdfConverter.h"

// ─────────────────────────────────────────────
// CLI STRUCT
// ─────────────────────────────────────────────
struct CliOptions
{
    QStringList files;
    QString targetFormat = "pdf";
    bool valid = false;
};

namespace
{
    // ─────────────────────────────────────────────
    // Windows toast notification (headless mode)
    // ─────────────────────────────────────────────
    void showNotification(const QString& title, const QString& msg)
    {
#ifdef Q_OS_WIN
        NOTIFYICONDATAW nid = {};
        nid.cbSize = sizeof(nid);
        nid.uFlags = NIF_INFO;
        nid.dwInfoFlags = NIIF_INFO;
        nid.uTimeout = 4000;

        wcsncpy_s(nid.szInfoTitle,
            title.toStdWString().c_str(), 63);

        wcsncpy_s(nid.szInfo,
            msg.toStdWString().c_str(), 255);

        Shell_NotifyIconW(NIM_ADD, &nid);
        Shell_NotifyIconW(NIM_MODIFY, &nid);
        Shell_NotifyIconW(NIM_DELETE, &nid);
#else
        Q_UNUSED(title)
            Q_UNUSED(msg)
#endif
    }

    // ─────────────────────────────────────────────
    // Engine setup
    // ─────────────────────────────────────────────
    std::shared_ptr<zc::ConversionEngine> createEngine()
    {
        auto engine = std::make_shared<zc::ConversionEngine>();

        engine->registerConverter(
            std::make_shared<zc::ImageToPdfConverter>());

        engine->registerConverter(
            std::make_shared<zc::OfficeToPdfConverter>());

        return engine;
    }

    // ─────────────────────────────────────────────
    // HEADLESS MODE
    // ─────────────────────────────────────────────
    int runHeadless(
        int argc,
        char* argv[],
        const QStringList& files,
        const QString& targetFormat)
    {
        QCoreApplication coreApp(argc, argv);

        if (files.isEmpty())
        {
            qWarning() << "[zconverter] No input files";
            return 1;
        }

        auto engine = createEngine();

        int totalJobs = 0;
        int completedJobs = 0;
        int failedJobs = 0;
        bool finished = false;

        QObject::connect(engine.get(),
            &zc::ConversionEngine::jobFinished,
            [&](int, bool success, QString msg)
            {
                completedJobs++;

                if (!success)
                {
                    failedJobs++;
                    qWarning() << "[zconverter] Failed:" << msg;
                }
            });

        QObject::connect(engine.get(),
            &zc::ConversionEngine::allJobsDone,
            [&]() { finished = true; });

        // ─────────────────────────────────────────
        // Submit jobs
        // ─────────────────────────────────────────
        for (const QString& inputPath : files)
        {
            QFileInfo info(inputPath);

            if (!info.exists())
            {
                qWarning() << "[zconverter] Missing file:" << inputPath;
                failedJobs++;
                continue;
            }

            QString outputPath =
                info.dir().filePath(
                    info.completeBaseName() + "." + targetFormat);

            zc::ConversionJob job;
            job.inputPath = inputPath;
            job.outputPath = outputPath;
            job.targetFormat = targetFormat.toLower();
            job.quality = 95;

            engine->submitJob(job);
            totalJobs++;
        }

        if (totalJobs == 0)
            return 1;

        // Timeout safety
        QTimer timeout;
        timeout.setSingleShot(true);

        QObject::connect(&timeout, &QTimer::timeout, [&]()
            {
                qWarning() << "[zconverter] Timeout reached";
                finished = true;
            });

        timeout.start(60000);

        while (!finished)
            coreApp.processEvents(QEventLoop::AllEvents, 100);

        // Notification
        if (failedJobs == 0)
        {
            showNotification("zconverter",
                QString("Converted %1 file(s)").arg(completedJobs));
        }
        else
        {
            showNotification("zconverter",
                QString("%1 success, %2 failed")
                .arg(completedJobs - failedJobs)
                .arg(failedJobs));
        }

        return (failedJobs == 0) ? 0 : 1;
    }
    // ─────────────────────────────────────────────
    // CLI PARSER (NEW + OLD)
    // ─────────────────────────────────────────────
    CliOptions parseCli(int argc, char* argv[])
    {
        CliOptions opt;

        for (int i = 1; i < argc; ++i)
        {
            QString arg = QString::fromLocal8Bit(argv[i]);

            // NEW SYSTEM: --to format files...
            if (arg == "--to" && i + 1 < argc)
            {
                opt.targetFormat =
                    QString::fromLocal8Bit(argv[i + 1]).toLower();

                for (int j = i + 2; j < argc; ++j)
                {
                    QString fileArg = QString::fromLocal8Bit(argv[j]);

                    if (!fileArg.startsWith("--"))
                        opt.files.append(fileArg);
                }

                opt.valid = !opt.files.isEmpty();
                return opt;
            }

            // OLD SYSTEM: --convert files...
            if (arg == "--convert")
            {
                opt.targetFormat = "pdf";

                for (int j = i + 1; j < argc; ++j)
                {
                    QString fileArg = QString::fromLocal8Bit(argv[j]);

                    if (!fileArg.startsWith("--"))
                        opt.files.append(fileArg);
                }

                opt.valid = !opt.files.isEmpty();
                return opt;
            }
        }

        return opt;
    }
// ─────────────────────────────────────────────
// MAIN
// ─────────────────────────────────────────────
int main(int argc, char* argv[])
{
    // ─────────────────────────────────────────
    // CLI MODE
    // ─────────────────────────────────────────
    CliOptions opt = parseCli(argc, argv);

    if (opt.valid)
    {
        return runHeadless(argc, argv, opt.files, opt.targetFormat);
    }

    // ─────────────────────────────────────────
    // GUI MODE
    // ─────────────────────────────────────────
    QApplication app(argc, argv);

    app.setApplicationName("zconverter");
    app.setApplicationVersion("1.0.0");

    auto engine = createEngine();

    qDebug() << "[zconverter] Engine ready";

    MainWindow window(engine);

    for (int i = 1; i < argc; ++i)
    {
        QFileInfo info(QString::fromLocal8Bit(argv[i]));

        if (info.exists())
            window.addFile(info.absoluteFilePath());
    }

    window.show();

    return app.exec();
}