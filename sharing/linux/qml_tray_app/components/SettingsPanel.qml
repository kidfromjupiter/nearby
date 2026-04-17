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
    readonly property color surface: "#ffffff"
    readonly property color accent: "#38aa62"
    readonly property color accentLight: "#dcfce7"
    readonly property color borderColor: "#bbf7d0"
    readonly property color textPrimary: "#111827"
    readonly property color textMuted: "#6b7280"

    background: Rectangle { color: root.bg }

    ColumnLayout {
        width: root.width
        height: root.height
        spacing: 0

        // Header
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

                Rectangle {
                    width: 32
                    height: 32
                    radius: 8
                    color: closeArea.containsMouse ? "#f3f4f6" : "transparent"

                    Label {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 14
                        color: root.textMuted
                    }

                    MouseArea {
                        id: closeArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.close()
                    }
                }
            }
        }

        Flickable {
            id: flick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: false
            contentWidth: width
            contentHeight: settingsCol.height + 32
            ScrollBar.vertical: ScrollBar {}

            Column {
                id: settingsCol
                x: 20
                y: 20
                width: flick.width - 40
                spacing: 20

                SectionLabel { text: "DEVICE" }
                SectionCard {
                    width: settingsCol.width

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Label {
                                text: "Device name"
                                font.pixelSize: 13
                                color: root.textMuted
                                Layout.preferredWidth: 110
                            }
                            ThemedField {
                                text: fileShareController.deviceName
                                onEditingFinished: fileShareController.deviceName = text
                            }
                        }
                    }
                }

                SectionLabel { text: "SHARING" }
                SectionCard {
                    width: settingsCol.width

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 12

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                            color: root.textMuted
                            text: "Nearby Sharing uses built-in transport and discovery settings."
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Label {
                                Layout.fillWidth: true
                                color: root.textPrimary
                                font.pixelSize: 13
                                text: "Auto-accept incoming"
                            }
                            ThemedToggle {
                                checked: fileShareController.autoAcceptIncoming
                                onToggled: fileShareController.autoAcceptIncoming = checked
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Label {
                                Layout.fillWidth: true
                                color: root.textPrimary
                                font.pixelSize: 13
                                text: "Enable 5 GHz hotspot"
                            }
                            ThemedToggle {
                                checked: fileShareController.enable5GhzHotspot
                                onToggled: fileShareController.enable5GhzHotspot = checked
                            }
                        }
                    }
                }

                SectionLabel { text: "LOGGING" }
                SectionCard {
                    width: settingsCol.width

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Label {
                                text: "Log path"
                                font.pixelSize: 13
                                color: root.textMuted
                                Layout.preferredWidth: 110
                            }
                            ThemedField {
                                font.pixelSize: 11
                                text: fileShareController.logPath
                                onEditingFinished: fileShareController.logPath = text
                            }
                        }
                    }
                }
            }
        }
    }

    component SectionLabel: Label {
        font.pixelSize: 11
        font.weight: Font.DemiBold
        font.letterSpacing: 0.8
        color: root.accent
    }

    component SectionCard: Rectangle {
        radius: 12
        color: root.surface
        border.color: root.borderColor
        height: (children.length > 0 ? children[0].implicitHeight : 0) + 24
    }

    component ThemedField: TextField {
        Layout.fillWidth: true
        implicitHeight: 38
        font.pixelSize: 13
        leftPadding: 12
        rightPadding: 12
        topPadding: 0
        bottomPadding: 0
        verticalAlignment: TextInput.AlignVCenter
        color: root.textPrimary
        background: Rectangle {
            radius: 8
            color: "#f9fafb"
            border.color: parent.activeFocus ? root.accent : root.borderColor
            border.width: parent.activeFocus ? 2 : 1
        }
    }

    component ThemedToggle: Switch {
        palette.highlight: root.accent
    }
}
