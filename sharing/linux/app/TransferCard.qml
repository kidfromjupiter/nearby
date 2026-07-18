import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root

    implicitWidth: 700
    implicitHeight: compact ? 156 : 212

    property bool compact: false
    property var shareTargetId: 0
    property string direction: "send"
    property string filename: ""
    property string targetname: "Unknown device"
    property real progressValue: 0
    property string status: "kUnknown"
    property bool isFinalStatus: false
    property bool cancelEnabled: true
    property int totalAttachmentsCount: 0
    property int transferredAttachmentsCount: 0
    property var totalBytes: 0
    property string finalActionText: ""

    signal cancelRequested()
    signal finalActionRequested()

    function baseName(path) {
        if (!path || path.length === 0) {
            return direction === "send" ? "Selected file" : transferSummary()
        }
        const normalized = decodeURIComponent(path).replace("file://", "")
        const parts = normalized.split("/")
        return parts.length === 0 ? normalized : parts[parts.length - 1]
    }

    function transferSummary() {
        if (totalAttachmentsCount > 0) {
            return totalAttachmentsCount === 1 ? "1 item" : totalAttachmentsCount + " items"
        }
        if (totalBytes > 0) {
            return formatBytes(totalBytes)
        }
        return "Shared items"
    }

    function formatBytes(bytes) {
        let value = Number(bytes)
        if (!isFinite(value) || value <= 0) {
            return ""
        }
        const units = ["B", "KB", "MB", "GB", "TB"]
        let unitIndex = 0
        while (value >= 1024 && unitIndex < units.length - 1) {
            value /= 1024
            unitIndex += 1
        }
        return (unitIndex === 0 ? Math.round(value) : value.toFixed(value >= 10 ? 1 : 2)) + " " + units[unitIndex]
    }

    function statusText() {
        if (status === "kStartFailed") {
            return "Transfer failed"
        }
        if (status === "kConnecting") {
            return direction === "send" ? "Connecting" : "Preparing transfer"
        }
        if (status === "kAwaitingRemoteAcceptance") {
            return "Waiting for receiver"
        }
        if (status === "kInProgress") {
            const verb = direction === "send" ? "Sending" : "Receiving"
            if (totalAttachmentsCount > 0) {
                return verb + " " + Math.min(transferredAttachmentsCount + 1, totalAttachmentsCount) + " of " + totalAttachmentsCount + " items"
            }
            return verb
        }
        if (status === "kComplete") {
            return direction === "send" ? "Sent" : "Received"
        }
        if (status === "kReject") {
            return "Rejected"
        }
        if (status === "kCancelled") {
            return "Cancelled"
        }
        if (status === "kTimedOut") {
            return "Timed out"
        }
        if (isFinalStatus) {
            return "Transfer failed"
        }
        return direction === "send" ? "Starting send" : "Preparing transfer"
    }

    Rectangle {
        anchors.fill: parent
        radius: 24
        color: "#FFFFFF"
        border.color: "#BCE5F5"
        border.width: 1
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: root.compact ? 16 : 28
        spacing: root.compact ? 16 : 24

        Rectangle {
            Layout.preferredWidth: root.compact ? 72 : 96
            Layout.preferredHeight: root.compact ? 72 : 96
            radius: root.compact ? 18 : 24
            color: "#E8F7FC"

            Image {
                anchors.centerIn: parent
                width: root.compact ? 44 : 60
                height: root.compact ? 44 : 60
                source: "qrc:/icons/file.svg"
                fillMode: Image.PreserveAspectFit
                sourceSize.width: width
                sourceSize.height: height
                smooth: true
                antialiasing: true
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: root.compact ? 5 : 10

            Text {
                text: root.targetname
                font.pointSize: root.compact ? 14 : 17
                font.weight: 700
                color: "#1A1C1E"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                text: root.baseName(root.filename)
                color: "#57707A"
                font.pointSize: root.compact ? 10 : 12
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            ProgressBar {
                id: transferProgress
                Layout.fillWidth: true
                from: 0
                to: 1
                value: Math.max(0, Math.min(1, root.progressValue))

                background: Rectangle {
                    implicitHeight: root.compact ? 8 : 10
                    radius: root.compact ? 4 : 5
                    color: "#DDEEF5"
                }

                contentItem: Item {
                    Rectangle {
                        width: transferProgress.visualPosition * parent.width
                        height: root.compact ? 8 : 10
                        radius: root.compact ? 4 : 5
                        color: "#0D6D90"
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: root.statusText()
                    color: "#57707A"
                    font.pointSize: root.compact ? 10 : 12
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Text {
                    text: Math.round(Math.max(0, Math.min(1, root.progressValue)) * 100) + "%"
                    color: "#0D6D90"
                    font.pointSize: root.compact ? 11 : 13
                    font.weight: 700
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    id: actionButton
                    visible: !root.isFinalStatus || root.finalActionText.length > 0
                    enabled: root.isFinalStatus || root.cancelEnabled
                    text: root.isFinalStatus ? root.finalActionText : "Cancel"
                    opacity: enabled ? 1 : 0.55
                    onClicked: {
                        if (root.isFinalStatus) {
                            root.finalActionRequested()
                        } else {
                            root.cancelRequested()
                        }
                    }

                    contentItem: Text {
                        text: actionButton.text
                        font.pointSize: root.compact ? 12 : 14
                        font.weight: 600
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitWidth: root.compact ? 100 : 118
                        implicitHeight: root.compact ? 36 : 46
                        radius: root.compact ? 10 : 12
                        color: root.isFinalStatus
                               ? (actionButton.hovered ? "#195871" : "#06384C")
                               : (actionButton.hovered ? "#B62828" : "#D94C4C")
                    }
                }
            }
        }
    }
}
