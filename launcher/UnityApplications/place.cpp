/*
 * Copyright (C) 2010 Canonical, Ltd.
 *
 * Authors:
 *  Olivier Tilloy <olivier.tilloy@canonical.com>
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

#include "place.h"

#include <QStringList>
#include <QDebug>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>

static const char* UNITY_PLACE_INTERFACE = "com.canonical.Unity.Place";

Place::Place(QObject* parent) :
    QAbstractListModel(parent),
    m_file(NULL),
    m_online(false),
    m_dbusIface(NULL),
    m_querying(false)
{
}

Place::Place(const Place &other)
{
    if (other.m_file != NULL)
        setFileName(other.m_file->fileName());
}

Place::~Place()
{
    delete m_dbusIface;
    delete m_file;
    m_entries.clear();
    m_static_entries.clear();
}

QString
Place::fileName() const
{
    return m_file->fileName();
}

void
Place::setFileName(const QString &file)
{
    m_file = new QSettings(file, QSettings::IniFormat);
    if (m_file->childGroups().contains("Place"))
    {
        m_dbusName = m_file->value("Place/DBusName").toString();
        m_dbusObjectPath = m_file->value("Place/dbusObjectPath").toString();
        QStringList entries = m_file->childGroups().filter("Entry:");
        QStringList::const_iterator iter;
        uint i = 0;
        for(iter = entries.begin(); iter != entries.end(); ++iter)
        {
            PlaceEntry* entry = new PlaceEntry(this);
            entry->setFileName(file);
            entry->setGroupName(iter->mid(6));
            m_file->beginGroup(*iter);
            entry->setDbusName(m_dbusName);
            entry->setDbusObjectPath(m_file->value("DBusObjectPath").toString());
            // FIXME: extract localized name
            entry->setName(m_file->value("Name").toString());
            entry->setIcon(m_file->value("Icon").toString());
            m_file->endGroup();
            entry->setPosition(i++);
            connect(entry, SIGNAL(positionChanged(uint)),
                    SLOT(onEntryPositionChanged(uint)));
            m_static_entries[entry->dbusObjectPath()] = entry;
        }
        connectToRemotePlace();
    }
    else
    {
        delete m_file;
        m_file = NULL;
        qWarning() << "Invalid place file, missing [Place] group";
    }
}

QString
Place::dbusName() const
{
    return m_dbusName;
}

QString
Place::dbusObjectPath() const
{
    return m_dbusObjectPath;
}

bool
Place::online() const
{
    return m_online;
}

QVariant
Place::data(const QModelIndex& index, int role) const
{
    Q_UNUSED(role);

    if (!index.isValid())
        return QVariant();

    return QVariant::fromValue(m_entries.at(index.row()));
}

int
Place::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)

    return m_entries.size();
}

void
Place::connectToRemotePlace()
{
    if (m_online) {
        return;
    }

    QDBusServiceWatcher* serviceWatcher = new QDBusServiceWatcher(this);
    serviceWatcher->setConnection(QDBusConnection::sessionBus());
    serviceWatcher->addWatchedService(m_dbusName);
    connect(serviceWatcher, SIGNAL(serviceRegistered(QString)),
            SLOT(slotRemotePlaceConnected()));
    connect(serviceWatcher, SIGNAL(serviceUnregistered(QString)),
            SLOT(slotRemotePlaceDisconnected()));

    m_dbusIface = new QDBusInterface(m_dbusName, m_dbusObjectPath,
                                     UNITY_PLACE_INTERFACE);
    QDBusConnection connection = m_dbusIface->connection();
    if (!connection.isConnected()) {
        qWarning() << "ERROR: unable to connect to bus:"
                   << connection.lastError();
        return;
    }

    if (m_dbusIface->isValid()) {
        slotRemotePlaceConnected();
    }
    else {
        /* A call to the interface will spawn the corresponding place daemon. */
        getEntries();
    }
}

void
Place::getEntries()
{
    if (m_querying) {
        /* A call to GetEntries is already pending. */
        return;
    }
    m_querying = true;

    /* Update the list of online entries. */
    QDBusPendingCall pcall = m_dbusIface->asyncCall("GetEntries");
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(pcall, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotEntries(QDBusPendingCallWatcher*)));
}

void
Place::startMonitoringEntries()
{
    QDBusConnection connection = m_dbusIface->connection();
    connection.connect(m_dbusName, m_dbusObjectPath, UNITY_PLACE_INTERFACE,
                       "EntryAdded", this, SLOT(onEntryAdded(const PlaceEntryInfoStruct&)));
    connection.connect(m_dbusName, m_dbusObjectPath, UNITY_PLACE_INTERFACE,
                       "EntryRemoved", this, SLOT(onEntryRemoved(const QString&)));
}

void
Place::stopMonitoringEntries()
{
    QDBusConnection connection = m_dbusIface->connection();
    connection.disconnect(m_dbusName, m_dbusObjectPath, UNITY_PLACE_INTERFACE,
                          "EntryAdded", this, SLOT(onEntryAdded(const PlaceEntryInfoStruct&)));
    connection.disconnect(m_dbusName, m_dbusObjectPath, UNITY_PLACE_INTERFACE,
                          "EntryRemoved", this, SLOT(onEntryRemoved(const QString&)));
}

void
Place::slotRemotePlaceConnected()
{
    m_online = true;
    Q_EMIT onlineChanged(m_online);

    startMonitoringEntries();
    getEntries();
}

void
Place::slotRemotePlaceDisconnected()
{
    m_online = false;
    Q_EMIT onlineChanged(m_online);

    stopMonitoringEntries();

    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    while (!m_entries.isEmpty()) {
        PlaceEntry* entry = m_entries.takeFirst();
        if (!m_static_entries.contains(entry->dbusObjectPath())) {
            delete entry;
        }
    }
    endRemoveRows();
}

void
Place::onEntryAdded(const PlaceEntryInfoStruct& p)
{
    PlaceEntry* entry = NULL;
    if (m_static_entries.contains(p.dbus_path)) {
        entry = m_static_entries.value(p.dbus_path);
    }
    else {
        entry = new PlaceEntry(this);
        entry->setDbusName(m_dbusName);
        entry->setDbusObjectPath(p.dbus_path);
    }
    entry->updateInfo(p);
    connect(entry, SIGNAL(positionChanged(uint)),
            SLOT(onEntryPositionChanged(uint)), Qt::UniqueConnection);
    int index = m_entries.size();
    beginInsertRows(QModelIndex(), index, index);
    m_entries.append(entry);
    endInsertRows();
    entry->connectToRemotePlaceEntry();
    emit entryAdded(entry);
}

void
Place::onEntryRemoved(const QString& dbusObjectPath)
{
    PlaceEntry* entry = NULL;
    QList<PlaceEntry*>::const_iterator i;
    for (i = m_entries.constBegin(); i != m_entries.constEnd(); ++i) {
        if ((*i)->dbusObjectPath() == dbusObjectPath) {
            entry = *i;
            break;
        }
    }
    if (entry != NULL) {
        emit entryRemoved(entry);
        int index = m_entries.indexOf(entry);
        beginRemoveRows(QModelIndex(), index, index);
        m_entries.removeOne(entry);
        endRemoveRows();
        if (!m_static_entries.contains(entry->dbusObjectPath())) {
            delete entry;
        }
    }
}

void
Place::onEntryPositionChanged(uint position)
{
    /* This doesn’t seem to be implemented/used in Unity, but it can’t hurt… */
    // TODO: may require some sanity checks.
    PlaceEntry* entry = static_cast<PlaceEntry*>(sender());
    int from = m_entries.indexOf(entry);
    if (from != -1) {
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), position);
        m_entries.move(from, position);
        endMoveRows();
    }
}

void
Place::gotEntries(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QList<PlaceEntryInfoStruct> > reply = *watcher;
    if (reply.isError()) {
        qWarning() << "ERROR:" << m_dbusName << reply.error().message();
    } else {
        QList<PlaceEntryInfoStruct> entries = reply.argumentAt<0>();
        QList<PlaceEntryInfoStruct>::const_iterator i;
        for (i = entries.constBegin(); i != entries.constEnd(); ++i) {
            if (m_static_entries.contains(i->dbus_path)) {
                PlaceEntry* entry = m_static_entries.value(i->dbus_path);
                entry->updateInfo(*i);
                int index = m_entries.size();
                beginInsertRows(QModelIndex(), index, index);
                m_entries.append(entry);
                endInsertRows();
                entry->connectToRemotePlaceEntry();
            }
            else {
                onEntryAdded(*i);
            }
        }
    }
    watcher->deleteLater();
    m_querying = false;
}

