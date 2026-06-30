import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "."

Rectangle {
    id: rootItem
    // Explicit sizes so parent containers know how to space them
    width: 100
    height: 110 // Reduced slightly since the progress outer bounds are gone
    color: "transparent"

    // --- CUSTOM ARGUMENTS (PROPERTIES) ---
    property string deviceName: "Unknown Device"
    property string iconSource: "qrc:icons/smartphone.svg"
    property var shareTargetId: 0
    property bool hovered: targetMouseArea.containsMouse

    ColumnLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 5


        // --- ICON CONTAINER ---
        Item {
            id: iconContainer
            implicitWidth: 60
            implicitHeight: 60
            Layout.alignment: Qt.AlignHCenter


            // Icon Circle Background
            Rectangle {
                id: iconCircle
                anchors.fill: parent
                color: rootItem.hovered ? "#195871" : "#6EA7B6"
                radius: width / 2

                Behavior on color {
                    ColorAnimation {
                        duration: 80
                        easing.type: Easing.OutQuad
                    }
                }

                Button {
                    id: iconButton
                    icon.source: rootItem.iconSource
                    anchors.fill: parent
                    anchors.margins: 5
                    icon.color: "white"
                    icon.height: iconCircle.height 
                    icon.width: iconCircle.width
                    
                    background: Rectangle {
                        color: "transparent"
                    }
                }
            }

            MouseArea {
                id: targetMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    EventBus.shareTargetSelected(rootItem.shareTargetId);
                }
            }
        }

        // --- TEXT COMPONENT ---
        Text {
            text: rootItem.deviceName
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: rootItem.hovered ? "#06384C" : "#1A1C1E"

            Behavior on color {
                ColorAnimation {
                    duration: 120
                    easing.type: Easing.OutQuad
                }
            }
        }
    }

}
