/*
 * This file is part of unity-2d
 *
 * Copyright 2010-2011 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by:
 * - Jure Ham <jure@hamsworld.net>
 */

import QtQuick 1.0
import "utils.js" as Utils
import Unity2d 1.0

/* The main component that manages the windows.
   This only acts as an outer shell, the actual logic is pretty much all in SpreadLayout.qml
   and SpreadItem.qml

   In the rest of the comments there will be some recurring terms that I explain below:
   - screen mode: in this mode each shot is positioned and scaled exactly as the real window.
   - spread mode: in this mode each shot is constrained to a cell in a layout.
   - intro animation: the animation that moves the shots from screen to spread mode
   - outro animation: the animation that moves the shots from spread mode back to screen mode

   The context property called control is the initiator of the entire spread process, and
   is triggered by D-Bus calls on the C++ side.

   The ScreenInfo's property availableGeometry represents the available space on the screen (i.e.
   screen minus launcher, panels, etc.).
*/

GridView {
    id: windows

    signal clicked
    signal windowActivated(variant window)

    MouseArea {
        anchors.fill: parent
        onClicked: windows.clicked()
        /* Eating all mouse events so that they are not passed beneath the workspace */
        hoverEnabled: true
    }

    /* This proxy model takes care of removing all windows that are not on
       the current workspace and that are not pinned to all workspaces. */
    SortFilterProxyModel {
        id: filteredByWorkspace
        model: switcher.allWindows
        dynamicSortFilter: true

        filterRole: WindowInfo.RoleWorkspace
        //filterRegExp: RegExp("^%1|-2$".arg(workspace.workspaceNumber))
    }

    /* If there's any application filter set, this proxy model will remove
       all windows that do not belong to it. */
    SortFilterProxyModel {
        id: filteredByApplication
        model: filteredByWorkspace
        dynamicSortFilter: true

        filterRole: WindowInfo.RoleDesktopFile
        filterRegExp: RegExp("%1".arg(switcher.applicationFilter))
    }

    property int columns: Math.ceil(Math.sqrt(count))
    property int rows: Math.ceil(count / columns)
    property int cellSpacingVertical: 10
    property int cellSpacingHorizontal: 20

    cellWidth: Math.floor(width / columns)
    cellHeight: height / rows - 20

    model: filteredByApplication

    delegate:
        Item {
            id: cell

            ScaledItem {
                id: scaledCell

                target: cell
                itemWidth: window.size.width
                itemHeight: window.size.height
                parent: windows
            }

            /* Workaround http://bugreports.qt.nokia.com/browse/QTBUG-15642 where onAdd is not called for the first item */
            //GridView.onAdd:
            Component.onCompleted: if (!switcher.initial) addAnimation.start()
            GridView.onRemove: if (!switcher.initial) removeAnimation.start()

            width: windows.cellWidth
            height: windows.cellHeight

            Keys.onPressed: {
                switch (event.key) {
                    case Qt.Key_Enter:
                    case Qt.Key_Return:
                        windows.windowActivated(spreadWindow)
                        event.accepted = true
                }
            }

            Window {
                id: spreadWindow

                property bool animateFollow: false
                property bool followCell: true

                isSelected: cell.activeFocus

                onEntered: {
                    windows.currentIndex = index
                    cell.forceActiveFocus()
                }
                Component.onCompleted: {
                    if (index == 0) {
                        cell.forceActiveFocus()
                    }
                }

                onClicked: windows.windowActivated(spreadWindow)

                /* Reparenting hack inspired by http://developer.qt.nokia.com/wiki/Drag_and_Drop_within_a_GridView */
                parent: windows

                z: window.z

                windowInfo: window
                state: windows.state
                states: [
                    State {
                        name: "screen"
                        PropertyChanges {
                            target: spreadWindow
                            /* Note that we subtract the availableGeometry x and y since window.location is
                            expressed in global screen coordinates. */
                            x: window.position.x - declarativeView.globalPosition.x
                            y: window.position.y - declarativeView.globalPosition.y
                            width: window.size.width
                            height: window.size.height
                            animateFollow: false
                        }
                    },
                    State {
                        name: "spread"
                        PropertyChanges {
                            target: spreadWindow
                            /* Center the window in its cell */
                            x: followCell ? scaledCell.x + cellSpacingVertical : x
                            y: followCell ? scaledCell.y + cellSpacingHorizontal : y
                            width: followCell ? scaledCell.width - cellSpacingVertical * 2 : width
                            height: followCell ? scaledCell.height - cellSpacingHorizontal * 2: height
                            animateFollow: !switcher.initial
                        }
                    }
                ]

                SequentialAnimation {
                    id: addAnimation
                }
                SequentialAnimation {
                    id: removeAnimation
                }
                transitions: [
                    Transition {
                        to: "screen,spread"
                        SequentialAnimation {
                        }
                    }
                ]
            }
    }
}
