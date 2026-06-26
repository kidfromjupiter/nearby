import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes

RowLayout {
    anchors.fill: parent
    spacing: 0

    Sidebar {}
    StackLayout {
        id: contentStack
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: 1

        Drop {}

        Rectangle {
            color: "transparent"
            Layout.fillWidth: true
            Layout.fillHeight: true

            Targets{}

        }
        Rectangle {
            color: "transparent"

            Text {
                text: "User Profile Management"
                anchors.centerIn: parent
                font.pointSize: 22
                color: "#333333"
            }
        }
    }
}
