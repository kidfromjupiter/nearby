import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes
import "."

Rectangle {
    id: sidebar
    Layout.fillHeight: true
    Layout.preferredWidth: 300

    property string pendingPath: ""

    color: "#CBF0FF"

    ColumnLayout {
        anchors.margins: 20
        anchors.fill: parent

        Rectangle {
            Layout.preferredHeight: innerColumn.implicitHeight + 20
            Layout.fillWidth: true
            color: "transparent"

            ColumnLayout {
                id: innerColumn
                Text {
                    text: "Device name"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                Text {
                    text: "lasans-laptop"
                    font.weight: 500
                    font.pointSize: 17
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"

            ColumnLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                //anchors.fill: parent

                Text {
                    text: "Visible to Everyone"
                    font.weight: 700
                    color: "#377B95"
                }

                Text {
                    Layout.preferredWidth: parent.width
                    wrapMode: Text.WordWrap
                    text: "This cannot be changed due to limitations in QuickShare on Linux"
                    color: "gray"
                }
            }
        }

        Rectangle {
            visible: pendingPath.length != 0
            Layout.preferredHeight: 350
            Layout.fillWidth: true
            border.color: "#91C8DE"
            border.width: 2
            radius: 20
            color: "#DCF5FF"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15

                Text {
                    Layout.fillWidth: true
                    font.pointSize: 15

                    font.weight: 700
                    color: "#377B95"
                    text: "Sharing"
                }
                Image {
                    source: "qrc:icons/file.svg"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    fillMode: Image.PreserveAspectCrop
                    sourceSize.height: height
                    sourceSize.width: width
                    smooth: true
                    antialiasing: true
                }

                Text {
                    Layout.fillWidth: true
                    color: "gray"
                    wrapMode: Text.WordWrap
                    text: decodeURIComponent(pendingPath).replace("file://", "")
                    horizontalAlignment: Text.AlignHCenter
                }
                Button {
                    id: cancelbutton
                    Layout.fillWidth: true
                    text: "Cancel"
                    onClicked: EventBus.cancelPendingShareRequested()
                    contentItem: Text {
                        text: cancelbutton.text
                        font.pointSize: 14
                        font.weight: 500
                        color: "white" // Dims text when pressed
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 100
                        implicitHeight: 40
                        radius: 10
                        color: cancelbutton.hovered ? "#195871" : "#06384C"
                        border.color: "#91C8DE"
                        border.width: 2
                    }
                }
            }
        }
    }
}
