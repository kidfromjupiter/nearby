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
    property int currentIndex: 0
    property var selectedTransferId: 0
    property var activeOutgoingTransferId: 0
    property bool outgoingStartFailed: false
    readonly property bool outgoingTransferActive: activeOutgoingTransferId != 0

    Connections {
        target: backend

        function onIncomingTransfer(share_target_id) {
            console.log("Getting incoming share");
            console.log("selected transfer: " + share_target_id)
            selectedTransferId = share_target_id;
            currentIndex = 2;
        }

        function onOutgoingTransferStartFailed(share_target_id) {
            if (share_target_id == topLayout.activeOutgoingTransferId) {
                topLayout.outgoingStartFailed = true;
            }
        }
    }

    Connections {
        target: EventBus

        function onFileSelected(path) {
            topLayout.pendingPath = path;
            startSharing();
        }

        function onShareTargetSelected(shareTargetId) {
            if (topLayout.pendingPath.length === 0
                    || topLayout.outgoingTransferActive) {
                return;
            }
            if (backend.sendFile(shareTargetId, topLayout.pendingPath)) {
                topLayout.outgoingStartFailed = false;
                topLayout.activeOutgoingTransferId = shareTargetId;
            }
        }

        function onCancelPendingShareRequested() {
            topLayout.cancelPendingShare();
        }
    }

    function cancelPendingShare() {
        if (outgoingTransferActive) {
            return;
        }
        returnHome();
    }

    function finishOutgoingTransfer(shareTargetId) {
        if (shareTargetId != activeOutgoingTransferId) {
            return;
        }
        activeOutgoingTransferId = 0;
        outgoingStartFailed = false;
    }

    function returnHome() {
        pendingPath = "";
        selectedTransferId = 0;
        activeOutgoingTransferId = 0;
        outgoingStartFailed = false;
        currentIndex = 0;
        backend.startReceive();
    }

    function startSharing() {
        backend.startDiscovery();
        currentIndex = 1;
    }

    Sidebar {
        pendingPath: topLayout.pendingPath
        cancelEnabled: !topLayout.outgoingTransferActive
    }

    Item {
        id: contentPane
        Layout.fillWidth: true
        Layout.fillHeight: true

        StackLayout {
            id: contentStack
            anchors.fill: parent
            currentIndex: topLayout.currentIndex

            Drop {}

            SearchingDevices {
                transferLocked: topLayout.outgoingTransferActive
                activeTransferId: topLayout.activeOutgoingTransferId
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Repeater {
                    id: transferRepeater
                    model: backend.transfers

                    IncomingShare {
                        anchors.fill: parent

                        visible: model.direction === "receive"
                                 && model.transferId == topLayout.selectedTransferId
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
                        onReturnHomeRequested: topLayout.returnHome()
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

        Repeater {
            model: backend.transfers

            Item {
                id: outgoingNotification

                readonly property bool terminal: model.isFinalStatus
                                                 || topLayout.outgoingStartFailed
                property bool cancelPending: false
                property bool exiting: false

                width: Math.min(contentPane.width * 0.9, 700)
                height: 156
                x: (contentPane.width - width) / 2
                y: -height - 8
                z: 100
                visible: topLayout.currentIndex === 1
                         && model.direction === "send"
                         && model.transferId == topLayout.activeOutgoingTransferId

                function showCard() {
                    if (!visible) {
                        return;
                    }
                    exiting = false;
                    cancelPending = false;
                    finalStatusTimer.stop();
                    exitAnimation.stop();
                    y = -height - 8;
                    enterAnimation.restart();
                    if (terminal) {
                        finalStatusTimer.restart();
                    }
                }

                onVisibleChanged: {
                    if (visible) {
                        Qt.callLater(showCard);
                    } else {
                        finalStatusTimer.stop();
                    }
                }

                onTerminalChanged: {
                    if (visible && terminal) {
                        finalStatusTimer.restart();
                    }
                }

                Component.onCompleted: {
                    if (visible) {
                        Qt.callLater(showCard);
                    }
                }

                TransferCard {
                    anchors.fill: parent
                    compact: true
                    shareTargetId: model.transferId
                    direction: model.direction
                    filename: model.localPath
                    targetname: model.deviceName
                    progressValue: model.progress
                    status: topLayout.outgoingStartFailed ? "kStartFailed" : model.status
                    isFinalStatus: outgoingNotification.terminal
                    cancelEnabled: !outgoingNotification.cancelPending
                    totalBytes: model.totalBytes
                    totalAttachmentsCount: model.totalAttachmentsCount
                    transferredAttachmentsCount: model.transferredAttachmentsCount
                    onCancelRequested: {
                        outgoingNotification.cancelPending = true;
                        backend.cancel(model.transferId);
                    }
                }

                NumberAnimation {
                    id: enterAnimation
                    target: outgoingNotification
                    property: "y"
                    to: 16
                    duration: 300
                    easing.type: Easing.OutCubic
                }

                Timer {
                    id: finalStatusTimer
                    interval: 2000
                    repeat: false
                    onTriggered: {
                        outgoingNotification.exiting = true;
                        exitAnimation.restart();
                    }
                }

                NumberAnimation {
                    id: exitAnimation
                    target: outgoingNotification
                    property: "y"
                    to: -outgoingNotification.height - 8
                    duration: 250
                    easing.type: Easing.InCubic
                    onStopped: {
                        if (outgoingNotification.exiting) {
                            outgoingNotification.exiting = false;
                            topLayout.finishOutgoingTransfer(model.transferId);
                        }
                    }
                }
            }
        }
    }
}
