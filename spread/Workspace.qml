import Qt 4.7

Item {
    id: workspace

    property alias applicationId: spread.applicationId
    property int row
    property int column

    z: 1

    Connections {
        target: control
        onActivateSpread: {
            spread.applicationId = applicationId
            spread.activateSpread()
        }
    }

    transformOrigin: Item.TopLeft
    x: column * childrenRect.width
    y: row * childrenRect.height

    onStateChanged: if (state == "screen") spread.cancelSpread()

    Spread {
        id: spread
        onExiting: workspace.state = "screen"
        onBackgroundClicked: {
            if (switcher.zoomedWorkspace) {
                // If another workspace is zoomed, cancel the current zoom.
                // If we are zooomed, then exit to our workspace.
                if (switcher.zoomedWorkspace != workspace)
                    switcher.zoomedWorkspace.state = ""
                else workspace.state = "screen"
                switcher.zoomedWorkspace = null
            } else if (workspace.state == "") {
                workspace.state = "zoomed"
                switcher.zoomedWorkspace = workspace
            }
        }
    }

    states: [
        State {
            name: "zoomed"
            PropertyChanges {
                target: workspace
                scale: switcher.zoomedScale
                z: 2
                x: switcher.leftMargin
                y: switcher.topMargin
            }
        },
        State {
            name: "screen"
            PropertyChanges {
                target: workspace
                scale: 1.0
                z: 2
                x: 0
                y: 0
            }
        }
    ]

    transitions: [
        Transition {
            SequentialAnimation {
                PropertyAction { target: workspace; property: "z"; value: 2 }
                NumberAnimation {
                    target: workspace
                    properties: "x,y,scale"
                    duration: 250
                    easing.type: Easing.InOutSine
                }
                PropertyAction { target: workspace; property: "z"; value: 2 }
            }
        }
    ]
}
