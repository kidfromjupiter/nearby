#ifndef SHARING_LINUX_QML_TRAY_APP_STRING_UTILS_H_
#define SHARING_LINUX_QML_TRAY_APP_STRING_UTILS_H_

#include <QString>
#include <string>

namespace StringUtils {

QString TrimmedOrFallback(const QString& value, const QString& fallback);

QString FromStdString(const std::string& value);

QString TrimmedFromStdString(const std::string& value);

}  // namespace StringUtils

#endif  // SHARING_LINUX_QML_TRAY_APP_STRING_UTILS_H_
