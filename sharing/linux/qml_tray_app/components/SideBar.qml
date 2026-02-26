import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    Layout.preferredWidth: 280
    Layout.fillHeight: true

    readonly property color surface: "#ffffff"
    readonly property color cardBorder: "#bbf7d0"
    readonly property color textPrimary: "#111827"
    readonly property color textMuted: "#6b7280"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 0

        // Receive mode: visibility info
        ColumnLayout {
            visible: fileShareController.pendingSendFilePath.length === 0
            Layout.fillWidth: true
            spacing: 0

            Label {
                Layout.leftMargin: 12
                Layout.topMargin: 16
                Layout.bottomMargin: 8
                text: "Visibility state"
                color: textMuted
                font.pixelSize: 13
            }

            Rectangle {
                Layout.fillWidth: true
                height: 52
                radius: 12
                color: "#e8faf0"
                border.color: cardBorder

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12

                    Label {
                        Layout.fillWidth: true
                        text: !fileShareController.running
                              ? "Inactive"
                              : fileShareController.mode === "Send" ? "Discovering" : "Always visible"
                        font.weight: Font.Medium
                        color: textPrimary
                    }

                    Label {
                        text: "›"
                        font.pixelSize: 22
                        color: textMuted
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.topMargin: 8
                Layout.rightMargin: 12
                text: !fileShareController.running
                    ? "The service is not running. Start it to discover or receive files."
                    : fileShareController.mode === "Send"
                    ? "Discovering nearby devices. Select a device below to send your file."
                    : "Nearby devices can share files with you. You'll be notified and must approve each transfer."
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: textMuted
            }
        }

        // Send mode: outbound file info
        ColumnLayout {
            visible: fileShareController.pendingSendFilePath.length > 0
            Layout.fillWidth: true
            spacing: 0

            Label {
                Layout.leftMargin: 12
                Layout.topMargin: 16
                Layout.bottomMargin: 8
                text: "Sharing 1 file"
                font.weight: Font.Medium
                color: textPrimary
            }

            Rectangle {
                Layout.leftMargin: 12
                width: 72
                height: 72
                radius: 12
                color: surface

                Label {
                    anchors.centerIn: parent
                    text: "📄"
                    font.pixelSize: 28
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.topMargin: 8
                Layout.rightMargin: 12
                text: fileShareController.pendingSendFileName
                elide: Text.ElideRight
                font.pixelSize: 13
                color: textMuted
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.topMargin: 12
                Layout.rightMargin: 12
                text: "Make sure both devices are unlocked, close together, and have Bluetooth turned on."
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: textMuted
            }
        }

        Item { Layout.fillHeight: true }

        // Cancel (only visible in send mode)
        Rectangle {
            visible: fileShareController.pendingSendFilePath.length > 0
            Layout.leftMargin: 12
            Layout.bottomMargin: 12
            height: 40
            width: cancelLbl.implicitWidth + 24
            radius: 12
            color: "#f3f4f6"
            border.color: "#d1d5db"

            Label {
                id: cancelLbl
                anchors.centerIn: parent
                text: "Cancel"
                font.weight: Font.Medium
                color: textPrimary
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: fileShareController.switchToReceiveMode()
            }
        }
    }
}
