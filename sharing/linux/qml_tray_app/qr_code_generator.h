#ifndef SHARING_LINUX_QML_TRAY_APP_QR_CODE_GENERATOR_H_
#define SHARING_LINUX_QML_TRAY_APP_QR_CODE_GENERATOR_H_

#include <QString>
#include <QStringList>

namespace QrCodeGenerator {

struct QrCodeData {
  QStringList rows;
  int size;
};

QrCodeData GenerateQrCode(const QString& url);

}  // namespace QrCodeGenerator

#endif  // SHARING_LINUX_QML_TRAY_APP_QR_CODE_GENERATOR_H_
