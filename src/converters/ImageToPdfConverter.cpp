#include "ImageToPdfConverter.h"
#include <QImage>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QMarginsF>

namespace zc {

    static const QStringList kSupportedImages = {
        "jpg", "jpeg", "png", "bmp",
        "tiff", "tif", "webp"
    };

    QString ImageToPdfConverter::name() const
    {
        return "Image to PDF Converter";
    }

    QStringList ImageToPdfConverter::supportedInputFormats() const
    {
        return kSupportedImages;
    }

    QStringList ImageToPdfConverter::supportedOutputFormats() const
    {
        return { "pdf" };
    }

    bool ImageToPdfConverter::canConvert(const QString& fromExt,
        const QString& toExt) const
    {
        QString from = fromExt.toLower();
        QString to = toExt.toLower();
        if (from == "jpeg") from = "jpg";
        return kSupportedImages.contains(from) && to == "pdf";
    }

    bool ImageToPdfConverter::convert(ConversionJob& job,
        ProgressCallback progress)
    {
        qDebug() << "[ImageToPdf] Converting"
            << job.inputPath << "-> PDF";

        progress(job.id, 0.0f);

        // Build output path if not provided
        if (job.outputPath.isEmpty()) {
            QFileInfo info(job.inputPath);
            job.outputPath = info.dir().filePath(
                info.completeBaseName() + ".pdf"
            );
        }

        // Make sure output directory exists
        QDir().mkpath(QFileInfo(job.outputPath).absolutePath());

        progress(job.id, 0.2f);

        bool ok = imageFileToPdf(job.inputPath,
            job.outputPath,
            job.quality);

        if (!ok) {
            job.errorMessage = QString("Failed to convert: %1")
                .arg(job.inputPath);
            return false;
        }

        job.outputBytes = QFileInfo(job.outputPath).size();
        progress(job.id, 1.0f);

        qDebug() << "[ImageToPdf] Done ->" << job.outputPath;
        return true;
    }

    bool ImageToPdfConverter::imageFileToPdf(const QString& inputPath,
        const QString& outputPath,
        int            quality)
    {
        Q_UNUSED(quality)

            // Load image
            QImage image(inputPath);
        if (image.isNull()) {
            qDebug() << "[ImageToPdf] Cannot load image:" << inputPath;
            return false;
        }

        // Create PDF writer
        QPdfWriter writer(outputPath);
        writer.setCreator("zconverter");
        writer.setTitle(QFileInfo(inputPath).completeBaseName());

        // Set page size to match image proportions (A4 default)
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setPageMargins(QMarginsF(0, 0, 0, 0));

        // Resolution
        writer.setResolution(300);

        // Paint image onto PDF page
        QPainter painter(&writer);
        if (!painter.isActive()) {
            qDebug() << "[ImageToPdf] Cannot create painter";
            return false;
        }

        // Scale image to fill the page
        QRect pageRect = painter.viewport();
        QImage scaled = image.scaled(
            pageRect.size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        // Center on page
        int x = (pageRect.width() - scaled.width()) / 2;
        int y = (pageRect.height() - scaled.height()) / 2;

        painter.drawImage(x, y, scaled);
        painter.end();

        return true;
    }

} // namespace zc