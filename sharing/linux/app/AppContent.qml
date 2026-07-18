import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes
import "."

RowLayout {
    id: topLayout
    anchors.fill: parent
    spacing: 0

    property string pendingPath: ""
    property bool pendingTransfer: false
    property bool incomingShare: false
    property int currentIndex: 0
    property var selectedTransferId: 0

    Connections {
        target: backend

        function onIncomingTransfer(share_target_id) {
            console.log("Getting incoming share");
            console.log("selected transfer: " + share_target_id)
            selectedTransferId = share_target_id;
            currentIndex = 2;
        }
    }

    Connections {
        target: EventBus

        function onFileSelected(path) {
            topLayout.pendingPath = path;
            startSharing();
        }

        function onShareTargetSelected(shareTargetId) {
            if (topLayout.pendingPath.length === 0) {
                return;
            }
            topLayout.selectedTransferId = shareTargetId;
            backend.sendFile(shareTargetId, topLayout.pendingPath);
            topLayout.currentIndex = 2;
        }

        function onCancelPendingShareRequested() {
            topLayout.cancelPendingShare();
        }
    }

    function cancelPendingShare() {
        pendingPath = "";
        selectedTransferId = 0;
        currentIndex = 0;
        backend.startReceive();
    }

    function startSharing() {
        backend.startDiscovery();
        currentIndex = 1;
    }

    Sidebar {
        pendingPath: topLayout.pendingPath
    }
    StackLayout {
        id: contentStack
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: topLayout.currentIndex

        Drop {}

        SearchingDevices {}

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Repeater {
                id: transferRepeater
                model: backend.transfers

                IncomingShare {
                    anchors.fill: parent

                    Component.onCompleted: {
                      console.log("transferId: " + model.transferId)
                      console.log("selectedTransferId: " + topLayout.selectedTransferId)
                      console.log(model.transferId == topLayout.selectedTransferId)

                    }

                    visible: topLayout.selectedTransferId == 0 || model.transferId == topLayout.selectedTransferId
                    shareTargetId: model.transferId
                    direction: model.direction
                    filename: model.localPath
                    targetname: model.deviceName
                    progressValue: model.progress
                    status: model.status
                    totalBytes: model.totalBytes
                    transferredBytes: model.transferredBytes
                    totalAttachmentsCount: model.totalAttachmentsCount
                    transferredAttachmentsCount: model.transferredAttachmentsCount
                    isFinalStatus: model.isFinalStatus
                    awaitingLocalConfirmation: model.awaitingLocalConfirmation
                }
            }

            Rectangle {
                anchors.fill: parent
                color: "#DCF5FF"
                visible: transferRepeater.count === 0

                Text {
                    anchors.centerIn: parent
                    text: "Preparing transfer..."
                    color: "#377B95"
                    font.pointSize: 18
                    font.weight: 600
                }
            }
        }
    }
}
