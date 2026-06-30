import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Row {
    id: shareTargetsRow
    spacing: 10

    property bool useDummyTargets: true

    ListModel {
        id: dummyTargets

        ListElement {
            deviceName: "Lasan's Pixel"
            type: 1
        }
        ListElement {
            deviceName: "Work Laptop"
            type: 2
        }
        ListElement {
            deviceName: "Kitchen Tablet"
            type: 3
        }
        ListElement {
            deviceName: "A55"
            type: 1
        }
        ListElement {
            deviceName: "Long Device Name For Wrapping"
            type: 2
        }
    }

    Repeater {
        model: shareTargetsRow.useDummyTargets ? dummyTargets : backend.targets

        Rectangle {
            width: 100
            height: 130
            color: "transparent"

            ColumnLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 5

                ShareTarget {
                    deviceName: model.deviceName
                    iconSource: {
                        if (model.type === 2)
                            return "qrc:icons/laptop.svg"
                        if (model.type === 3)
                            return "qrc:icons/tablet.svg"
                        return "qrc:icons/smartphone.svg"
                    }
                }
            }
        }
    }
}
