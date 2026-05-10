#include "CommandLineParser.h"

namespace zc
{
    ParsedCommand CommandLineParser::parse(const QStringList& args)
    {
        ParsedCommand cmd;

        if (args.contains("--convert-pdf"))
        {
            cmd.convertToPdf = true;
        }

        for (const QString& arg : args)
        {
            if (!arg.startsWith("--"))
            {
                cmd.files.append(arg);
            }
        }

        return cmd;
    }
}