import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    required property var modelData

    Layout.fillWidth: true
    height: 76
    radius: 24
    color: "#dcfce7"
    border.color: "#bbf7d0"

    readonly property color surface: "#ffffff"
    readonly property color textPrimary: "#111827"
    readonly property color textMuted: "#6b7280"

    readonly property string targetName: modelData.name && modelData.name.length > 0
                                         ? modelData.name : "Unknown device"

    function initialLetter(label) {
        if (!label || label.length === 0) return "?"
        return label.charAt(0).toUpperCase()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 12

        Rectangle {
            width: 48; height: 48; radius: 24
            color: surface

            Label {
                anchors.centerIn: parent
                text: initialLetter(targetName)
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
                text: targetName
                font.weight: Font.Medium
                elide: Text.ElideRight
                color: textPrimary
            }

            Label {
                Layout.fillWidth: true
                text: "#" + modelData.id
                font.pixelSize: 11
                color: textMuted
                elide: Text.ElideRight
            }
        }

        Rectangle {
            height: 36
            width: sendBtnLbl.implicitWidth + 24
            radius: 10
            color: surface
            border.color: "#d1d5db"
            opacity: (fileShareController.mode === "Send"
                      && fileShareController.pendingSendFilePath.length > 0) ? 1.0 : 0.45

            Label {
                id: sendBtnLbl
                anchors.centerIn: parent
                text: "Send"
                font.weight: Font.Medium
                color: textPrimary
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                enabled: fileShareController.mode === "Send"
                         && fileShareController.pendingSendFilePath.length > 0
                onClicked: fileShareController.sendPendingFileToTarget(modelData.id)
            }
        }
    }
}
