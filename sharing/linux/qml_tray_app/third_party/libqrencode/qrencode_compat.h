#ifndef SHARING_LINUX_QML_TRAY_APP_THIRD_PARTY_LIBQRENCODE_QRENCODE_COMPAT_H_
#define SHARING_LINUX_QML_TRAY_APP_THIRD_PARTY_LIBQRENCODE_QRENCODE_COMPAT_H_

// Minimal libqrencode ABI used by this app when the runtime shared library is
// available but the development headers are not installed.

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
  QR_ECLEVEL_L = 0,
  QR_ECLEVEL_M,
  QR_ECLEVEL_Q,
  QR_ECLEVEL_H
} QRecLevel;

typedef struct {
  int version;
  int width;
  unsigned char* data;
} QRcode;

QRcode* QRcode_encodeData(int size, const unsigned char* data, int version,
                          QRecLevel level);
void QRcode_free(QRcode* qrcode);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // SHARING_LINUX_QML_TRAY_APP_THIRD_PARTY_LIBQRENCODE_QRENCODE_COMPAT_H_
