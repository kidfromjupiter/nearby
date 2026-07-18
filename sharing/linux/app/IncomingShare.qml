import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes

Rectangle {
    id: root
    color: "#DCF5FF"
    Layout.fillWidth: true
    Layout.fillHeight: true

    property var shareTargetId: 0
    property string direction: "receive"
    property string filename: "ThisIsAnImage.jpg"
    property string targetname: "Lasan's A55"
    property bool transferring: !awaitingLocalConfirmation
    property real progressValue: 0.64
    property string status: "kUnknown"
    property bool isFinalStatus: false
    property bool awaitingLocalConfirmation: false
    property int totalAttachmentsCount: 0
    property int transferredAttachmentsCount: 0
    property var totalBytes: 0
    property var transferredBytes: 0

    signal returnHomeRequested()

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
        if (status === "kAwaitingLocalConfirmation") {
            return "Incoming share request"
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

    TransferCard {
        visible: root.transferring
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.9, 700)
        height: 212
        shareTargetId: root.shareTargetId
        direction: root.direction
        filename: root.filename
        targetname: root.targetname
        progressValue: root.progressValue
        status: root.status
        isFinalStatus: root.isFinalStatus
        totalBytes: root.totalBytes
        totalAttachmentsCount: root.totalAttachmentsCount
        transferredAttachmentsCount: root.transferredAttachmentsCount
        finalActionText: root.isFinalStatus ? "Go back home" : ""
        onCancelRequested: backend.cancel(root.shareTargetId)
        onFinalActionRequested: root.returnHomeRequested()
    }

    Item {
        visible: !root.transferring
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.85, 360)
        height: 460

        Rectangle {
            id: cardBackground
            anchors.fill: parent
            radius: 24
            color: "#FFFFFF"
            border.color: "#BCE5F5"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 28
            spacing: 16

            Text {
                Layout.fillWidth: true
                font.pointSize: 18
                font.weight: 700
                color: "#377B95"
                text: root.direction === "send" ? "Outgoing share" : "Incoming share"
                horizontalAlignment: Text.AlignHCenter
            }

            Image {
                source: "qrc:/icons/file.svg"
                Layout.fillHeight: true
                Layout.fillWidth: true
                fillMode: Image.PreserveAspectCrop
                sourceSize.height: height
                sourceSize.width: width
                smooth: true
                antialiasing: true
            }

            Text {
                Layout.fillWidth: true
                color: "#57707A"
                font.pointSize: 11
                wrapMode: Text.WrapAnywhere
                text: root.baseName(root.filename)
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 2
                elide: Text.ElideMiddle
            }

            Text {
                Layout.fillWidth: true
                color: "#57707A"
                font.pointSize: 15
                font.weight: 700
                wrapMode: Text.WrapAnywhere
                text: root.targetname
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 2
                elide: Text.ElideMiddle
            }

            RowLayout {
                Button {
                    id: cancelbutton
                    Layout.fillWidth: true
                    text: "Reject"
                    onClicked: backend.reject(root.shareTargetId)

                    contentItem: Text {
                        text: cancelbutton.text
                        font.pointSize: 14
                        font.weight: 600
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitHeight: 46
                        radius: 12
                        color: cancelbutton.hovered ? "#db1e1e" : "#6EA7B6"
                        border.color: "#91C8DE"
                        border.width: 1
                    }
                }

                Button {
                    id: acceptButton
                    Layout.fillWidth: true
                    text: "Accept"
                    onClicked: backend.accept(root.shareTargetId)

                    contentItem: Text {
                        text: acceptButton.text
                        font.pointSize: 14
                        font.weight: 600
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitHeight: 46
                        radius: 12
                        color: acceptButton.hovered ? "#195871" : "#06384C"
                        border.color: "#91C8DE"
                        border.width: 1
                    }
                }
            }
        }
    }
}
