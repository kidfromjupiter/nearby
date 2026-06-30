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

    Connections {
      target: backend

      function onStatusTextChanged(){
          console.log(backend.statusText)
      }
    }
    //Component.onCompleted: {
    //    backend.startReceive();
    //}
    //Component.onDestruction: {
    //    backend.stopReceive();
    //}


    Sidebar {}
    StackLayout {
        id: contentStack
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: 0

        Drop {
        }

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
