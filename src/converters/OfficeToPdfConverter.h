#pragma once
#include "engine/IConverter.h"

namespace zc {

    // Converts Office documents to PDF using LibreOffice headless
    // Supports: DOCX, DOC, TXT, RTF, ODT, PPTX, PPT, ODP,
    //           XLSX, XLS, ODS, HTML → PDF
    class OfficeToPdfConverter : public IConverter
    {
    public:
        OfficeToPdfConverter() = default;
        ~OfficeToPdfConverter() override = default;

        QString     name()                   const override;
        QStringList supportedInputFormats()  const override;
        QStringList supportedOutputFormats() const override;

        bool canConvert(const QString& fromExt,
            const QString& toExt)  const override;

        bool convert(ConversionJob& job,
            ProgressCallback progress) override;

    private:
        // Path to soffice.exe
        static QString libreOfficePath();

        // Run the actual LibreOffice conversion
        bool runLibreOffice(const QString& inputPath,
            const QString& outputDir,
            int            timeoutMs = 60000);
    };

} // namespace zc