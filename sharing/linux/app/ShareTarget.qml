import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes

Rectangle {
    id: rootItem
    // Explicit sizes so parent containers know how to space them
    width: 100
    height: 130
    color: "transparent"

    // --- CUSTOM ARGUMENTS (PROPERTIES) ---
    property string deviceName: "Unknown Device"
    property real progressValue: 0.0 // Value between 0.0 and 1.0
    property string iconSource: "qrc:icons/smartphone.svg"

    ColumnLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 5

        // --- ICON CONTAINER WITH PROGRESS RINGS ---
        Item {
            id: iconContainer
            implicitWidth: 72
            implicitHeight: 72
            Layout.alignment: Qt.AlignHCenter

            // Target angle calculated using the custom dynamic argument
            property real targetAngle: 360 * rootItem.progressValue
            
            Behavior on targetAngle {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.OutQuad
                }
            }

            // Background Gray Track Ring
            Shape {
                anchors.fill: parent
                layer.enabled: true
                layer.samples: 8
                antialiasing: true

                ShapePath {
                    strokeColor: "#E0E4E8"
                    strokeWidth: 4
                    fillColor: "transparent"

                    PathAngleArc {
                        centerX: iconContainer.width / 2
                        centerY: iconContainer.height / 2
                        radiusX: 32
                        radiusY: 32
                        startAngle: -90
                        sweepAngle: 360
                    }
                }
            }

            // Active Progress Ring
            Shape {
                anchors.fill: parent
                layer.enabled: true
                layer.samples: 8
                antialiasing: true

                ShapePath {
                    strokeColor: "#00658F"
                    strokeWidth: 4
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap

                    PathAngleArc {
                        centerX: iconContainer.width / 2
                        centerY: iconContainer.height / 2
                        radiusX: 32
                        radiusY: 32
                        startAngle: -90
                        sweepAngle: iconContainer.targetAngle
                    }
                }
            }

            // Icon Circle
            Rectangle {
                width: 60
                height: 60
                anchors.centerIn: parent
                color: "#6EA7B6"
                radius: 30

                Button {
                    icon.source: rootItem.iconSource
                    anchors.fill: parent
                    anchors.margins: 5
                    icon.color: "white"
                    icon.height: height
                    icon.width: width
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
