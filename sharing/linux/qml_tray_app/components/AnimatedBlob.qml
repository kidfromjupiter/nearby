import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

Item {
    anchors.fill: parent

    readonly property color textPrimary: "#111827"
    readonly property color textMuted: "#6b7280"
    readonly property color textSoft: "#4b5563"
    readonly property color cardSurface: "#ffffff"
    readonly property color cardBorder: "#d1fae5"
    readonly property bool isSendMode: fileShareController.pendingSendFilePath.length > 0
    readonly property var incomingTransfer: findIncomingTransfer()
    readonly property var incomingTarget: findTargetForTransfer(incomingTransfer)
    readonly property bool hasIncomingTransfer: incomingTransfer !== null
                                               && incomingTarget !== null
    readonly property bool isReceivingActive: hasIncomingTransfer
                                              && String(incomingTransfer.status || "") !== "Complete"
    readonly property real receivingIntensity: {
        if (!isReceivingActive)
            return 0
        var numeric = Number(incomingTransfer.progress)
        if (!isFinite(numeric) || numeric < 0)
            numeric = 0
        return Math.max(0.35, Math.min(1.0, numeric))
    }
    property real blobChaos: receivingIntensity
    property string dismissedTransferKey: ""

    Behavior on blobChaos {
        NumberAnimation {
            duration: 220
            easing.type: Easing.OutCubic
        }
    }

    function isIncomingTransferActive(status) {
        return status === "InProgress"
                || status === "Queued"
                || status === "Connecting"
                || status === "AwaitingLocalConfirmation"
                || status === "AwaitingRemoteAcceptance"
                || status === "Complete"
    }

    function transferKey(transfer) {
        if (!transfer)
            return ""
        return String(transfer.targetId || "")
                + "|" + String(transfer.status || "")
                + "|" + String(transfer.filePath || "")
                + "|" + String(transfer.fileName || "")
    }

    function findIncomingTransfer() {
        var transfers = fileShareController.transfers
        var latestCompleted = null
        for (var i = transfers.length - 1; i >= 0; --i) {
            var entry = transfers[i]
            if (!entry)
                continue
            if (String(entry.direction || "") !== "incoming")
                continue
            var status = String(entry.status || "")
            if (!isIncomingTransferActive(status))
                continue
            if (status === "Complete") {
                if (latestCompleted === null)
                    latestCompleted = entry
                continue
            }
            return entry
        }
        return latestCompleted
    }

    function findTargetForTransfer(transfer) {
        if (!transfer)
            return null
        var targets = fileShareController.discoveredTargets
        for (var i = 0; i < targets.length; ++i) {
            var entry = targets[i]
            if (entry && entry.id === transfer.targetId)
                return entry
        }
        return {
            id: transfer.targetId,
            name: String(transfer.targetName || "Incoming device"),
            isIncoming: true
        }
    }

    function incomingHeadline(status) {
        if (status === "Complete")
            return "Received"
        if (status === "AwaitingLocalConfirmation")
            return "Incoming transfer"
        if (status === "AwaitingRemoteAcceptance")
            return "Preparing transfer"
        if (status === "Connecting")
            return "Connecting"
        return "Receiving"
    }

    // The card only becomes actionable once the file has landed on disk and we
    // have a local path to open.
    function incomingClickReady() {
        return hasIncomingTransfer
                && String(incomingTransfer.status || "") === "Complete"
                && String(incomingTransfer.filePath || "").length > 0
    }

    Label {
        x: 48; y: 48
        visible: fileShareController.running
        text: isSendMode
              ? "Ready to send"
              : "Ready to receive"
        font.pixelSize: 20
        font.weight: Font.Medium
        color: textPrimary
    }
    Canvas {
        id: blobCanvas3
        width: 380; height: 380
        anchors.centerIn: parent
        visible: !isSendMode

        property real t: 0
        property double lastMs: Date.now()

        Timer {
            interval: 16
            running: true
            repeat: true
            onTriggered: {
                var now = Date.now()
                var dt = Math.min(0.05, Math.max(0.0, (now - blobCanvas3.lastMs) * 0.001))
                blobCanvas3.lastMs = now
                blobCanvas3.t += dt * (Math.PI * 2 / Math.max(4.6, 8.0 - blobChaos * 2.6))
                blobCanvas3.requestPaint()
            }
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var chaos = blobChaos
            var cx = width / 2 + Math.sin(t * 0.33) * chaos * 10
            var cy = height / 2 + Math.cos(t * 0.27) * chaos * 8
            var n = 10
            var pts = []

            for (var i = 0; i < n; i++) {
                var a = (i / n) * Math.PI * 2 - Math.PI / 2
                var r = 150 + chaos * 22
                    + Math.sin(a * 2 + t)               * (11 + chaos * 14)
                    + Math.cos(a * 3 - t * 0.2)         * (8 + chaos * 10)
                    + Math.sin(a * 1.5 + t * 0.7)       * (6 + chaos * 8)
                    + Math.sin(a * 1.5 + t)             * (4 + chaos * 6)
                    + Math.cos(a * 5 - t * 1.2)         * (chaos * 9)
                    + Math.sin(a * 7 + t * 0.85)        * (chaos * 6)
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
            grad.addColorStop(0, "#e7faed")
            ctx.fillStyle = grad
            ctx.fill()
        }
    }

    Canvas {
        id: blobCanvas2
        width: 380; height: 380
        anchors.centerIn: parent
        visible: !isSendMode

        property real t: 0
        property double lastMs: Date.now()

        Timer {
            interval: 16
            running: true
            repeat: true
            onTriggered: {
                var now = Date.now()
                var dt = Math.min(0.05, Math.max(0.0, (now - blobCanvas2.lastMs) * 0.001))
                blobCanvas2.lastMs = now
                blobCanvas2.t += dt * (Math.PI * 2 / Math.max(4.9, 8.0 - blobChaos * 2.2))
                blobCanvas2.requestPaint()
            }
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var chaos = blobChaos
            var cx = width / 2 + Math.cos(t * 0.29) * chaos * 8
            var cy = height / 2 + Math.sin(t * 0.41) * chaos * 11
            var n = 10
            var pts = []

            for (var i = 0; i < n; i++) {
                var a = (i / n) * Math.PI * 2 - Math.PI / 2
                var r = 140 + chaos * 18
                    + Math.sin(a * 2 + t)               * (11 + chaos * 12)
                    + Math.cos(a * 3 - t * 0.8)         * (8 + chaos * 9)
                    + Math.sin(a * 1.5 + t * 0.23)      * (6 + chaos * 7)
                    + Math.sin(a * 1.5 + t * 0.85)      * (4 + chaos * 6)
                    + Math.cos(a * 4 + t * 1.1)         * (chaos * 7)
                    + Math.sin(a * 6 - t * 0.95)        * (chaos * 5)
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
            grad.addColorStop(0, "#caeada")
            ctx.fillStyle = grad
            ctx.fill()
        }
    }



    Canvas {
        id: blobCanvas
        width: 380; height: 380
        anchors.centerIn: parent
        visible: !isSendMode

        property real t: 0
        property double lastMs: Date.now()

        Timer {
            interval: 16
            running: true
            repeat: true
            onTriggered: {
                var now = Date.now()
                var dt = Math.min(0.05, Math.max(0.0, (now - blobCanvas.lastMs) * 0.001))
                blobCanvas.lastMs = now
                blobCanvas.t += dt * (Math.PI * 2 / Math.max(4.4, 8.0 - blobChaos * 3.0))
                blobCanvas.requestPaint()
            }
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var chaos = blobChaos
            var cx = width / 2 + Math.sin(t * 0.55) * chaos * 12
            var cy = height / 2 + Math.cos(t * 0.47) * chaos * 10
            var n = 10
            var pts = []

            for (var i = 0; i < n; i++) {
                var a = (i / n) * Math.PI * 2 - Math.PI / 2
                var r = 130 + chaos * 15
                    + Math.sin(a * 2 + t)               * (11 + chaos * 15)
                    + Math.cos(a * 3 - t * 0.6)         * (8 + chaos * 12)
                    + Math.sin(a * 1.5 + t * 0.35)      * (6 + chaos * 9)
                    + Math.sin(a + t * 0.75)            * (3 + chaos * 6)
                    + Math.cos(a * 5 - t * 1.35)        * (chaos * 10)
                    + Math.sin(a * 8 + t * 0.92)        * (chaos * 6)
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
            grad.addColorStop(0, "#acdac4")
            ctx.fillStyle = grad
            ctx.fill()
        }
    }

    Rectangle {
        id: incomingTransferCard
        anchors.centerIn: parent
        visible: !isSendMode && hasIncomingTransfer
                 && transferKey(incomingTransfer) !== dismissedTransferKey
        width: 200
        height: 210
        radius: 34
        color: cardSurface
        border.color: cardBorder
        border.width: 1
        z: 10

        Rectangle {
            anchors.fill: parent
            anchors.margins: 10
            radius: parent.radius - 10
            color: "#ffffff"
            opacity: 0.84
        }

        Column {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 10

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: incomingHeadline(String(incomingTransfer.status || ""))
                font.pixelSize: 12
                font.weight: Font.DemiBold
                color: "#059669"
                horizontalAlignment: Text.AlignHCenter
            }

            DeviceCard {
                anchors.horizontalCenter: parent.horizontalCenter
                modelData: incomingTarget
            }

            Label {
                width: parent.width
                visible: String(incomingTransfer.fileName || "").length > 0
                text: String(incomingTransfer.fileName || "")
                font.pixelSize: 13
                font.weight: Font.StyleItalic
                color: textPrimary
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: incomingClickReady()
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: {
                dismissedTransferKey = transferKey(incomingTransfer)
                fileShareController.openFileLocation(String(incomingTransfer.filePath || ""))
            }
        }
    }

    onIncomingTransferChanged: {
        if (!incomingTransfer) {
            dismissedTransferKey = ""
            return
        }

        if (transferKey(incomingTransfer) !== dismissedTransferKey)
            return

        if (String(incomingTransfer.status || "") !== "Complete")
            dismissedTransferKey = ""
    }


    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 48
        visible: !isSendMode
        text: fileShareController.statusMessage
        font.pixelSize: 13
        color: textMuted
    }
}
