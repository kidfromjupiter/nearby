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

        // ── Header ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 64
            color: "transparent"
            // border.color: root.borderColor

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
                    // color: closeArea.containsMouse ? "#f3f4f6" : "transparent"

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

        // ── Scrollable content ────────────────────────────────────────────
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

                // ── Device ───────────────────────────────────────────────
                SectionLabel { text: "DEVICE" }

                SectionCard {
                    width: settingsCol.width

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 14

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

                // ── Connection ────────────────────────────────────────────
                SectionLabel { text: "CONNECTION" }

                SectionCard {
                    width: settingsCol.width

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 14

                        // ── Advanced Settings ─────────────────────────────
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            // Header row (always visible)
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Canvas {
                                    id: advChevron
                                    width: 14; height: 14
                                    property bool open: false
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        ctx.strokeStyle = root.accent
                                        ctx.lineWidth = 1.8
                                        ctx.lineCap = "round"
                                        ctx.lineJoin = "round"
                                        ctx.beginPath()
                                        if (open) {
                                            ctx.moveTo(2, 5); ctx.lineTo(7, 10); ctx.lineTo(12, 5)
                                        } else {
                                            ctx.moveTo(5, 2); ctx.lineTo(10, 7); ctx.lineTo(5, 12)
                                        }
                                        ctx.stroke()
                                    }
                                }

                                Label {
                                    text: "Advanced Settings"
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                    color: root.accent
                                }

                                Item { Layout.fillWidth: true }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        advChevron.open = !advChevron.open
                                        advChevron.requestPaint()
                                    }
                                }
                            }

                            // Warning + Strategy (shown when expanded)
                            ColumnLayout {
                                visible: advChevron.open
                                Layout.fillWidth: true
                                spacing: 10

                                Label {
                                    Layout.fillWidth: true
                                    text: "⚠️  Only mess with these if you know what you're doing."
                                    font.pixelSize: 11
                                    color: "#b45309"
                                    wrapMode: Text.WordWrap
                                    topPadding: 6
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 10
                                    Label {
                                        text: "Strategy"
                                        font.pixelSize: 13
                                        color: root.textMuted
                                        Layout.preferredWidth: 110
                                    }
                                    ThemedCombo {
                                        id: strategyCombo
                                        model: ["P2pPointToPoint", "P2pStar", "P2pCluster"]
                                        currentIndex: {
                                            var s = fileShareController.connectionStrategy
                                            if (s === "P2pStar") return 1
                                            if (s === "P2pCluster") return 2
                                            return 0
                                        }
                                        onActivated: fileShareController.connectionStrategy = currentText
                                        Connections {
                                            target: fileShareController
                                            function onConnectionStrategyChanged() {
                                                var s = fileShareController.connectionStrategy
                                                strategyCombo.currentIndex = s === "P2pStar" ? 1 : s === "P2pCluster" ? 2 : 0
                                            }
                                        }
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 10
                                    Label {
                                        text: "Service ID"
                                        font.pixelSize: 13
                                        color: root.textMuted
                                        Layout.preferredWidth: 110
                                    }
                                    ThemedField {
                                        text: fileShareController.serviceId
                                        onEditingFinished: fileShareController.serviceId = text
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: root.borderColor
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: "Auto-accept incoming"
                                font.pixelSize: 13
                                color: root.textPrimary
                                Layout.fillWidth: true
                            }
                            Switch {
                                checked: fileShareController.autoAcceptIncoming
                                palette.highlight: root.accent
                                onCheckedChanged: fileShareController.autoAcceptIncoming = checked
                            }
                        }
                    }
                }

                // ── Transport mediums ─────────────────────────────────────
                SectionLabel { text: "TRANSPORT MEDIUMS" }

                SectionCard {
                    width: settingsCol.width

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 4

                        MediumRow { label: "Bluetooth";   checked: fileShareController.bluetoothEnabled;   onToggled: (v) => fileShareController.bluetoothEnabled   = v }
                        MediumRow { label: "BLE";         checked: fileShareController.bleEnabled;         onToggled: (v) => fileShareController.bleEnabled         = v }
                        MediumRow { label: "WiFi LAN";    checked: fileShareController.wifiLanEnabled;     onToggled: (v) => fileShareController.wifiLanEnabled     = v }
                        MediumRow { label: "WiFi Hotspot";checked: fileShareController.wifiHotspotEnabled; onToggled: (v) => fileShareController.wifiHotspotEnabled = v }
                        MediumRow { label: "WebRTC";      checked: fileShareController.webRtcEnabled;      onToggled: (v) => fileShareController.webRtcEnabled      = v }
                    }
                }

                // ── Logging ───────────────────────────────────────────────
                SectionLabel { text: "LOGGING" }

                SectionCard {
                    width: settingsCol.width

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 14

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

    // ── Section heading label ─────────────────────────────────────────────
    component SectionLabel: Label {
        font.pixelSize: 11
        font.weight: Font.DemiBold
        font.letterSpacing: 0.8
        color: root.accent
    }

    // ── Rounded card that sizes itself to its content ─────────────────────
    component SectionCard: Rectangle {
        radius: 12
        color: root.surface
        border.color: root.borderColor
        // height wraps the first ColumnLayout child placed inside via anchors.top
        height: (children.length > 0 ? children[0].implicitHeight : 0) + 32
    }

    // ── Themed text field ─────────────────────────────────────────────────
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

    // ── Themed combo box ──────────────────────────────────────────────────
    component ThemedCombo: ComboBox {
        id: theComboBox
        Layout.fillWidth: true
        implicitHeight: 38
        font.pixelSize: 13
        leftPadding: 12
        rightPadding: 36   // leave room for the indicator
        topPadding: 0
        bottomPadding: 0

        background: Rectangle {
            radius: 8
            color: "#f9fafb"
            border.color: theComboBox.down || theComboBox.hovered ? root.accent : root.borderColor
            border.width: theComboBox.down ? 2 : 1
        }

        contentItem: Label {
            text: theComboBox.displayText
            font: theComboBox.font
            color: root.textPrimary
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        indicator: Item {
            x: theComboBox.width - width - 10
            y: (theComboBox.height - height) / 2
            width: 16
            height: 16

            Canvas {
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = root.accent
                    ctx.lineWidth = 2
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    ctx.beginPath()
                    ctx.moveTo(2, 5)
                    ctx.lineTo(8, 11)
                    ctx.lineTo(14, 5)
                    ctx.stroke()
                }
            }
        }

        popup: Popup {
            y: theComboBox.height + 4
            width: theComboBox.width
            padding: 4

            background: Rectangle {
                radius: 10
                color: root.surface
                border.color: root.borderColor
            }

            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: theComboBox.delegateModel
                currentIndex: theComboBox.highlightedIndex
                ScrollIndicator.vertical: ScrollIndicator {}
            }
        }

        delegate: ItemDelegate {
            width: ListView.view ? ListView.view.width : 0
            highlighted: theComboBox.highlightedIndex === index

            contentItem: Label {
                leftPadding: 8
                text: modelData
                font.pixelSize: 13
                color: root.textPrimary
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 7
                color: parent.highlighted ? root.accentLight : "transparent"
            }
        }
    }

    // ── One toggle row for a transport medium ─────────────────────────────
    component MediumRow: RowLayout {
        id: mrow
        required property string label
        required property bool checked
        signal toggled(bool value)

        Layout.fillWidth: true
        implicitHeight: 38
        spacing: 12

        Rectangle {
            width: 20
            height: 20
            radius: 5
            color: mrow.checked ? root.accentLight : root.surface
            border.color: mrow.checked ? root.accent : "#d1d5db"

            Label {
                anchors.centerIn: parent
                text: "✓"
                font.pixelSize: 11
                font.weight: Font.Bold
                color: root.accent
                visible: mrow.checked
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: mrow.toggled(!mrow.checked)
            }
        }

        Label {
            text: mrow.label
            font.pixelSize: 13
            color: root.textPrimary
            Layout.fillWidth: true

            TapHandler {
                cursorShape: Qt.PointingHandCursor
                onTapped: mrow.toggled(!mrow.checked)
            }
        }
    }
}
