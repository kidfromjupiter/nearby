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
            targetId: 1
            deviceName: "Lasan's Pixel"
            type: 1
        }
        ListElement {
            targetId: 2
            deviceName: "Work Laptop"
            type: 2
        }
        ListElement {
            targetId: 3
            deviceName: "Kitchen Tablet"
            type: 3
        }
        ListElement {
            targetId: 4
            deviceName: "A55"
            type: 1
        }
        ListElement {
            targetId: 5
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
                    shareTargetId: shareTargetsRow.useDummyTargets ? model.targetId : model.id
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
