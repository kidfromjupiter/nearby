import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Drawer {
    id: root
    edge: Qt.RightEdge
    width: 380
    height: parent ? parent.height : 0
    implicitWidth: 380
    implicitHeight: parent ? parent.height : 0

    readonly property color bg: "#f0fdf4"
    readonly property color cardBg: "#ffffff"
    readonly property color borderColor: "#bbf7d0"
    readonly property color textPrimary: "#111827"
    readonly property color textMuted: "#6b7280"

    background: Rectangle { color: root.bg }

    ColumnLayout {
        width: root.width
        height: root.height
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            height: 64
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20

                Label {
                    text: "Settings"
                    font.pixelSize: 20
                    font.weight: Font.Bold
                    color: root.textPrimary
                }

                Item { Layout.fillWidth: true }

                ToolButton {
                    text: "✕"
                    onClicked: root.close()
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: root.width - 40
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 16

                GroupBox {
                    title: "Device"
                    Layout.fillWidth: true

                    background: Rectangle {
                        color: root.cardBg
                        radius: 10
                        border.color: root.borderColor
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Label {
                            text: "Device name"
                            color: root.textMuted
                        }
                        TextField {
                            Layout.fillWidth: true
                            text: fileShareController.deviceName
                            onEditingFinished: fileShareController.deviceName = text
                        }
                    }
                }

                GroupBox {
                    title: "Sharing"
                    Layout.fillWidth: true

                    background: Rectangle {
                        color: root.cardBg
                        radius: 10
                        border.color: root.borderColor
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            color: root.textMuted
                            text: "Nearby Sharing uses built-in transport and discovery settings."
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                Layout.fillWidth: true
                                color: root.textPrimary
                                text: "Auto-accept incoming"
                            }
                            Switch {
                                checked: fileShareController.autoAcceptIncoming
                                onToggled: fileShareController.autoAcceptIncoming = checked
                            }
                        }
                    }
                }

                GroupBox {
                    title: "Logging"
                    Layout.fillWidth: true

                    background: Rectangle {
                        color: root.cardBg
                        radius: 10
                        border.color: root.borderColor
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Label {
                            text: "Log path"
                            color: root.textMuted
                        }
                        TextField {
                            Layout.fillWidth: true
                            text: fileShareController.logPath
                            onEditingFinished: fileShareController.logPath = text
                        }
                    }
                }
            }
        }
    }
}
