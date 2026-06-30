import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes

Rectangle {
    id: root
    color: "#DCF5FF"
    Layout.fillWidth: true
    Layout.fillHeight: true

    property string filename: "ThisIsAnImage.jpg"
    property string targetname: "Lasan's A55"
    property bool transferring: true
    property real progressValue: 0.64
    property string statusText: "Receiving file"

    Item {
        visible: root.transferring
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.9, 700)
        height: 212

        Rectangle {
            anchors.fill: parent
            radius: 24
            color: "#FFFFFF"
            border.color: "#BCE5F5"
            border.width: 1
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 28
            spacing: 24

            Rectangle {
                Layout.preferredWidth: 96
                Layout.preferredHeight: 96
                radius: 24
                color: "#E8F7FC"

                Image {
                    anchors.centerIn: parent
                    width: 60
                    height: 60
                    source: "qrc:/icons/file.svg"
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: width
                    sourceSize.height: height
                    smooth: true
                    antialiasing: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 10

                Text {
                    text: root.targetname
                    font.pointSize: 17
                    font.weight: 700
                    color: "#1A1C1E"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: root.filename
                    color: "#57707A"
                    font.pointSize: 12
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                ProgressBar {
                    id: transferProgress
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    value: Math.max(0, Math.min(1, root.progressValue))

                    background: Rectangle {
                        implicitHeight: 10
                        radius: 5
                        color: "#DDEEF5"
                    }

                    contentItem: Item {
                        Rectangle {
                            width: transferProgress.visualPosition * parent.width
                            height: 10
                            radius: 5
                            color: "#0D6D90"
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: root.statusText
                        color: "#57707A"
                        font.pointSize: 12
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Text {
                        text: Math.round(Math.max(0, Math.min(1, root.progressValue)) * 100) + "%"
                        color: "#0D6D90"
                        font.pointSize: 13
                        font.weight: 700
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    Item {
                        Layout.fillWidth: true
                    }

                    Button {
                        id: cancelButton
                        text: "Cancel"

                        contentItem: Text {
                            text: cancelButton.text
                            font.pointSize: 14
                            font.weight: 600
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            implicitWidth: 118
                            implicitHeight: 46
                            radius: 12
                            color: cancelButton.hovered ? "#B62828" : "#D94C4C"
                        }
                    }
                }
            }
        }
    }

    Item {
        visible: !root.transferring
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.85, 360)
        height: 460

        Rectangle {
            id: cardBackground
            anchors.fill: parent
            radius: 24
            color: "#FFFFFF"
            border.color: "#BCE5F5"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 28
            spacing: 16

            Text {
                Layout.fillWidth: true
                font.pointSize: 18
                font.weight: 700
                color: "#377B95"
                text: "Incoming share"
                horizontalAlignment: Text.AlignHCenter
            }

            Image {
                source: "qrc:/icons/file.svg"
                Layout.fillHeight: true
                Layout.fillWidth: true
                fillMode: Image.PreserveAspectCrop
                sourceSize.height: height
                sourceSize.width: width
                smooth: true
                antialiasing: true
            }

            Text {
                Layout.fillWidth: true
                color: "#57707A"
                font.pointSize: 11
                wrapMode: Text.WrapAnywhere
                text: root.filename
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 2
                elide: Text.ElideMiddle
            }

            Text {
                Layout.fillWidth: true
                color: "#57707A"
                font.pointSize: 15
                font.weight: 700
                wrapMode: Text.WrapAnywhere
                text: root.targetname
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 2
                elide: Text.ElideMiddle
            }

            RowLayout {
                Button {
                    id: cancelbutton
                    Layout.fillWidth: true
                    text: "Cancel"

                    contentItem: Text {
                        text: cancelbutton.text
                        font.pointSize: 14
                        font.weight: 600
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitHeight: 46
                        radius: 12
                        color: cancelbutton.hovered ? "#db1e1e" : "#6EA7B6"
                        border.color: "#91C8DE"
                        border.width: 1
                    }
                }

                Button {
                    id: acceptButton
                    Layout.fillWidth: true
                    text: "Accept"

                    contentItem: Text {
                        text: acceptButton.text
                        font.pointSize: 14
                        font.weight: 600
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitHeight: 46
                        radius: 12
                        color: acceptButton.hovered ? "#195871" : "#06384C"
                        border.color: "#91C8DE"
                        border.width: 1
                    }
                }
            }
        }
    }
}
