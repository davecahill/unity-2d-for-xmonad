import Qt 4.7

/*
Each SpreadWindow represents a real window on the desktop (we are
passed a WindowInfo object with all the information about it).

Its state ("" or "spread") decides which mode the window should
follow to position itself on screen ("screen" or "spread" mode
respectively).

In screen mode we use the real window's position and size to exactly mimic it.

In spread mode, we are assigned a cell in the spread, and we resize
and reposition ourselves to be fully constrained and centered inside
that specific cell.
The shot should occupy as much space as possible inside the cell,
but never be bigger than its original window's size, and always
maintain the same aspect ratio as the original window.
*/

Item {
    id: window

    property variant windowInfo
    property int transitionDuration

    /* The following group of properties is the only thing needed to position
       this window in screen mode (exactly where the real window is).
       Note that we subtract the availableGeometry x and y since window.location is
       expressed in global screen coordinates. */
     property int realX: (windowInfo.position.x - screen.availableGeometry.x)
     property int realY: (windowInfo.position.y - screen.availableGeometry.y)
     property int realWidth: windowInfo.size.width
     property int realHeight: windowInfo.size.height
     property int realZ: windowInfo.z

    /* These values are applied only when in spread state */
    property int minMargin: 20
    property int availableWidth: cell.width - minMargin
    property int availableHeight: cell.height - minMargin

    /* Scale down to fit availableWidth/availableHeight while preserving the aspect
       ratio of the window. Never scale up the window. */
    property double availableAspectRatio: availableWidth / availableHeight
    property double windowAspectRatio: realWidth / realHeight
    property bool isHorizontal: windowAspectRatio >= availableAspectRatio
    property int maxWidth: Math.min(realWidth, availableWidth)
    property int maxHeight: Math.min(realHeight, availableHeight)
    property int spreadWidth: isHorizontal ? maxWidth : maxHeight * windowAspectRatio
    property int spreadHeight: !isHorizontal ? maxHeight : maxWidth / windowAspectRatio

    /* In the default state the spread window is exactly at the same position an size as
       the real windwo */
    x: realX
    y: realY
    width: realWidth
    height: realHeight
    z: realZ

    /* Maintain the selection status of this item to adjust visual appearence,
       but never change it from inside the component. Since all selection logic
       need to be managed outside of the component due to interaction with keyboard,
       we just forward mouse signals. */
    property bool isSelected: false

    signal clicked
    signal entered
    signal exited

    property bool enableBehaviors: false

    /* Screenshot of the window, minus the decorations. The actual image is
       obtained via the WindowImageProvider which serves the "image://window/*" source URIs.
       Please note that the screenshot is taken at the moment the source property is
       actually assigned, during component initialization.
       If taking the screenshot fails (for example for minimized windows), then this
       is hidden and the icon box (see "icon_box" below) is shown. */
    Image {
        id: shot

        anchors.fill: parent
        fillMode: Image.Stretch

        /* HACK: QML uses an internal cache for Image objects that seems to use as
           key the source property of the image.
           This is great for normal images but in this case we really want the
           screenshot to reload everytime.
           Since I could not find any way to disable this cache, I am using this
           hack which essentially appends the current time to the source URL of the
           Image, tricking the cache into doing a request to the image provider.
        */
        source: "image://window/" + windowInfo.decoratedXid + "|"
                                  + windowInfo.contentXid + "@"
                                  + screen.currentTime()

        /* This will be disabled during intro/outro animations for performance reasons,
           but it's good to have in spread mode when the window is */
        smooth: true

        visible: (status != Image.Error)
    }

    /* This replaces the shot whenever retrieving its image fails.
       It is essentially a white rectangle of the same size as the shot,
       with a border and the window icon floating in the center.
    */
    Rectangle {
        id: iconBox

        anchors.fill: parent

        border.width: 1
        border.color: "black"

        visible: (shot.status == Image.Error)

        Image {
            source: "image://icons/" + windowInfo.icon
            asynchronous: true

            anchors.centerIn: parent
            fillMode: Image.PreserveAspectFit

            /* Please note that sourceSize is necessary, otherwise the
               IconImageProvider will crash when loading the icon */
            height: 48
            width: 48
            sourceSize { width: width; height: height }
        }
    }

    Item {
        id: overlay

        anchors.fill: parent

        /* Shown only in spread state, see transitions */
        visible: false

        /* A darkening rectangle that covers every window in spread state,
           except the currently selected window. See overlay.states */
        Rectangle {
            id: darken

            anchors.fill:  parent

            color: "black"
            opacity: 0.1

            visible: !window.isSelected
        }

        /* A label with the window title centered over the shot.
           It will appear only for the currently selected window. See overlay.states */
        Rectangle {
            id: labelBox

            /* The width of the box around the text should be the same as
               the text itself, with 3 pixels of margin on all sides, but it should also
               never overflow the shot's borders.

               Normally one would just set anchors.margins, but this can't work
               here because first we need to let the Text calculate it's "natural" width
               ("paintedWidth" in QT terms) -- that is, the size it would have
               ìf free to expand horizontally unconstrained, to know if it's smaller than
               the labelBox or not.
               However if we bind the Text's width to the width of the labelBox, and the
               width of the labelBox to the Text's size, we get a binding loop error.

               The trick is to bind the Text's width to the labelBox's parent, and then
               the labelBox to the Text's size. Since the relation between labelBox and
               parent is taken care of by the positioner indirectly, there's no loop.

               Yeah, messy. Blame QML ;)
            */
            property int labelMargins: 6
            width: Math.min(parent.width, label.paintedWidth + labelMargins)
            height: label.height + labelMargins
            anchors.centerIn: parent

            /* This equals backgroundColor: "black" and opacity: 0.6
               but we don't want to set it that way since it would be
               inherited by the Text child, and we want it to be fully
               opaque instead */
            color: "#99000000"
            radius: 3
            visible: window.isSelected

            Text {
                id: label

                anchors.centerIn: parent
                width: overlay.width - parent.labelMargins

                text: windowInfo.title
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter

                property real originalFontSize
                Component.onCompleted: {
                    originalFontSize = font.pointSize
                }

                color: "white"
            }
        }
    }

    MouseArea {
        id: mouseArea

        width: shot.paintedWidth
        height: shot.paintedHeight
        anchors.centerIn: parent
        hoverEnabled: true

        onClicked: window.clicked()
        onEntered: window.entered()
        onExited: window.exited()
    }

    /* The following behaviors smoothly treansition all changes in size and position
       of the window, especially when it "follows" the position of the grid cell it is
       related to. The are enabled after the item has completed adding. */
    Behavior on x {
        enabled: enableBehaviors
        NumberAnimation { easing.type: Easing.InOutSine; duration: transitionDuration }
    }
    Behavior on y {
        enabled: enableBehaviors
        NumberAnimation { easing.type: Easing.InOutSine; duration: transitionDuration }
    }
    Behavior on width {
        enabled: enableBehaviors
        NumberAnimation { easing.type: Easing.InOutSine; duration: transitionDuration }
    }
    Behavior on height {
        enabled: enableBehaviors
        NumberAnimation { easing.type: Easing.InOutSine; duration: transitionDuration }
    }

    /* It is very important that we enable the behaviors at the end of the enterAnimation
       so that items will be able to slide around in the grid when other items before them
       are added or removed. */
    property variant enterAnimation: SequentialAnimation {
        PropertyAction { target: window; properties: "opacity, scale"; value: 0.0 }
        NumberAnimation { target: window; properties: "opacity, scale";
                          to: 1.0; duration: transitionDuration; easing.type: Easing.InOutSine }
        PropertyAction { target: window; property: "enableBehaviors"; value: true }
    }

    /* When a delegate is about to be removed from a GridView the GridView.onRemove
       signal will be fired, and it's setup to run this animation.
       To make sure that the delegate is not destroyed until the animation is complete
       GridView makes available the property GridView.delayRemove that prevents the
       grid from actually destroying the delegate while it's set to true, and then
       destroys it when set to false. */
    property variant exitAnimation: SequentialAnimation {
        PropertyAction { target: cell; property: "GridView.delayRemove"; value: true }
        NumberAnimation { target: window; properties: "opacity,scale"; to: 0.0;
                          duration: transitionDuration; easing.type: Easing.InOutSine }
        PropertyAction { target: cell; property: "GridView.delayRemove"; value: false }
    }

    states: [
        /* This state is what we want to have at the end of the intro.
           In other words, it puts the window in its right place and size when in spread mode. */
        State {
            name: "spread"
            PropertyChanges {
                target: window

                width: spreadWidth
                height: spreadHeight

                /* Center window within the cell */
                x: cell.x + (cell.width - spreadWidth) / 2
                y: cell.y + (cell.height - spreadHeight) / 2
            }

            /* Keep the font the same size it would have if the Spread wasn't scaled down to
               fit into the workspace switcher.
               The check on originalFontSize not being zero is to prevent errors in assigning
               a zero point size when originalFontSize is not initialized yet. */
            PropertyChanges {
                target:label
                font.pointSize: (originalFontSize != 0) ? (originalFontSize / spread.workspaceScale) : 1
            }
        }
    ]

    transitions: [
        /* This is the animation that is exectuted when moving between any of the two states.
           It will be executed in the same sequence for both the intro and outro. */
        Transition {
            SequentialAnimation {
                PropertyAction { target: shot; property: "smooth"; value: false }
                PropertyAction { target: mouseArea; property: "enabled"; value: false }
                PropertyAction { target: overlay; property: "visible"; value: false }
                NumberAnimation {
                    target: window
                    properties: "x,y,width,height"
                    duration: window.transitionDuration
                    easing.type: Easing.InOutSine
                }
                PropertyAction { target: shot; property: "smooth"; value: true }
                PropertyAction { target: mouseArea; property: "enabled"; value: (window.state == "spread") }
                PropertyAction { target: overlay; property: "visible"; value: true }
            }
        }
    ]
}
