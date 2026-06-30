import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes
import Qt.labs.platform as Platform
import QtCore

Rectangle {
    id: dropZone
    signal fileSelected(string filePath)
    color: "transparent"
    Platform.FileDialog {
        id: fileDialog
        title: "Choose file to share"
        folder: StandardPaths.writableLocation(StandardPaths.HomeLocation)

        nameFilters: ["All files (*)"]

        onAccepted: {
            fileSelected(fileDialog.file);
        }
        onRejected: {
            console.log("File picker canceled");
        }
    }
    //signal fileDropped(string path)
    DropArea {
        anchors.fill: parent

        onExited: {
            dropText.text = "Drag and drop files to share";
            dropZone.color = "transparent";
        }
        onEntered: k => {
            console.log("Entered");
        }

        // The action: What happens when the user releases the item here

        onDropped: drop => {
            drop.acceptProposedAction();
            if (drop.hasUrls) {
                console.log(drop.urls[0].toString());
                for (let i of drop.formats) {
                    console.log(i);
                }
            }
        }
    }
    ColumnLayout {

        anchors.centerIn: parent
        Button {
            icon.source: "qrc:icons/up_file.svg"
            icon.height: 80
            icon.width: 80
            icon.color: "#195871"
            Layout.fillWidth: true
            background: Rectangle {
                color: "transparent"
            }
        }

        Text {
            id: dropText
            text: "Drag and drop files to share"
            font.pointSize: 18
            color: "#333333"
        }
        Text {
            id: subText
            text: "Click to open file picker"
            font.pointSize: 12
            color: "gray"
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor // Changes cursor to a hand on hover
        onClicked: {
            fileDialog.open(); // Opens the native OS file picker
        }
    }
    Canvas {
        id: dashedBorderCanvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);

            // Setup styling parameters
            ctx.strokeStyle = "#91C8DE";
            ctx.lineWidth = 3;

            // Set the dash pattern array: [length of dash, length of space]
            ctx.setLineDash([8, 6]);

            // Draw a rectangle border (X, Y, Width, Height)
            // Offset by half line-width (1px) so the line doesn't get clipped on the edges
            ctx.strokeRect(80, 80, width - 160, height - 160);
        }
    }
}
