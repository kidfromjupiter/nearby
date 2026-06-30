import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Row {
    id: shareTargetsRow
    spacing: 10

    Repeater {
        model: backend.targets

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
