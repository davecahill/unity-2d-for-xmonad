/*
 * Copyright (C) 2010 Canonical, Ltd.
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

// Local
#include <config.h>
#include "shelldeclarativeview.h"
#include "dashdbus.h"

// libunity-2d-private
#include <debug_p.h>
#include <hotkey.h>
#include <hotkeymonitor.h>
#include <keyboardmodifiersmonitor.h>
#include <keymonitor.h>
#include <dashsettings.h>
#include <launcherclient.h>
#include <screeninfo.h>

// Qt
#include <QApplication>
#include <QBitmap>
#include <QCloseEvent>
#include <QDeclarativeContext>
#include <QDesktopWidget>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>
#include <QX11Info>
#include <QGraphicsObject>

// X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>

static const int KEY_HOLD_THRESHOLD = 250;

static const char* SPREAD_DBUS_SERVICE = "com.canonical.Unity2d.Spread";
static const char* SPREAD_DBUS_PATH = "/Spread";
static const char* SPREAD_DBUS_INTERFACE = "com.canonical.Unity2d.Spread";
static const char* SPREAD_DBUS_METHOD_IS_SHOWN = "IsShown";

static const char* COMMANDS_LENS_ID = "commands.lens";

ShellDeclarativeView::ShellDeclarativeView()
    : Unity2DDeclarativeView()
    , m_mode(DesktopMode)
    , m_expanded(true)
    , m_active(false)
    , m_superKeyPressed(false)
    , m_superKeyHeld(false)
{
    setAttribute(Qt::WA_X11NetWmWindowTypeDock, true);
    /* Respect the struts manually because the window manager does not enforce struts
       on dock windows */
    move(QApplication::desktop()->availableGeometry(QX11Info::appScreen()).topLeft());

    setTransparentBackground(QX11Info::isCompositingManagerRunning());

    m_superKeyHoldTimer.setSingleShot(true);
    m_superKeyHoldTimer.setInterval(KEY_HOLD_THRESHOLD);
    connect(&m_superKeyHoldTimer, SIGNAL(timeout()), SLOT(updateSuperKeyHoldState()));
    connect(this, SIGNAL(superKeyTapped()), SLOT(toggleDash()));

    connect(&launcher2dConfiguration(), SIGNAL(superKeyEnableChanged(bool)), SLOT(updateSuperKeyMonitoring()));
    updateSuperKeyMonitoring();

    /* Alt+F1 reveal the launcher and gives the keyboard focus to the Dash Button. */
    Hotkey* altF1 = HotkeyMonitor::instance().getHotkeyFor(Qt::Key_F1, Qt::AltModifier);
    connect(altF1, SIGNAL(pressed()), SLOT(onAltF1Pressed()));

    /* Alt+F2 shows the dash with the commands lens activated. */
    Hotkey* altF2 = HotkeyMonitor::instance().getHotkeyFor(Qt::Key_F2, Qt::AltModifier);
    connect(altF2, SIGNAL(pressed()), SLOT(showCommandsLens()));

    /* Super+{n} for 0 ≤ n ≤ 9 activates the item with index (n + 9) % 10. */
    for (Qt::Key key = Qt::Key_0; key <= Qt::Key_9; key = (Qt::Key) (key + 1)) {
        Hotkey* hotkey = HotkeyMonitor::instance().getHotkeyFor(key, Qt::MetaModifier);
        connect(hotkey, SIGNAL(pressed()), SLOT(forwardNumericHotkey()));
        hotkey = HotkeyMonitor::instance().getHotkeyFor(key, Qt::MetaModifier | Qt::ShiftModifier);
        connect(hotkey, SIGNAL(pressed()), SLOT(forwardNumericHotkey()));
    }

    //connect(desktop, SIGNAL(resized(int)), SLOT(updateDashModeDependingOnScreenGeometry()));
}

// TODO: this is probably expressed more nicely in QML.
void
ShellDeclarativeView::updateDashModeDependingOnScreenGeometry()
{
    QRect rect = ScreenInfo::instance()->geometry();
    QSize minSize = Unity2d::DashSettings::minimumSizeForDesktop();
    if (rect.width() < minSize.width() && rect.height() < minSize.height()) {
        setDashMode(FullScreenMode);
    } else {
        setDashMode(DesktopMode);
    }
}

void
ShellDeclarativeView::focusOutEvent(QFocusEvent* event)
{
    Unity2DDeclarativeView::focusOutEvent(event);
    setDashActive(false);
    Q_EMIT focusChanged();
}

void
ShellDeclarativeView::focusInEvent(QFocusEvent* event)
{
    Unity2DDeclarativeView::focusInEvent(event);
    Q_EMIT focusChanged();
}

void
ShellDeclarativeView::setWMFlags()
{
    Display *display = QX11Info::display();
    Atom stateAtom = XInternAtom(display, "_NET_WM_STATE", False);
    Atom propAtom;

    propAtom = XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
    XChangeProperty(display, effectiveWinId(), stateAtom,
                    XA_ATOM, 32, PropModeAppend, (unsigned char *) &propAtom, 1);

    propAtom = XInternAtom(display, "_NET_WM_STATE_SKIP_PAGER", False);
    XChangeProperty(display, effectiveWinId(), stateAtom,
                    XA_ATOM, 32, PropModeAppend, (unsigned char *) &propAtom, 1);
}

bool
ShellDeclarativeView::isSpreadActive()
{
    /* Check if the spread is present on DBUS first, as we don't want to have DBUS
       activate it if it's not running yet */
    QDBusConnectionInterface* sessionBusIFace = QDBusConnection::sessionBus().interface();
    QDBusReply<bool> reply = sessionBusIFace->isServiceRegistered(SPREAD_DBUS_SERVICE);
    if (reply.isValid() && reply.value() == true) {
        QDBusInterface spreadInterface(SPREAD_DBUS_SERVICE, SPREAD_DBUS_PATH,
                                       SPREAD_DBUS_INTERFACE);

        QDBusReply<bool> spreadActiveResult = spreadInterface.call(SPREAD_DBUS_METHOD_IS_SHOWN);
        if (spreadActiveResult.isValid() && spreadActiveResult.value() == true) {
            return true;
        }
    }
    return false;
}

void
ShellDeclarativeView::showEvent(QShowEvent *event)
{
    QDeclarativeView::showEvent(event);
    /* Note that this has to be called everytime the window is shown, as the WM
       will remove the flags when the window is hidden */
    setWMFlags();
}

void
ShellDeclarativeView::setDashActive(bool value)
{
    if (value != m_active) {
        if (value) {
            /* Check if the spread is active before activating the dash.
               We need to do this since the spread can't prevent the launcher from
               monitoring the super key and therefore getting to this point if
               it's tapped. */
            if (isSpreadActive()) {
                return;
            }

            /* FIXME: should not be called here; it is unrelated to the dash being active or not
               It should be called at the shell instantiation and whenever
               ScreenInfo::instance()->geometry() changes */
            updateDashModeDependingOnScreenGeometry();
            // FIXME: should be moved to Shell.qml
            // We need a delay, otherwise the window may not be visible when we try to activate it
            QTimer::singleShot(0, this, SLOT(forceActivateWindow()));
        }
        m_active = value;
        Q_EMIT dashActiveChanged(m_active);
    }
}

bool
ShellDeclarativeView::dashActive() const
{
    return m_active;
}

void
ShellDeclarativeView::setDashMode(ShellDeclarativeView::DashMode mode)
{
    if (m_mode == mode) {
        return;
    }

    m_mode = mode;
    dashModeChanged(m_mode);
}

ShellDeclarativeView::DashMode
ShellDeclarativeView::dashMode() const
{
    return m_mode;
}

void
ShellDeclarativeView::setExpanded(bool value)
{
    if (m_expanded == value) {
        return;
    }

    m_expanded = value;
    expandedChanged(m_expanded);
}

bool
ShellDeclarativeView::expanded() const
{
    return m_expanded;
}

void
ShellDeclarativeView::setActiveLens(const QString& activeLens)
{
    if (activeLens != m_activeLens) {
        m_activeLens = activeLens;
        Q_EMIT activeLensChanged(activeLens);
    }
}

const QString&
ShellDeclarativeView::activeLens() const
{
    return m_activeLens;
}

void
ShellDeclarativeView::resizeEvent(QResizeEvent* event)
{
    if (!QX11Info::isCompositingManagerRunning()) {
        updateMask();
    }
    QDeclarativeView::resizeEvent(event);
}

static QBitmap
createCornerMask()
{
    QPixmap pix(unity2dDirectory() + "/shell/dash/artwork/desktop_dash_background_no_transparency.png");
    return pix.createMaskFromColor(Qt::red, Qt::MaskOutColor);
}

void
ShellDeclarativeView::updateMask()
{
    if (m_mode == FullScreenMode) {
        clearMask();
        return;
    }
    QBitmap bmp(size());
    {
        static QBitmap corner = createCornerMask();
        static QBitmap top = corner.copy(0, 0, corner.width(), 1);
        static QBitmap left = corner.copy(0, 0, 1, corner.height());

        const int cornerX = bmp.width() - corner.width();
        const int cornerY = bmp.height() - corner.height();

        QPainter painter(&bmp);
        painter.fillRect(bmp.rect(), Qt::color1);
        painter.setBackgroundMode(Qt::OpaqueMode);
        painter.setBackground(Qt::color1);
        painter.setPen(Qt::color0);
        painter.drawPixmap(cornerX, cornerY, corner);

        painter.drawTiledPixmap(cornerX, 0, top.width(), cornerY, top);
        painter.drawTiledPixmap(0, cornerY, cornerX, left.height(), left);
    }
    setMask(bmp);
}

void
ShellDeclarativeView::toggleDash()
{
    if (dashActive()) {
        setDashActive(false);
    } else {
        Q_EMIT activateHome();
    }
}

void
ShellDeclarativeView::showCommandsLens()
{
    Q_EMIT activateLens(COMMANDS_LENS_ID);
}

void
ShellDeclarativeView::onAltF1Pressed()
{
    if (!isActiveWindow()) {
        Q_EMIT launcherFocusRequested();
        forceActivateWindow();
    } else {
        forceDeactivateWindow();
    }
}

/* ----------------- super key handling ---------------- */

void
ShellDeclarativeView::updateSuperKeyHoldState()
{
    /* If the key was released in the meantime, just do nothing, otherwise
       consider the key being held, unless we're told to ignore it. */
    if (m_superKeyPressed && !m_superPressIgnored) {
        m_superKeyHeld = true;
        Q_EMIT superKeyHeldChanged(m_superKeyHeld);
    }
}

void
ShellDeclarativeView::updateSuperKeyMonitoring()
{
    KeyboardModifiersMonitor *modifiersMonitor = KeyboardModifiersMonitor::instance();
    KeyMonitor *keyMonitor = KeyMonitor::instance();
    HotkeyMonitor& hotkeyMonitor = HotkeyMonitor::instance();

    QVariant value = launcher2dConfiguration().property("superKeyEnable");
    if (!value.isValid() || value.toBool() == true) {
        hotkeyMonitor.enableModifiers(Qt::MetaModifier);
        QObject::connect(modifiersMonitor,
                         SIGNAL(keyboardModifiersChanged(Qt::KeyboardModifiers)),
                         this, SLOT(setHotkeysForModifiers(Qt::KeyboardModifiers)));
        /* Ignore Super presses if another key was pressed simultaneously
           (i.e. a shortcut). https://bugs.launchpad.net/unity-2d/+bug/801073 */
        QObject::connect(keyMonitor,
                         SIGNAL(keyPressed()),
                         this, SLOT(ignoreSuperPress()));
        setHotkeysForModifiers(modifiersMonitor->keyboardModifiers());
    } else {
        hotkeyMonitor.disableModifiers(Qt::MetaModifier);
        QObject::disconnect(modifiersMonitor,
                            SIGNAL(keyboardModifiersChanged(Qt::KeyboardModifiers)),
                            this, SLOT(setHotkeysForModifiers(Qt::KeyboardModifiers)));
        QObject::disconnect(keyMonitor,
                            SIGNAL(keyPressed()),
                            this, SLOT(ignoreSuperPress()));
        m_superKeyHoldTimer.stop();
        m_superKeyPressed = false;
        if (m_superKeyHeld) {
            m_superKeyHeld = false;
            Q_EMIT superKeyHeldChanged(false);
        }
    }
}

void
ShellDeclarativeView::setHotkeysForModifiers(Qt::KeyboardModifiers modifiers)
{
    /* This is the new new state of the Super key (AKA Meta key), while
       m_superKeyPressed is the previous state of the key at the last modifiers change. */
    bool superKeyPressed = modifiers.testFlag(Qt::MetaModifier);

    if (m_superKeyPressed != superKeyPressed) {
        m_superKeyPressed = superKeyPressed;
        if (superKeyPressed) {
            m_superPressIgnored = false;
            /* If the key is pressed, start up a timer to monitor if it's being held short
               enough to qualify as just a "tap" or as a proper hold */
            m_superKeyHoldTimer.start();
        } else {
            m_superKeyHoldTimer.stop();

            /* If the key is released, and was not being held, it means that the user just
               performed a "tap". Unless we're told to ignore that tap, that is. */
            if (!m_superKeyHeld && !m_superPressIgnored) {
                Q_EMIT superKeyTapped();
            }
            /* Otherwise the user just terminated a hold. */
            else if(m_superKeyHeld){
                m_superKeyHeld = false;
                Q_EMIT superKeyHeldChanged(m_superKeyHeld);
            }
        }
    }
}

void
ShellDeclarativeView::ignoreSuperPress()
{
    /* There was a key pressed, ignore current super tap/hold */
    m_superPressIgnored = true;
}

void
ShellDeclarativeView::forwardNumericHotkey()
{
    Hotkey* hotkey = qobject_cast<Hotkey*>(sender());
    if (hotkey != NULL) {
        /* Shortcuts from 1 to 9 should activate the items with index
           from 1 to 9 (index 0 being the so-called "BFB" or Dash launcher).
           Shortcut for 0 should activate item with index 10.
           In other words, the indexes are activated in the same order as
           the keys appear on a standard keyboard. */
        Qt::Key key = hotkey->key();
        if (key >= Qt::Key_1 && key <= Qt::Key_9) {
            int index = key - Qt::Key_0;
            if (hotkey->modifiers() & Qt::ShiftModifier) {
                Q_EMIT newInstanceShortcutPressed(index);
            } else {
                Q_EMIT activateShortcutPressed(index);
            }
        } else if (key == Qt::Key_0) {
            if (hotkey->modifiers() & Qt::ShiftModifier) {
                Q_EMIT newInstanceShortcutPressed(10);
            } else {
                Q_EMIT activateShortcutPressed(10);
            }
        }
    }
}

/* ----------------- monitored area handling ---------------- */

void
ShellDeclarativeView::mouseMoveEvent(QMouseEvent *event)
{
    Unity2DDeclarativeView::mouseMoveEvent(event);

    bool containsMouse = m_monitoredArea.contains(event->pos());
    if (m_monitoredAreaContainsMouse != containsMouse) {
        m_monitoredAreaContainsMouse = containsMouse;
        Q_EMIT monitoredAreaContainsMouseChanged();
    }
}

void
ShellDeclarativeView::leaveEvent(QEvent *event)
{
    Unity2DDeclarativeView::leaveEvent(event);

    if (m_monitoredAreaContainsMouse) {
        m_monitoredAreaContainsMouse = false;
        Q_EMIT monitoredAreaContainsMouseChanged();
    }
}

/* When another window calls XGrabPointer we receive a LeaveNotify event
   but QT doesn't emit a corresponding leaveEvent. Therefore we have to intercept it
   ourselves from X11 and act accordingly.
   The same is true for the opposite, when XUngrabPointer is called. */
bool
ShellDeclarativeView::x11EventFilter(XEvent* event)
{
    if (event->type == LeaveNotify && event->xcrossing.mode == NotifyGrab) {
        if (m_monitoredAreaContainsMouse) {
            m_monitoredAreaContainsMouse = false;
            Q_EMIT monitoredAreaContainsMouseChanged();
        }
    } else if (event->type == EnterNotify && event->xcrossing.mode == NotifyUngrab) {
        if (!m_monitoredAreaContainsMouse) {
            m_monitoredAreaContainsMouse = true;
            Q_EMIT monitoredAreaContainsMouseChanged();
        }
    }
    return false;
}

QRect
ShellDeclarativeView::monitoredArea() const
{
    return m_monitoredArea;
}

void
ShellDeclarativeView::setMonitoredArea(QRect monitoredArea)
{
    if (m_monitoredArea != monitoredArea) {
        m_monitoredArea = monitoredArea;
        Q_EMIT monitoredAreaChanged();

        bool containsMouse = monitoredArea.contains(mapFromGlobal(QCursor::pos()));
        if (containsMouse != m_monitoredAreaContainsMouse) {
            m_monitoredAreaContainsMouse = containsMouse;
            Q_EMIT monitoredAreaContainsMouseChanged();
        }
    }
}

bool
ShellDeclarativeView::monitoredAreaContainsMouse() const
{
    return m_monitoredAreaContainsMouse;
}
