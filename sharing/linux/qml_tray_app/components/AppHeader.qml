import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    Layout.fillWidth: true
    height: 80

    readonly property color textPrimary: "#111827"
    readonly property color textMuted: "#6b7280"
    readonly property color accent: "#16a34a"

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24

        ColumnLayout {
            spacing: 2
            Label {
                text: "Device name"
                font.pixelSize: 12
                color: textMuted
            }
            Label {
                text: fileShareController.deviceName
                font.pixelSize: 22
                font.weight: Font.Medium
                color: textPrimary
            }
        }

        Item { Layout.fillWidth: true }

        Label {
            text: fileShareController.running ? "● Running" : "○ Stopped"
            color: fileShareController.running ? accent : textMuted
            font.pixelSize: 13
            font.weight: Font.Medium
        }

        Rectangle {
            height: 40
            width: startStopLabel.implicitWidth + 32
            radius: 12
            color: fileShareController.running ? "#fee2e2" : "#dcfce7"
            border.color: fileShareController.running ? "#fca5a5" : "#86efac"

            Label {
                id: startStopLabel
                anchors.centerIn: parent
                text: fileShareController.running ? "Stop" : "Start"
                color: fileShareController.running ? "#dc2626" : accent
                font.weight: Font.Medium
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: fileShareController.running ? fileShareController.stop() : fileShareController.start()
            }
        }
    }
}
