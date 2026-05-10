#pragma once
#include <QString>
#include <QStringList>
#include <functional>
#include "ConversionJob.h"

namespace zc {

    // Progress callback: value 0.0 to 1.0
    using ProgressCallback = std::function<void(int jobId, float progress)>;

    // Every converter plugin implements this interface
    class IConverter {
    public:
        virtual ~IConverter() = default;

        // Human readable name e.g. "PDF Converter"
        virtual QString name() const = 0;

        // List of input extensions this handles e.g. {"pdf","docx"}
        virtual QStringList supportedInputFormats() const = 0;

        // List of output extensions this can produce
        virtual QStringList supportedOutputFormats() const = 0;

        // Quick check before running
        virtual bool canConvert(const QString& fromExt,
            const QString& toExt) const = 0;

        // Do the actual conversion — runs on a worker thread
        // Returns true on success, false on failure
        virtual bool convert(ConversionJob& job,
            ProgressCallback progress) = 0;
    };

} // namespace zc