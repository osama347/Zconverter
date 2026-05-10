#include "OfficeToPdfConverter.h"
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>

namespace zc {

    static const QStringList kSupportedFormats = {
        // Word documents
        "docx", "doc", "odt", "rtf", "txt",
        // Presentations
        "pptx", "ppt", "odp",
        // Spreadsheets
        "xlsx", "xls", "ods",
        // Web
        "html", "htm"
    };

    QString OfficeToPdfConverter::name() const
    {
        return "Office to PDF Converter (LibreOffice)";
    }

    QStringList OfficeToPdfConverter::supportedInputFormats() const
    {
        return kSupportedFormats;
    }

    QStringList OfficeToPdfConverter::supportedOutputFormats() const
    {
        return { "pdf" };
    }

    bool OfficeToPdfConverter::canConvert(const QString& fromExt,
        const QString& toExt) const
    {
        return kSupportedFormats.contains(fromExt.toLower())
            && toExt.toLower() == "pdf";
    }

    QString OfficeToPdfConverter::libreOfficePath()
    {
        // Primary install location
        QString path = "C:/Program Files/LibreOffice/program/soffice.exe";
        if (QFileInfo::exists(path)) return path;

        // x86 fallback
        path = "C:/Program Files (x86)/LibreOffice/program/soffice.exe";
        if (QFileInfo::exists(path)) return path;

        return {};
    }

    bool OfficeToPdfConverter::runLibreOffice(const QString& inputPath,
        const QString& outputDir,
        int            timeoutMs)
    {
        QString soffice = libreOfficePath();
        if (soffice.isEmpty()) {
            qDebug() << "[OfficePdf] soffice.exe not found";
            return false;
        }

        // Build arguments
        // --headless        : no GUI
        // --convert-to pdf  : output format
        // --outdir          : where to put the PDF
        QStringList args;
        args << "--headless"
            << "--norestore"
            << "--nofirststartwizard"
            << "--convert-to" << "pdf"
            << "--outdir" << QDir::toNativeSeparators(outputDir)
            << QDir::toNativeSeparators(inputPath);

        qDebug() << "[OfficePdf] Running:" << soffice << args;

        QProcess process;
        process.setProgram(soffice);
        process.setArguments(args);

        // Merge stdout + stderr so we see all output
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start();

        if (!process.waitForStarted(5000)) {
            qDebug() << "[OfficePdf] Failed to start LibreOffice";
            return false;
        }

        if (!process.waitForFinished(timeoutMs)) {
            qDebug() << "[OfficePdf] LibreOffice timed out";
            process.kill();
            return false;
        }

        QString output = process.readAllStandardOutput();
        qDebug() << "[OfficePdf] Output:" << output;

        int exitCode = process.exitCode();
        qDebug() << "[OfficePdf] Exit code:" << exitCode;

        return exitCode == 0;
    }

    bool OfficeToPdfConverter::convert(ConversionJob& job,
        ProgressCallback progress)
    {
        qDebug() << "[OfficePdf] Converting"
            << job.inputPath << "-> PDF";

        progress(job.id, 0.0f);

        // Verify LibreOffice exists
        if (libreOfficePath().isEmpty()) {
            job.errorMessage =
                "LibreOffice not found. "
                "Please install from libreoffice.org";
            return false;
        }

        progress(job.id, 0.1f);

        // LibreOffice outputs the PDF in the same folder as input
        // unless we specify --outdir
        // We use the input file's folder as output dir
        QFileInfo inputInfo(job.inputPath);
        QString   outputDir = inputInfo.absolutePath();

        // If custom output path requested, use its folder
        if (!job.outputPath.isEmpty()) {
            outputDir = QFileInfo(job.outputPath).absolutePath();
            QDir().mkpath(outputDir);
        }

        progress(job.id, 0.2f);

        // Run LibreOffice — this takes a few seconds
        bool ok = runLibreOffice(job.inputPath, outputDir);

        if (!ok) {
            job.errorMessage =
                QString("LibreOffice failed to convert: %1")
                .arg(inputInfo.fileName());
            return false;
        }

        progress(job.id, 0.9f);

        // LibreOffice names the output file as:
        // originalname.pdf in the outputDir
        QString expectedPdf = outputDir + "/" +
            inputInfo.completeBaseName() + ".pdf";

        if (!QFileInfo::exists(expectedPdf)) {
            job.errorMessage =
                QString("Output PDF not found: %1").arg(expectedPdf);
            return false;
        }

        // If a specific output path was requested, rename/move
        if (!job.outputPath.isEmpty() &&
            job.outputPath != expectedPdf)
        {
            // Remove existing file at destination if any
            QFile::remove(job.outputPath);
            QFile::rename(expectedPdf, job.outputPath);
            job.outputBytes = QFileInfo(job.outputPath).size();
        }
        else {
            job.outputPath = expectedPdf;
            job.outputBytes = QFileInfo(expectedPdf).size();
        }

        progress(job.id, 1.0f);

        qDebug() << "[OfficePdf] Done ->" << job.outputPath;
        return true;
    }

} // namespace zc