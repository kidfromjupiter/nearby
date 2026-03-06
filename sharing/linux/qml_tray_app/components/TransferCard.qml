import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    required property var modelData

    Layout.fillWidth: true
    implicitHeight: xferLayout.implicitHeight + 32
    radius: 24
    color: "#dcfce7"
    border.color: "#bbf7d0"

    readonly property color surface: "#ffffff"
    readonly property color textPrimary: "#111827"
    readonly property color textMuted: "#6b7280"

    readonly property bool isActive: modelData.status === "InProgress"
                                     || modelData.status === "Queued"
                                     || modelData.status === "Connecting"
                                     || modelData.status === "AwaitingLocalConfirmation"
                                     || modelData.status === "AwaitingRemoteAcceptance"
    readonly property bool isTerminal: !isActive

    function formatBytes(bytes) {
        if (bytes === undefined || bytes === null) return "0 B"
        var value = Number(bytes)
        if (!isFinite(value) || value < 0) return "0 B"
        var units = ["B", "KB", "MB", "GB", "TB"]
        var unit = 0
        while (value >= 1024 && unit < units.length - 1) { value /= 1024; unit += 1 }
        return (value >= 10 || unit === 0 ? value.toFixed(0) : value.toFixed(1)) + " " + units[unit]
    }

    ColumnLayout {
        id: xferLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                width: 48; height: 48; radius: 24
                color: surface

                Label {
                    anchors.centerIn: parent
                    text: modelData.targetName && modelData.targetName.length > 0
                          ? modelData.targetName.charAt(0).toUpperCase() : "?"
                    font.pixelSize: 18
                    font.weight: Font.Medium
                    color: textPrimary
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: modelData.targetName
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                    color: textPrimary
                }

                Label {
                    font.pixelSize: 12
                    color: textMuted
                    text: {
                        if (isActive)
                            return modelData.direction === "outgoing" ? "Sending..." : "Receiving..."
                        return modelData.status + (modelData.fileName && modelData.fileName.length > 0
                                                   ? " • " + modelData.fileName : "")
                    }
                }
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            visible: isActive
            from: 0; to: 1
            value: modelData.progress
        }

        Label {
            Layout.fillWidth: true
            visible: isActive
            horizontalAlignment: Text.AlignRight
            text: Math.round((modelData.progress || 0) * 100) + "% • " +
                  formatBytes(modelData.transferredBytes)
            font.pixelSize: 12
            color: textMuted
        }

        RowLayout {
            Layout.fillWidth: true
            visible: isTerminal

            Item { Layout.fillWidth: true }

            Rectangle {
                height: 36
                width: clearLbl.implicitWidth + 24
                radius: 10
                color: surface
                border.color: "#d1d5db"

                Label {
                    id: clearLbl
                    anchors.centerIn: parent
                    text: "Clear"
                    font.weight: Font.Medium
                    color: textPrimary
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: fileShareController.clearTransfers()
                }
            }
        }
    }
}
