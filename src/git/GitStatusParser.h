#pragma once

#include "GitStatusFile.h"

#include <QList>
#include <QString>

class GitStatusParser
{
public:
    [[nodiscard]] static QList<GitStatusFile> Parse(const QString &output);
};
