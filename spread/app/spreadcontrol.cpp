/*
 * Copyright (C) 2010 Canonical, Ltd.
 *
 * Authors:
 *  Ugo Riboni <ugo.riboni@canonical.com>
 *  Florian Boucault <florian.boucault@canonical.com>
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

#include <QDebug>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusServiceWatcher>
#include <QDBusConnection>

#include "spreadcontrol.h"
#include "spreadadaptor.h"

static const char* DBUS_SERVICE = "com.canonical.UnityQt.Spread";
static const char* DBUS_OBJECT_PATH = "/Spread";

SpreadControl::SpreadControl(QObject *parent) :
    QObject(parent), mServiceWatcher(new QDBusServiceWatcher(this))
{
    mServiceWatcher->setConnection(QDBusConnection::sessionBus());
    mServiceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(mServiceWatcher, SIGNAL(serviceUnregistered(const QString&)), SLOT(slotServiceUnregistered(const QString&)));
}

SpreadControl::~SpreadControl() {
    QDBusConnection::sessionBus().unregisterService(mService);
}

bool SpreadControl::connectToBus(const QString& _service, const QString& _path)
{
    mService = _service.isEmpty() ? DBUS_SERVICE : _service;
    QString path = _path.isEmpty() ? DBUS_OBJECT_PATH : _path;

    bool ok = QDBusConnection::sessionBus().registerService(mService);
    if (!ok) {
        return false;
    }
    new SpreadAdaptor(this);
    QDBusConnection::sessionBus().registerObject(path, this);

    return true;
}

void SpreadControl::SpreadAllWindows() {
    qDebug() << "DBUS: Received request to spread all windows";
    Q_EMIT activateSpread(0);
}

void SpreadControl::SpreadApplicationWindows(unsigned int applicationId) {
    qDebug() << "DBUS: Received request to spread application windows of" << applicationId;
    Q_EMIT activateSpread(applicationId);
}

void SpreadControl::CancelSpread() {
    qDebug() << "DBUS: Received request to cancel the spread";
    Q_EMIT cancelSpread();
}

void SpreadControl::slotServiceUnregistered(const QString& service)
{
    mServiceWatcher->removeWatchedService(service);
}
