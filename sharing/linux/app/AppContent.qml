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
        currentIndex: 2

        Drop {}

        Rectangle {
            color: "transparent"
            Layout.fillWidth: true
            Layout.fillHeight: true

            Targets {}
        }
        IncomingShare {
            filename: "VacationPhoto_2026.jpg"
            targetname: "Lasan's A55"
            progressValue: 0.64
            statusText: "Receiving 1 of 1 items"
            transferring: true
        }

    }
}
