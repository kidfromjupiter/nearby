import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "components"

ApplicationWindow {
    id: root
    width: 980
    height: 760
    minimumWidth: 820
    minimumHeight: 620
    visible: true
    title: "Quick Share"

    background: Rectangle { color: "#f0fdf4" }

    onClosing: function(close) {
        close.accepted = false
        root.hide()
        fileShareController.hideToTray()
    }

    SettingsPanel {
        id: settingsPanel
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        AppHeader {
            onSettingsRequested: settingsPanel.open()
        }

        // ── Body ─────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            SideBar {}

            // ── Main content (white panel) ────────────────────────────────
            Rectangle {
                id: mainContent
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#ffffff"
                topLeftRadius: 48
                clip: true

                readonly property bool isSendMode: fileShareController.pendingSendFilePath.length > 0
                readonly property bool isIdle: fileShareController.discoveredTargets.length === 0
                                               && fileShareController.transfers.length === 0

                // ── Idle: animated blob ───────────────────────────────────
                AnimatedBlob { visible: mainContent.isIdle }

                // ── Non-idle: scrollable device + transfer cards ──────────
                Flickable {
                    id: mainFlickable
                    anchors.fill: parent
                    clip: true
                    visible: !mainContent.isIdle
                    contentWidth: width
                    contentHeight: mainCol.implicitHeight + 96
                    ScrollBar.vertical: ScrollBar {}

                    ColumnLayout {
                        id: mainCol
                        x: 48
                        y: 48
                        width: mainFlickable.width - 96
                        spacing: 16

                        SendUrlPanel {
                            Layout.alignment: Qt.AlignHCenter
                            width: Math.max(240, Math.min(mainCol.width, 420))
                            visible: mainContent.isSendMode
                        }


                        Label {
                            text: "Nearby devices"
                            font.pixelSize: 20
                            font.weight: Font.Medium
                            color: "#111827"
                        }

                        Item {
                            Layout.fillWidth: true
                            implicitHeight: deviceFlow.childrenRect.height
                            visible: fileShareController.discoveredTargets.length > 0

                            Flow {
                                id: deviceFlow
                                width: parent.width
                                spacing: 20

                                Repeater {
                                    model: fileShareController.discoveredTargets
                                    delegate: DeviceCard {}
                                }
                            }
                        }

                    }
                }
            }
        }
    }
}
