import QtQuick
import QtQuick.Controls

Item {
    anchors.fill: parent

    readonly property color textPrimary: "#111827"
    readonly property color textMuted: "#6b7280"

    Label {
        x: 48; y: 48
        visible: fileShareController.running
        text: "Ready to receive" + (fileShareController.pendingSendFilePath.length > 0 ? " / send" : "")
        font.pixelSize: 20
        font.weight: Font.Medium
        color: textPrimary
    }

    Canvas {
        id: blobCanvas
        width: 380; height: 380
        anchors.centerIn: parent

        property real t: 0
        property double startMs: Date.now()

        Timer {
            interval: 16
            running: true
            repeat: true
            onTriggered: {
                // seconds -> radians scale (t grows forever, no loop jump)
                blobCanvas.t = (Date.now() - blobCanvas.startMs) * 0.001 * (Math.PI * 2 / 8.0)
                blobCanvas.requestPaint()
            }
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var cx = width / 2
            var cy = height / 2
            var n = 10
            var pts = []

            for (var i = 0; i < n; i++) {
                var a = (i / n) * Math.PI * 2 - Math.PI / 2
                var r = 130
                      + Math.sin(a * 2 + t)          * 11
                      + Math.cos(a * 3 - t * 0.6)    *  8
                      + Math.sin(a * 1.5 + t * 0.35) *  6
                pts.push({ x: cx + Math.cos(a) * r, y: cy + Math.sin(a) * r })
            }

            var len = pts.length
            ctx.beginPath()
            for (var j = 0; j < len; j++) {
                var p0 = pts[(j - 1 + len) % len]
                var p1 = pts[j]
                var p2 = pts[(j + 1) % len]
                var p3 = pts[(j + 2) % len]
                var cp1x = p1.x + (p2.x - p0.x) / 6
                var cp1y = p1.y + (p2.y - p0.y) / 6
                var cp2x = p2.x - (p3.x - p1.x) / 6
                var cp2y = p2.y - (p3.y - p1.y) / 6
                if (j === 0) ctx.moveTo(p1.x, p1.y)
                ctx.bezierCurveTo(cp1x, cp1y, cp2x, cp2y, p2.x, p2.y)
            }
            ctx.closePath()

            var grad = ctx.createRadialGradient(cx - 40, cy - 40, 0, cx, cy, 150)
            grad.addColorStop(0, "#e8faf0")
            ctx.fillStyle = grad
            ctx.fill()
        }
    }

    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 48
        text: fileShareController.statusMessage
        font.pixelSize: 13
        color: textMuted
    }
}
