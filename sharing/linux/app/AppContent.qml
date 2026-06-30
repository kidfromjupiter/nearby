import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes

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
    Component.onCompleted: {
        backend.startReceive();
    }
    Component.onDestruction: {
        backend.stopReceive();
        backend.stopDiscovery();
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
        onCancelPendingShareRequested: {
            top.cancelPendingShare();
        }
    }
    StackLayout {
        id: contentStack
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: top.currentIndex

        Drop {
            onFileSelected: path => {
                top.pendingPath = path;
                startSharing();
            }
        }

        Rectangle {
            color: "transparent"
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                id: pulseButton
                property real pulseSize: Math.min(parent.width, parent.height) * 0.5

                width: pulseSize
                height: pulseSize
                radius: pulseSize / 2
                color: "#bde8f9"
                anchors.centerIn: parent

                SequentialAnimation {
                    id: pulseAnimation
                    loops: Animation.Infinite
                    running: true

                    ParallelAnimation {
                        NumberAnimation {
                            target: pulseButton
                            property: "scale"
                            from: 1.0
                            to: 1.5
                            duration: 1000
                            easing.type: Easing.InOutSine
                        }

                        NumberAnimation {
                            target: pulseButton
                            property: "opacity"
                            from: 1.0
                            to: 0
                            duration: 1000
                            easing.type: Easing.InOutSine
                        }
                    }

                    ParallelAnimation {
                        NumberAnimation {
                            target: pulseButton
                            property: "scale"
                            from: 1.5
                            to: 1.0
                            duration: 1000
                            easing.type: Easing.InOutSine
                        }

                        NumberAnimation {
                            target: pulseButton
                            property: "opacity"
                            from: 0
                            to: 1.0
                            duration: 1000
                            easing.type: Easing.InOutSine
                        }
                    }
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width - 64
                spacing: 40

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 10

                    Text {
                        text: "Searching for devices..."
                        font.pointSize: 24
                        font.weight: 600
                        color: "#377B95"
                    }
                }

                Targets {
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        IncomingShare {
            filename: top.filename
            targetname: top.targetname
            progressValue: top.progressValue
            statusText: top.statusText
            transferring: top.transferring
        }
    }
}
