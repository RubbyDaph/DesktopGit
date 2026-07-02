#pragma once

#include <QString>

class GitDiffFormatter
{
public:
    [[nodiscard]] static QString FormatForDisplay(const QString &diff);
};
