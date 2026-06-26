import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Row {
    id: shareTargetsRow
    spacing: 10

    Repeater {
        model: ["Lasan's A55", "Lasan's Tab S9+", "lasan-laptop", "Somethign else"]

        // Your original delegate structure
        Rectangle {
            // Note: You must give the Rectangle an explicit size
            // so the Row knows how to position them.
            width: 100
            height: 130
            color: "transparent"

            ColumnLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 5

                ShareTarget{
                  progressValue: 0.7

                }

            }
        }
    }
}
