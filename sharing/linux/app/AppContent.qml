import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes
import "."

RowLayout {
    id: top
    anchors.fill: parent
    spacing: 0

    property string pendingPath: ""
    property bool pendingTransfer: false
    property bool incomingShare: false
    property int currentIndex: 1

    property string filename: "VacationPhoto_2026.jpg"
    property string targetname: "Lasan's A55"
    property real progressValue: 0.64
    property string statusText: "Receiving 1 of 1 items"
    property bool transferring: true

    Connections {
        target: backend

        function onStatusTextChanged() {
            console.log(backend.statusText);
        }
        function onIncomingTransfer(share_target_id, device_name, status) {
            console.log("Getting incoming share");
            currentIndex = 2;
            transferring = false;
        }
    }

    Connections {
        target: EventBus

        function onFileSelected(path) {
            top.pendingPath = path;
            startSharing();
        }

        function onCancelPendingShareRequested() {
            top.cancelPendingShare();
        }
    }

    Component.onCompleted: {
        backend.startReceive();
    }

    function cancelPendingShare() {
        pendingPath = "";
        currentIndex = 0;
        backend.stopDiscovery();
        backend.startReceive();
    }

    function startSharing() {
        backend.stopReceive();
        backend.startDiscovery();
        currentIndex = 1;
    }

    Sidebar {
        pendingPath: top.pendingPath
    }
    StackLayout {
        id: contentStack
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: top.currentIndex

        Drop {}

        SearchingDevices {}

        IncomingShare {
            filename: top.filename
            targetname: top.targetname
            progressValue: top.progressValue
            statusText: top.statusText
            transferring: top.transferring
        }
    }
}
