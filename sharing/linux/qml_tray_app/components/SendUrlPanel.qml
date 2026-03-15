import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    property string urlText: fileShareController.qrCodeUrl
    property var qrRows: fileShareController.qrCodeRows
    property int qrSize: fileShareController.qrCodeSize
    property string fileName: fileShareController.pendingSendFileName
    readonly property color panelTint: "#ecfdf3"
    readonly property color panelBorder: "#a7f3d0"
    readonly property color qrPaper: "#fffdf7"
    readonly property color qrInk: "#14532d"
    readonly property color accentSoft: "#d1fae5"
    readonly property color accentStrong: "#34d399"
    readonly property bool compact: width < 360
    readonly property real qrFrameSize: 360
    readonly property real qrInnerSize: qrFrameSize - (compact ? 34 : 42)

    spacing: compact ? 14 : 18
    implicitWidth: 420

    Label {
        Layout.alignment: Qt.AlignHCenter
        text: "Scan to connect"
        font.pixelSize: compact ? 16 : 18
        font.weight: Font.DemiBold
        color: "#111827"
    }

    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        width: root.qrFrameSize
        height: width
        radius: compact ? 24 : 32
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#f7fff9" }
            GradientStop { position: 1.0; color: root.panelTint }
        }
        border.color: root.panelBorder
        border.width: 1

        Rectangle {
            width: parent.width * 0.52
            height: width
            radius: width / 2
            x: parent.width - width * 0.72
            y: -width * 0.22
            color: "#ffffff"
            opacity: 0.35
        }


        Rectangle {
            anchors.centerIn: parent
            width: root.qrInnerSize
            height: width
            radius: compact ? 18 : 24
            color: root.qrPaper
            border.color: "#dcfce7"
            border.width: 1

            Canvas {
                id: qrCanvas
                anchors.fill: parent
                anchors.margins: compact ? 16 : 22
                antialiasing: true
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.fillStyle = root.qrPaper
                    ctx.fillRect(0, 0, width, height)

                    if (root.qrSize <= 0 || !root.qrRows || root.qrRows.length !== root.qrSize)
                        return

                    var quietZone = 4
                    var totalModules = root.qrSize + quietZone * 2

                    var moduleSize = Math.min(width, height) / totalModules
                    var drawSize = moduleSize * totalModules
                    var offsetX = (width - drawSize) / 2
                    var offsetY = (height - drawSize) / 2
                    var dotInset = moduleSize * 0.18
                    var dotSize = Math.max(1, moduleSize - dotInset * 2)
                    var dotRadius = dotSize/1.2

                    ctx.fillStyle = root.qrInk
                    for (var row = 0; row < root.qrSize; ++row) {
                        var rowData = root.qrRows[row]
                        for (var col = 0; col < root.qrSize; ++col) {
                            if (rowData.charAt(col) !== "1")
                                continue

                            var dotX = offsetX + (col + quietZone) * moduleSize + dotInset
                            var dotY = offsetY + (row + quietZone) * moduleSize + dotInset

                            ctx.beginPath()
                            ctx.arc(dotX + dotRadius, dotY + dotRadius, dotRadius, 0, Math.PI * 2)
                            ctx.fill()
                        }
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: root.qrSize <= 0
                text: "Preparing QR code..."
                font.pixelSize: compact ? 12 : 13
                color: "#6b7280"
            }
        }
    }

    Label {
        Layout.alignment: Qt.AlignHCenter
        text: root.fileName.length > 0 ? "Sending: " + root.fileName : ""
        font.pixelSize: compact ? 12 : 13
        color: "#6b7280"
        visible: text.length > 0
    }

    onQrRowsChanged: qrCanvas.requestPaint()
    onQrSizeChanged: qrCanvas.requestPaint()
}
