import Qt 4.7

/* When added as a child of a ListView and the listview itself is set to the
   'list' property it will make it possible to use drag’n’drop to re-order
   the items in the list. */

// FIXME: flicking the list fast exhibits unpleasant visual artifacts:
// the y coordinate of the looseItems is correct, however they are not
// re-drawn at the correct position until the mouse cursor is moved
// further. This may be a bug in QML.
// Ref: https://bugs.launchpad.net/unity-2d/+bug/727082.
MouseArea {
    id: dnd
    anchors.fill: parent

    property variant list

    /* list index of the tile being dragged */
    property int draggedTileIndex
    /* id (desktop file path) of the tile being dragged */
    property string draggedTileId: ""
    /* absolute mouse coordinates in the list */
    property variant listCoordinates: mapToItem(list.contentItem, mouseX, mouseY)
    /* list index of the tile underneath the cursor */
    property int tileAtCursorIndex: list.indexAt(listCoordinates.x, listCoordinates.y)

    Timer {
        id: longPressDelay
        /* The standard threshold for long presses is hard-coded to 800ms
           (http://doc.qt.nokia.com/qml-mousearea.html#onPressAndHold-signal).
           This value is too high for our use case. */
        interval: 500 /* in milliseconds */
        onTriggered: {
            if (list.moving) return
            dnd.parent.interactive = false
            var id = items.get(dnd.draggedTileIndex).desktop_file
            if (id != undefined) dnd.draggedTileId = id
        }
    }
    onPressed: {
        /* tileAtCursorIndex is not valid yet because the mouse area is
           not sensitive to hovering (if it were, it would eat hover
           events for other mouse areas below, which is not desired). */
        var coord = mapToItem(list.contentItem, mouse.x, mouse.y)
        draggedTileIndex = list.indexAt(coord.x, coord.y)
        longPressDelay.start()
    }
    function drop() {
        longPressDelay.stop()
        draggedTileId = ""
        parent.interactive = true
    }
    onReleased: {
        if (draggedTileId != "") {
            drop()
        } else if (draggedTileIndex == tileAtCursorIndex) {
            /* Forward the click to the launcher item below. */
            var point = mapToItem(list.contentItem, mouse.x, mouse.y)
            var item = list.contentItem.childAt(point.x, point.y)
            /* FIXME: the coordinates of the mouse event forwarded are
               incorrect. Luckily, it’s acceptable as they are not used in
               the handler anyway. */
            if (item && typeof(item.clicked) == "function") item.clicked(mouse)
        }
    }
    onExited: drop()
    onPositionChanged: {
        if (draggedTileId != "" && tileAtCursorIndex != -1 && tileAtCursorIndex != draggedTileIndex) {
            /* Workaround a bug in QML whereby moving an item down in
               the list results in its visual representation being
               shifted too far down by one index
               (http://bugreports.qt.nokia.com/browse/QTBUG-15841).
               Since the bug happens only when moving an item *down*,
               and since moving an item one index down is strictly
               equivalent to moving the item below one index up, we
               achieve the same result by tricking the list model into
               thinking that the mirror operation was performed.
               Note: this bug will be fixed in Qt 4.7.2, at which point
               this workaround can go away. */
            if (tileAtCursorIndex > draggedTileIndex) {
                items.move(tileAtCursorIndex, draggedTileIndex, 1)
            } else {
                /* This should be the only code path here, if it wasn’t
                   for the bug explained and worked around above. */
                items.move(draggedTileIndex, tileAtCursorIndex, 1)
            }
            draggedTileIndex = tileAtCursorIndex
        }
    }
}