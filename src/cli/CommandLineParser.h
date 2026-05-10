#pragma once

#include <QStringList>

namespace zc
{
    struct ParsedCommand
    {
        bool convertToPdf = false;
        QStringList files;
    };

    class CommandLineParser
    {
    public:
        static ParsedCommand parse(const QStringList& args);
    };
}