#pragma once

#include <vector>
#include <QString>

inline const QString translatorsCommentPrefix = "TRANSLATORS:";
std::vector<int> parseReferences(const QString& inStr);
QString formatReferences(const std::vector<int>& refs);
QString jsonEscape(const QString& string);
