pragma Singleton

import QtQuick

QtObject {
    signal fileSelected(string filePath)
    signal cancelPendingShareRequested()
    signal shareTargetSelected(var shareTargetId)
}
