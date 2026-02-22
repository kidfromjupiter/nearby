import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 980
    height: 760
    minimumWidth: 820
    minimumHeight: 620
    visible: true
    title: "Nearby File Tray"

    readonly property color appBg: "#f5f6f8"
    readonly property color surface: "#ffffff"
    readonly property color border: "#d7dbe0"
    readonly property color textPrimary: "#1f2328"
    readonly property color textMuted: "#59636e"

    palette.window: appBg
    palette.base: surface
    palette.button: "#f0f3f7"
    palette.text: textPrimary
    palette.windowText: textPrimary
    palette.buttonText: textPrimary
    palette.placeholderText: textMuted
    palette.highlight: "#2f6feb"
    palette.highlightedText: "#ffffff"

    background: Rectangle {
        color: root.appBg
    }

    function endpointLabel(endpointId) {
        var label = fileShareController.peerNameForEndpoint(endpointId)
        if (!label || label.length === 0 || label === "Unknown device") {
            return "Unknown device"
        }
        return label
    }

    function formatBytes(bytes) {
        if (bytes === undefined || bytes === null) {
            return "0 B"
        }
        var value = Number(bytes)
        if (!isFinite(value) || value < 0) {
            return "0 B"
        }
        var units = ["B", "KB", "MB", "GB", "TB"]
        var unit = 0
        while (value >= 1024 && unit < units.length - 1) {
            value /= 1024
            unit += 1
        }
        return (value >= 10 || unit === 0 ? value.toFixed(0) : value.toFixed(1)) + " " + units[unit]
    }

    onClosing: function(close) {
        close.accepted = false
        root.hide()
        fileShareController.hideToTray()
    }

    header: ToolBar {
        height: 52

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 10

            Label {
                text: "Nearby File Share"
                font.bold: true
                font.pixelSize: 18
                color: root.textPrimary
            }

            Rectangle {
                radius: 12
                color: fileShareController.mode === "Send" ? "#dbeafe" : "#dcfce7"
                border.color: fileShareController.mode === "Send" ? "#60a5fa" : "#4ade80"
                Layout.preferredHeight: 24
                Layout.preferredWidth: 86

                Label {
                    anchors.centerIn: parent
                    text: fileShareController.mode
                    font.bold: true
                    color: "#1f2328"
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                text: fileShareController.running ? "Running" : "Stopped"
                color: fileShareController.running ? "#1f7a1f" : "#a33"
                font.bold: true
            }

            Button {
                text: fileShareController.running ? "Stop" : "Start"
                onClicked: {
                    if (fileShareController.running) {
                        fileShareController.stop()
                    } else {
                        fileShareController.start()
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Frame {
            Layout.fillWidth: true
            padding: 10
            background: Rectangle {
                color: root.surface
                border.color: root.border
                radius: 6
            }

            GridLayout {
                anchors.fill: parent
                columns: 4
                columnSpacing: 10
                rowSpacing: 8

                Label { text: "Device name" }
                TextField {
                    Layout.fillWidth: true
                    text: fileShareController.deviceName
                    onEditingFinished: fileShareController.deviceName = text
                }

                Label { text: "Selected file" }
                Label {
                    Layout.fillWidth: true
                    text: fileShareController.pendingSendFileName.length > 0
                          ? fileShareController.pendingSendFileName
                          : "None"
                    elide: Text.ElideRight
                    color: root.textMuted
                }

                Label { text: "Status" }
                Label {
                    Layout.columnSpan: 3
                    Layout.fillWidth: true
                    text: fileShareController.statusMessage
                    elide: Text.ElideRight
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Frame {
                Layout.preferredWidth: 420
                Layout.fillHeight: true
                padding: 10
                background: Rectangle {
                    color: root.surface
                    border.color: root.border
                    radius: 6
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "Nearby Devices"
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        Rectangle {
                            radius: 10
                            color: fileShareController.mode === "Send" ? "#dbeafe" : "#f1f5f9"
                            border.color: fileShareController.mode === "Send" ? "#60a5fa" : "#cbd5e1"
                            Layout.preferredHeight: 22
                            Layout.preferredWidth: 74

                            Label {
                                anchors.centerIn: parent
                                text: fileShareController.mode === "Send" ? "Discovering" : "Paused"
                                font.pixelSize: 11
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: fileShareController.mode === "Send"
                              ? "Select a nearby device to send your selected file."
                              : "Use tray menu Send to choose a file and start discovery."
                        wrapMode: Text.WordWrap
                        color: root.textMuted
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6
                        model: fileShareController.discoveredDevices

                        delegate: Rectangle {
                            required property string modelData
                            width: ListView.view.width
                            height: 70
                            color: root.surface
                            border.color: root.border
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Label {
                                        Layout.fillWidth: true
                                        text: root.endpointLabel(modelData)
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: modelData
                                        color: root.textMuted
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }
                                }

                                Button {
                                    text: "Send"
                                    enabled: fileShareController.mode === "Send"
                                             && fileShareController.pendingSendFilePath.length > 0
                                    onClicked: fileShareController.sendPendingFileToEndpoint(modelData)
                                }
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10

                Frame {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 220
                    padding: 10
                    background: Rectangle {
                        color: root.surface
                        border.color: root.border
                        radius: 6
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 6

                        Label {
                            text: "Connected Devices"
                            font.bold: true
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 6
                            model: fileShareController.connectedDevices

                            delegate: Rectangle {
                                required property string modelData
                                width: ListView.view.width
                                height: 46
                                color: root.surface
                                border.color: root.border
                                radius: 6

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10

                                    Label {
                                        Layout.fillWidth: true
                                        text: root.endpointLabel(modelData)
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }

                                    Rectangle {
                                        radius: 10
                                        color: "#e2e8f0"
                                        border.color: "#cbd5e1"
                                        Layout.preferredHeight: 22
                                        Layout.preferredWidth: Math.max(68, mediumLabel.implicitWidth + 16)

                                        Label {
                                            id: mediumLabel
                                            anchors.centerIn: parent
                                            text: fileShareController.mediumForEndpoint(modelData)
                                            font.pixelSize: 11
                                            color: "#334155"
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    padding: 10
                    background: Rectangle {
                        color: root.surface
                        border.color: root.border
                        radius: 6
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: "Transfers"
                                font.bold: true
                            }
                            Item { Layout.fillWidth: true }
                            Button {
                                text: "Clear"
                                onClicked: fileShareController.clearTransfers()
                            }
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 6
                            model: fileShareController.transfers

                            delegate: Rectangle {
                                required property var modelData
                                width: ListView.view.width
                                height: 86
                                color: root.surface
                                border.color: root.border
                                radius: 6

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true

                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.direction + " | " + root.endpointLabel(modelData.endpointId)
                                            font.bold: true
                                            elide: Text.ElideRight
                                        }

                                        Label { text: modelData.status }

                                        Rectangle {
                                            radius: 10
                                            color: "#e2e8f0"
                                            border.color: "#cbd5e1"
                                            Layout.preferredHeight: 22
                                            Layout.preferredWidth: Math.max(68, transferMediumLabel.implicitWidth + 16)

                                            Label {
                                                id: transferMediumLabel
                                                anchors.centerIn: parent
                                                text: modelData.medium
                                                font.pixelSize: 11
                                                color: "#334155"
                                            }
                                        }
                                    }

                                    ProgressBar {
                                        Layout.fillWidth: true
                                        from: 0
                                        to: 1
                                        value: modelData.progress
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignRight
                                        text: root.formatBytes(modelData.bytesTransferred)
                                              + " / " + root.formatBytes(modelData.totalBytes)
                                        color: root.textMuted
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
