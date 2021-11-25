import QtQuick 2.0
import QtMultimedia 5.0

Rectangle {
    id:root
    color:"black"

    MediaPlayer {
        id:camera
        objectName: qsTr("camera")
        autoLoad: false

        onError: {
            if (MediaPlayer.NoError != error) {
                console.log("[qmlvideo] VideoItem.onError error " + error + " errorString " + errorString)
                root.fatalError()
            }
        }
    }

    VideoOutput {
        id: video
        objectName: qsTr("cameraContent")
        anchors.fill: parent
        source: camera
    }
}

