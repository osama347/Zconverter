#pragma once
#include <QString>
#include <QFileInfo>

namespace zc {

    // The status of a single conversion job
    enum class JobStatus {
        Pending,
        Running,
        Done,
        Failed,
        Cancelled
    };

    // Describes one file conversion task
    struct ConversionJob {
        int         id = 0;
        QString     inputPath;
        QString     outputPath;
        QString     targetFormat;   // e.g. "pdf", "mp4", "png"
        int         quality = 85;
        bool        removeMetadata = false;
        JobStatus   status = JobStatus::Pending;
        QString     errorMessage;
        qint64      inputBytes = 0;
        qint64      outputBytes = 0;
        double      durationMs = 0.0;

        // Convenience: get just the filename
        QString inputFileName() const {
            return QFileInfo(inputPath).fileName();
        }

        // Convenience: get input extension
        QString inputExtension() const {
            return QFileInfo(inputPath).suffix().toLower();
        }
    };

} // namespace zc