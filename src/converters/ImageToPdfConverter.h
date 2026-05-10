#pragma once
#include "engine/IConverter.h"

namespace zc {

    // Converts image files to PDF using Qt PrintSupport
    // Supports: JPG, JPEG, PNG, BMP, TIFF, TIF, WEBP → PDF
    class ImageToPdfConverter : public IConverter
    {
    public:
        ImageToPdfConverter() = default;
        ~ImageToPdfConverter() override = default;

        QString     name()                   const override;
        QStringList supportedInputFormats()  const override;
        QStringList supportedOutputFormats() const override;

        bool canConvert(const QString& fromExt,
            const QString& toExt)  const override;

        bool convert(ConversionJob& job,
            ProgressCallback progress) override;

    private:
        bool imageFileToPdf(const QString& inputPath,
            const QString& outputPath,
            int            quality);
    };

} // namespace zc