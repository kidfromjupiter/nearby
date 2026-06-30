import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    color: "transparent"
    Layout.fillWidth: true
    Layout.fillHeight: true

    Rectangle {
        id: pulseButton
        property real pulseSize: Math.min(parent.width, parent.height) * 0.5

        width: pulseSize
        height: pulseSize
        radius: pulseSize / 2
        color: "#bde8f9"
        anchors.centerIn: parent

        SequentialAnimation {
            id: pulseAnimation
            loops: Animation.Infinite
            running: true

            ParallelAnimation {
                NumberAnimation {
                    target: pulseButton
                    property: "scale"
                    from: 0.75
                    to: 1.5
                    duration: 600
                    easing.type: Easing.InOutSine
                }

                NumberAnimation {
                    target: pulseButton
                    property: "opacity"
                    from: 1.0
                    to: 0
                    duration: 600
                    easing.type: Easing.InOutSine
                }
            }

            ParallelAnimation {
                NumberAnimation {
                    target: pulseButton
                    property: "scale"
                    from: 1.5
                    to: 0.75
                    duration: 1500
                    easing.type: Easing.InOutSine
                }

                NumberAnimation {
                    target: pulseButton
                    property: "opacity"
                    from: 0
                    to: 1.0
                    duration: 1500
                    easing.type: Easing.InOutSine
                }
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width - 64
        spacing: 40

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 10

            Text {
                text: "Searching for devices..."
                font.pointSize: 24
                font.weight: 600
                color: "#377B95"
            }
        }

        Targets {
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
