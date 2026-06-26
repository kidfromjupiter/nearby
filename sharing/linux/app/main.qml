import QtQuick
import QtQuick.Window

Window {
    id: root
    width: 640
    height: 480
    visible: true
    title: qsTr("QuickShare")
    color: "#DCF5FF"

    FontLoader {
        id: googlesans
        source: "qrc:/googlesans_var.ttf"
    }

    Loader {
        id: contentLoader
        anchors.fill: parent
        source: "AppContent.qml"
    }

    function reloadInnerContent() {
        const nextSource = "AppContent.qml?rev=" + Date.now()
        contentLoader.active = false
        contentLoader.source = ""
        contentLoader.active = true
        contentLoader.source = nextSource
    }
}
