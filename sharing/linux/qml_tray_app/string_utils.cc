#include "string_utils.h"

namespace StringUtils {

QString TrimmedOrFallback(const QString& value, const QString& fallback) {
  const QString trimmed = value.trimmed();
  return trimmed.isEmpty() ? fallback : trimmed;
}

QString FromStdString(const std::string& value) {
  return QString::fromStdString(value);
}

QString TrimmedFromStdString(const std::string& value) {
  return QString::fromStdString(value).trimmed();
}

}  // namespace StringUtils
