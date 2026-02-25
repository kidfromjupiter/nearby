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

                readonly property bool isIdle: fileShareController.discoveredDevices.length === 0
                                               && fileShareController.transfers.length === 0

                // ── Idle: animated blob ───────────────────────────────────
                AnimatedBlob {}

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

                        Label {
                            text: "Nearby devices"
                            font.pixelSize: 20
                            font.weight: Font.Medium
                            color: "#111827"
                        }

                        Repeater {
                            model: fileShareController.discoveredDevices
                            delegate: DeviceCard {}
                        }

                        Repeater {
                            model: fileShareController.transfers
                            delegate: TransferCard {}
                        }
                    }
                }
            }
        }
    }
}
