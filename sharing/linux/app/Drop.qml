import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes

Rectangle {
    id: dropZone
    color: "transparent"
    DropArea {
        anchors.fill: parent
        // Visual feedback: Change color when an item is hovered over it
        onEntered: drag => {
            dropZone.color = "#91C8DE";
        }

        onExited: {
            dropText.text = "Drag and drop files to share";
            dropZone.color = "transparent";
        }

        // The action: What happens when the user releases the item here
        onDropped: drop => {
            dropZone.color = "#98FB98"; // Success Mint Green
            dropText.text = "Dropped: " + drop.source.objectName;

            // Snap the dragged item exactly to the center of the drop zone
            drop.source.x = dropZone.x + (dropZone.width - drop.source.width) / 2;
            drop.source.y = dropZone.y + (dropZone.height - drop.source.height) / 2;

            drop.accept(); // Tell Qt the drop event was handled successfully
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
