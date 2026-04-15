#include "qr_code_generator.h"
#include <QByteArray>
#include "third_party/libqrencode/qrencode_compat.h"

namespace QrCodeGenerator {

QrCodeData GenerateQrCode(const QString& url) {
  QrCodeData result;
  result.size = 0;

  const QByteArray encoded_url = url.trimmed().toUtf8();
  if (encoded_url.isEmpty()) {
    return result;
  }

  QRcode* qr_code = QRcode_encodeData(
      encoded_url.size(),
      reinterpret_cast<const unsigned char*>(encoded_url.constData()), 0,
      QR_ECLEVEL_M);
  
  if (qr_code == nullptr || qr_code->data == nullptr || qr_code->width <= 0) {
    if (qr_code != nullptr) {
      QRcode_free(qr_code);
    }
    return result;
  }

  result.size = qr_code->width;
  result.rows.reserve(result.size);

  for (int row = 0; row < result.size; ++row) {
    QString row_data;
    row_data.reserve(result.size);

    for (int col = 0; col < result.size; ++col) {
      const unsigned char module =
          qr_code->data[row * result.size + col] & 0x1;
      row_data.append(module ? QLatin1Char('1') : QLatin1Char('0'));
    }

    result.rows.append(row_data);
  }

  QRcode_free(qr_code);
  return result;
}

}  // namespace QrCodeGenerator
