import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1200
    height: 820
    minimumWidth: 980
    minimumHeight: 700
    visible: true
    title: "Nearby QML Tray"

    property string apiEndpointId: ""
    property string apiPayloadText: ""
    property string queryMediumResult: "-"
    property string queryPeerResult: "-"
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

    ListModel {
        id: connectedEndpointsModel
    }

    ListModel {
        id: payloadEventsModel
    }

    function endpointLabel(endpointId) {
        var label = nearbyController.peerNameForEndpoint(endpointId)
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

    function refreshConnectedEndpointsModel() {
        var selectedId = targetCombo.currentValue ? String(targetCombo.currentValue) : ""
        connectedEndpointsModel.clear()
        var devices = nearbyController.connectedDevices
        for (var i = 0; i < devices.length; ++i) {
            var endpointId = String(devices[i])
            connectedEndpointsModel.append({
                "endpointId": endpointId,
                "label": endpointLabel(endpointId)
            })
        }
        var nextIndex = -1
        for (var j = 0; j < connectedEndpointsModel.count; ++j) {
            if (connectedEndpointsModel.get(j).endpointId === selectedId) {
                nextIndex = j
                break
            }
        }
        if (nextIndex < 0 && connectedEndpointsModel.count > 0) {
            nextIndex = 0
        }
        targetCombo.currentIndex = nextIndex
    }

    onClosing: function(close) {
        close.accepted = false
        root.hide()
        nearbyController.hideToTray()
    }

    header: ToolBar {
        height: 54

        // background: Rectangle {
        //     color: root.surface
        //     border.color: root.border
        // }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 10

            Label {
                text: "Nearby Control"
                font.bold: true
                font.pixelSize: 18
                color: root.textPrimary
            }

            Item { Layout.fillWidth: true }

            Label {
                text: nearbyController.running ? "Running" : "Stopped"
                color: nearbyController.running ? "#1f7a1f" : "#a33"
                font.bold: true
            }

            Button {
                text: nearbyController.running ? "Stop" : "Start"
                onClicked: {
                    if (nearbyController.running) {
                        nearbyController.stop()
                    } else {
                        nearbyController.start()
                    }
                }
            }

            Button {
                text: "Hide To Tray"
                onClicked: {
                    root.hide()
                    nearbyController.hideToTray()
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

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                Label {
                    text: "Settings"
                    font.bold: true
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 8
                    columnSpacing: 8
                    rowSpacing: 8

                    Label { text: "Mode" }
                    ComboBox {
                        id: modeCombo
                        model: ["Receive", "Send"]
                        currentIndex: nearbyController.mode === "Send" ? 1 : 0
                        onActivated: nearbyController.mode = currentText
                    }

                    ColumnLayout {
                        anchors.leftMargin: 30
                        anchors.rightMargin: 30
                        Label {
                            anchors.fill : parent
                            text: "Mediums"
                            Layout.alignment: Qt.AlignTop
                            horizontalAlignment: Text.AlignHCenter
                            topPadding: 6
                        }
                        RowLayout {
                            spacing: 4
                            CheckBox {
                                id: bluetoothCheckbox
                                text: "Bluetooth"
                                checked: nearbyController.bluetoothEnabled
                                onCheckedChanged: nearbyController.bluetoothEnabled = checked
                            }
                            CheckBox {
                                id: bleCheckbox
                                text: "BLE"
                                checked: nearbyController.bleEnabled
                                onCheckedChanged: nearbyController.bleEnabled = checked
                            }
                            CheckBox {
                                id: wifiLanCheckbox
                                text: "WiFi LAN"
                                checked: nearbyController.wifiLanEnabled
                                onCheckedChanged: nearbyController.wifiLanEnabled = checked
                            }
                            CheckBox {
                                id: wifiHotspotCheckbox
                                text: "WiFi Hotspot"
                                checked: nearbyController.wifiHotspotEnabled
                                onCheckedChanged: nearbyController.wifiHotspotEnabled = checked
                            }
                            CheckBox {
                                id: webRtcCheckbox
                                text: "WebRTC"
                                checked: nearbyController.webRtcEnabled
                                onCheckedChanged: nearbyController.webRtcEnabled = checked
                            }
                        }
                    }

                    Label { text: "Strategy" }
                    ComboBox {
                        id: strategyCombo
                        Layout.preferredWidth: 170
                        model: ["P2pCluster", "P2pStar", "P2pPointToPoint"]
                        currentIndex: Math.max(0, find(nearbyController.connectionStrategy))
                        onActivated: nearbyController.connectionStrategy = currentText
                    }

                    Label { text: "Device" }
                    TextField {
                        Layout.preferredWidth: 180
                        text: nearbyController.deviceName
                        onEditingFinished: nearbyController.deviceName = text
                    }

                    Label { text: "Service ID" }
                    TextField {
                        Layout.fillWidth: true
                        Layout.columnSpan: 3
                        text: nearbyController.serviceId
                        onEditingFinished: nearbyController.serviceId = text
                    }

                    Label { text: "Log Path" }
                    TextField {
                        Layout.fillWidth: true
                        Layout.columnSpan: 7
                        text: nearbyController.logPath
                        onEditingFinished: nearbyController.logPath = text
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: "Status: " + nearbyController.statusMessage
                    elide: Text.ElideRight
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Frame {
                Layout.fillHeight: true
                Layout.preferredWidth: 420
                padding: 10
                background: Rectangle {
                    color: root.surface
                    border.color: root.border
                    radius: 6
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    Label {
                        text: "API Controls"
                        font.bold: true
                    }

                    Frame {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 150
                        padding: 6
                        background: Rectangle {
                            color: root.surface
                            border.color: root.border
                            radius: 4
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 4

                            Label {
                                text: "Incoming Requests"
                                font.bold: true
                            }

                            ListView {
                                id: incomingRequestsList
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                spacing: 4
                                model: nearbyController.pendingConnections

                                delegate: Rectangle {
                                    required property string modelData
                                    width: incomingRequestsList.width
                                    height: 38
                                    color: root.surface
                                    border.color: root.border
                                    radius: 4

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 6
                                        anchors.rightMargin: 6
                                        spacing: 6

                                        Label {
                                            Layout.fillWidth: true
                                            text: root.endpointLabel(modelData)
                                            elide: Text.ElideRight
                                        }

                                        Button {
                                            text: "Accept"
                                            onClicked: nearbyController.acceptIncoming(modelData)
                                        }

                                        Button {
                                            text: "Reject"
                                            onClicked: nearbyController.rejectIncoming(modelData)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    TextField {
                        id: endpointField
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        placeholderText: "Endpoint ID (advanced)"
                        text: root.apiEndpointId
                        onTextChanged: root.apiEndpointId = text
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 6
                        rowSpacing: 6

                        Button {
                            text: "Connect"
                            Layout.fillWidth: true
                            enabled: endpointField.text.trim().length > 0
                            onClicked: nearbyController.connectToDevice(endpointField.text.trim())
                        }
                        Button {
                            text: "Disconnect"
                            Layout.fillWidth: true
                            enabled: endpointField.text.trim().length > 0
                            onClicked: nearbyController.disconnectDevice(endpointField.text.trim())
                        }
                        Button {
                            text: "Accept"
                            Layout.fillWidth: true
                            enabled: endpointField.text.trim().length > 0
                            onClicked: nearbyController.acceptIncoming(endpointField.text.trim())
                        }
                        Button {
                            text: "Reject"
                            Layout.fillWidth: true
                            enabled: endpointField.text.trim().length > 0
                            onClicked: nearbyController.rejectIncoming(endpointField.text.trim())
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        ComboBox {
                            id: targetCombo
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            model: connectedEndpointsModel
                            textRole: "label"
                            valueRole: "endpointId"
                            displayText: currentIndex >= 0 ? currentText : "Select connected device"
                        }

                        Button {
                            text: "Use ID"
                            enabled: targetCombo.currentValue !== undefined && targetCombo.currentValue !== null
                            onClicked: {
                                endpointField.text = String(targetCombo.currentValue)
                                root.apiEndpointId = endpointField.text
                            }
                        }
                    }

                    TextField {
                        id: payloadField
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        placeholderText: "Text payload"
                        text: root.apiPayloadText
                        onTextChanged: root.apiPayloadText = text
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 6
                        rowSpacing: 6

                        Button {
                            text: "Send Text"
                            Layout.fillWidth: true
                            enabled: endpointField.text.trim().length > 0 && payloadField.text.length > 0
                            onClicked: nearbyController.sendText(endpointField.text.trim(), payloadField.text)
                        }

                        Button {
                            text: "Get Medium"
                            Layout.fillWidth: true
                            enabled: endpointField.text.trim().length > 0
                            onClicked: root.queryMediumResult = nearbyController.mediumForEndpoint(endpointField.text.trim())
                        }

                        Button {
                            text: "Upgrade Bandwidth"
                            Layout.fillWidth: true
                            enabled: endpointField.text.trim().length > 0
                            onClicked: nearbyController.initiateBandwidthUpgrade(endpointField.text.trim())
                        }

                        Button {
                            text: "Get Peer Name"
                            Layout.fillWidth: true
                            enabled: endpointField.text.trim().length > 0
                            onClicked: root.queryPeerResult = nearbyController.peerNameForEndpoint(endpointField.text.trim())
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 8

                        Label { text: "Medium" }
                        Label {
                            text: root.queryMediumResult
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Label { text: "Peer" }
                        Label {
                            text: root.queryPeerResult
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 6

                        Button {
                            text: "Clear Transfers"
                            Layout.fillWidth: true
                            onClicked: nearbyController.clearTransfers()
                        }

                        Button {
                            text: "Hide To Tray"
                            Layout.fillWidth: true
                            onClicked: {
                                root.hide()
                                nearbyController.hideToTray()
                            }
                        }
                    }

                    Label {
                        text: "Payload Events"
                        font.bold: true
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: payloadEventsModel
                        spacing: 4

                        delegate: Rectangle {
                            required property var model
                            width: ListView.view.width
                            height: 44
                            color: root.surface
                            border.color: root.border
                            radius: 4

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 0

                                Label {
                                    Layout.fillWidth: true
                                    text: model.peer
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: model.type + ": " + model.value
                                    elide: Text.ElideRight
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
                    Layout.preferredHeight: 320
                    padding: 8
                    background: Rectangle {
                        color: root.surface
                        border.color: root.border
                        radius: 6
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 6

                        Label {
                            text: "Endpoints"
                            font.bold: true
                        }

                        TabBar {
                            id: endpointTabs
                            Layout.fillWidth: true

                            TabButton { text: "Discovered" }
                            TabButton { text: "Pending" }
                            TabButton { text: "Connected" }
                        }

                        StackLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            currentIndex: endpointTabs.currentIndex

                            ListView {
                                id: discoveredList
                                clip: true
                                spacing: 4
                                model: nearbyController.discoveredDevices

                                delegate: Rectangle {
                                    required property string modelData
                                    width: discoveredList.width
                                    height: 42
                                    color: root.surface
                                    border.color: root.border
                                    radius: 4

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8

                                        Label {
                                            Layout.fillWidth: true
                                            text: root.endpointLabel(modelData)
                                            elide: Text.ElideRight
                                        }

                                        Button {
                                            text: "Use"
                                            onClicked: {
                                                endpointField.text = modelData
                                                root.apiEndpointId = modelData
                                            }
                                        }

                                        Button {
                                            text: "Connect"
                                            onClicked: nearbyController.connectToDevice(modelData)
                                        }
                                    }
                                }
                            }

                            ListView {
                                id: pendingList
                                clip: true
                                spacing: 4
                                model: nearbyController.pendingConnections

                                delegate: Rectangle {
                                    required property string modelData
                                    width: pendingList.width
                                    height: 42
                                    color: root.surface
                                    border.color: root.border
                                    radius: 4

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8

                                        Label {
                                            Layout.fillWidth: true
                                            text: root.endpointLabel(modelData)
                                            elide: Text.ElideRight
                                        }

                                        Button {
                                            text: "Use"
                                            onClicked: {
                                                endpointField.text = modelData
                                                root.apiEndpointId = modelData
                                            }
                                        }

                                        Button {
                                            text: "Accept"
                                            onClicked: nearbyController.acceptIncoming(modelData)
                                        }

                                        Button {
                                            text: "Reject"
                                            onClicked: nearbyController.rejectIncoming(modelData)
                                        }
                                    }
                                }
                            }

                            ListView {
                                id: connectedList
                                clip: true
                                spacing: 4
                                model: nearbyController.connectedDevices

                                delegate: Rectangle {
                                    required property string modelData
                                    width: connectedList.width
                                    height: 46
                                    color: root.surface
                                    border.color: root.border
                                    radius: 4

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8

                                        Label {
                                            Layout.fillWidth: true
                                            text: root.endpointLabel(modelData)
                                            elide: Text.ElideRight
                                        }

                                        Label {
                                            text: nearbyController.mediumForEndpoint(modelData)
                                        }

                                        Button {
                                            text: "Use"
                                            onClicked: {
                                                endpointField.text = modelData
                                                root.apiEndpointId = modelData
                                            }
                                        }

                                        Button {
                                            text: "Disconnect"
                                            onClicked: nearbyController.disconnectDevice(modelData)
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
                    padding: 8
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
                                onClicked: nearbyController.clearTransfers()
                            }
                        }

                        ListView {
                            id: transferList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 6
                            model: nearbyController.transfers

                            delegate: Rectangle {
                                required property var modelData
                                width: transferList.width
                                height: 86
                                color: root.surface
                                border.color: root.border
                                radius: 4

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true

                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.direction + " | " + root.endpointLabel(modelData.endpointId) + " | payload " + modelData.payloadId
                                            elide: Text.ElideRight
                                        }

                                        Label { text: modelData.status }
                                        Label { text: modelData.medium }
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
                                        text: root.formatBytes(modelData.bytesTransferred) + " / " + root.formatBytes(modelData.totalBytes)
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

    Connections {
        target: nearbyController

        function onConnectedDevicesChanged() {
            root.refreshConnectedEndpointsModel()
        }

        function onPendingConnectionsChanged() {
            if (nearbyController.pendingConnections.length > 0) {
                endpointTabs.currentIndex = 1
            }
        }

        function onPayloadReceived(endpoint_id, type, value) {
            payloadEventsModel.insert(0, {
                "peer": root.endpointLabel(endpoint_id),
                "type": type,
                "value": String(value)
            })
            if (payloadEventsModel.count > 200) {
                payloadEventsModel.remove(payloadEventsModel.count - 1)
            }
        }

        function onModeChanged() {
            modeCombo.currentIndex = nearbyController.mode === "Send" ? 1 : 0
        }
    }

    Component.onCompleted: refreshConnectedEndpointsModel()
}
