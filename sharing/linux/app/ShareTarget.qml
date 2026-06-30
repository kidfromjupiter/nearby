import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: rootItem
    // Explicit sizes so parent containers know how to space them
    width: 100
    height: 110 // Reduced slightly since the progress outer bounds are gone
    color: "transparent"

    // --- CUSTOM ARGUMENTS (PROPERTIES) ---
    property string deviceName: "Unknown Device"
    property string iconSource: "qrc:icons/smartphone.svg"

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
                color: "#6EA7B6"
                radius: width / 2

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
        }

        // --- TEXT COMPONENT ---
        Text {
            text: rootItem.deviceName
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: "#1A1C1E" // Using the text charcoal color from your palette
        }
    }
}
