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

#include "placeentry.h"

#include <QDBusMetaType>
#include <QDebug>

// Marshall the RendererInfoStruct data into a D-Bus argument
QDBusArgument &operator<<(QDBusArgument &argument, const RendererInfoStruct &r)
{
    argument.beginStructure();
    argument << r.default_renderer;
    argument << r.groups_model;
    argument << r.results_model;
    argument.beginMap(QVariant::String, QVariant::String);
    QMap<QString, QString>::const_iterator i;
    for (i = r.renderer_hints.constBegin(); i != r.renderer_hints.constEnd(); ++i) {
        argument.beginMapEntry();
        argument << i.key() << i.value();
        argument.endMapEntry();
    }
    argument.endMap();
    argument.endStructure();
    return argument;
}

// Retrieve the RendererInfoStruct data from the D-Bus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, RendererInfoStruct &r)
{
    argument.beginStructure();
    argument >> r.default_renderer;
    argument >> r.groups_model;
    argument >> r.results_model;
    r.renderer_hints.clear();
    argument.beginMap();
    while (!argument.atEnd()) {
        QString key;
        QString value;
        argument.beginMapEntry();
        argument >> key >> value;
        argument.endMapEntry();
        r.renderer_hints[key] = value;
    }
    argument.endMap();
    argument.endStructure();
    return argument;
}


// Marshall the PlaceEntryInfoStruct data into a D-Bus argument
QDBusArgument &operator<<(QDBusArgument &argument, const PlaceEntryInfoStruct &p)
{
    argument.beginStructure();
    argument << p.dbus_path;
    argument << p.name;
    argument << p.icon;
    argument << p.position;
    argument.beginArray(QVariant::String);
    QStringList::const_iterator i;
    for (i = p.mimetypes.constBegin(); i != p.mimetypes.constEnd(); ++i) {
        argument << *i;
    }
    argument.endArray();
    argument << p.sensitive;
    argument << p.sections_model;
    argument.beginMap(QVariant::String, QVariant::String);
    QMap<QString, QString>::const_iterator j;
    for (j = p.hints.constBegin(); j != p.hints.constEnd(); ++j) {
        argument.beginMapEntry();
        argument << j.key() << j.value();
        argument.endMapEntry();
    }
    argument.endMap();
    argument << p.entry_renderer_info;
    argument << p.global_renderer_info;
    argument.endStructure();
    return argument;
}

// Retrieve the PlaceEntryInfoStruct data from the D-Bus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, PlaceEntryInfoStruct &p)
{
    argument.beginStructure();
    argument >> p.dbus_path;
    argument >> p.name;
    argument >> p.icon;
    argument >> p.position;
    argument.beginArray();
    p.mimetypes.clear();
    while (!argument.atEnd()) {
        QString mimetype;
        argument >> mimetype;
        p.mimetypes.append(mimetype);
    }
    argument.endArray();
    argument >> p.sensitive;
    argument >> p.sections_model;
    p.hints.clear();
    argument.beginMap();
    while (!argument.atEnd()) {
        QString key;
        QString value;
        argument.beginMapEntry();
        argument >> key >> value;
        argument.endMapEntry();
        p.hints[key] = value;
    }
    argument.endMap();
    /* The PlaceEntryInfoChanged signal on the com.canonical.Unity.PlaceEntry
       interface omits the two RenderingInfo structs. */
    if (!argument.atEnd()) {
        argument >> p.entry_renderer_info;
        argument >> p.global_renderer_info;
    }
    argument.endStructure();
    return argument;
}


static const char* UNITY_PLACE_ENTRY_INTERFACE = "com.canonical.Unity.PlaceEntry";
static const char* SECTION_PROPERTY = "section";

PlaceEntry::PlaceEntry() :
    m_position(0),
    m_sensitive(true),
    m_sections(NULL),
    m_entryGroupsModel(NULL),
    m_entryResultsModel(NULL),
    m_globalGroupsModel(NULL),
    m_globalResultsModel(NULL),
    m_online(false),
    m_dbusIface(NULL)
{
    // FIXME: this is not the right place to do this…
    qDBusRegisterMetaType<RendererInfoStruct>();
    qDBusRegisterMetaType<PlaceEntryInfoStruct>();
    qDBusRegisterMetaType<QList<PlaceEntryInfoStruct> >();
}

PlaceEntry::PlaceEntry(const PlaceEntry& other) :
    m_fileName(other.m_fileName),
    m_groupName(other.m_groupName),
    m_dbusName(other.m_dbusName),
    m_dbusObjectPath(other.m_dbusObjectPath),
    m_icon(other.m_icon),
    m_name(other.m_name),
    m_position(other.m_position),
    m_mimetypes(other.m_mimetypes),
    m_sensitive(other.m_sensitive),

    m_entryRendererName(other.m_entryRendererName),
    m_entryGroupsModelName(other.m_entryGroupsModelName),
    m_entryResultsModelName(other.m_entryResultsModelName),

    m_globalRendererName(other.m_globalRendererName),
    m_globalGroupsModelName(other.m_globalGroupsModelName),
    m_globalResultsModelName(other.m_globalResultsModelName)
{
    setSections(other.m_sections);
    setHints(other.m_hints);

    setEntryGroupsModel(other.m_entryGroupsModel);
    setEntryResultsModel(other.m_entryResultsModel);
    setEntryRendererHints(other.m_entryRendererHints);

    setGlobalGroupsModel(other.m_globalGroupsModel);
    setGlobalResultsModel(other.m_globalResultsModel);
    setGlobalRendererHints(other.m_globalRendererHints);
}

PlaceEntry::~PlaceEntry()
{
    delete m_sections;
    delete m_dbusIface;
}

bool
PlaceEntry::active() const
{
    // TODO: implement me
    return false;
}

bool
PlaceEntry::running() const
{
    return false;
}

bool
PlaceEntry::urgent() const
{
    return false;
}

QString
PlaceEntry::name() const
{
    return m_name;
}

void
PlaceEntry::setName(QString name)
{
    m_name = name;
}

QString
PlaceEntry::icon() const
{
    return m_icon;
}

void
PlaceEntry::setIcon(QString icon)
{
    m_icon = icon;
}

bool
PlaceEntry::launching() const
{
    /* This basically means no launching animation when opening the device.
       Unity behaves likes this. */
    return false;
}

QString
PlaceEntry::fileName() const
{
    return m_fileName;
}

void
PlaceEntry::setFileName(QString fileName)
{
    m_fileName = fileName;
}

QString
PlaceEntry::groupName() const
{
    return m_groupName;
}

void
PlaceEntry::setGroupName(QString groupName)
{
    m_groupName = groupName;
}

QString
PlaceEntry::dbusName() const
{
    return m_dbusName;
}

QString
PlaceEntry::dbusObjectPath() const
{
    return m_dbusObjectPath;
}

uint
PlaceEntry::position() const
{
    return m_position;
}

QStringList
PlaceEntry::mimetypes() const
{
    return m_mimetypes;
}

bool
PlaceEntry::sensitive() const
{
    return m_sensitive;
}

DeeListModel*
PlaceEntry::sections() const
{
    return m_sections;
}

QMap<QString, QVariant>
PlaceEntry::hints() const
{
    return m_hints;
}

QString
PlaceEntry::entryRendererName() const
{
    return m_entryRendererName;
}

QString
PlaceEntry::entryGroupsModelName() const
{
    return m_entryGroupsModelName;
}

DeeListModel*
PlaceEntry::entryGroupsModel()
{
    if (m_entryGroupsModel == NULL) {
        if (!m_entryGroupsModelName.isNull()) {
            m_entryGroupsModel = new DeeListModel;
            QString path = m_entryGroupsModelName;
            path.replace(".", "/");
            m_entryGroupsModel->setObjectPath("/com/canonical/dee/model/" + path);
            m_entryGroupsModel->setService(m_entryGroupsModelName);
        }
    }
    return m_entryGroupsModel;
}

QString
PlaceEntry::entryResultsModelName() const
{
    return m_entryResultsModelName;
}

DeeListModel*
PlaceEntry::entryResultsModel()
{
    if (m_entryResultsModel == NULL) {
        if (!m_entryResultsModelName.isNull()) {
            m_entryResultsModel = new DeeListModel;
            QString path = m_entryResultsModelName;
            path.replace(".", "/");
            m_entryResultsModel->setObjectPath("/com/canonical/dee/model/" + path);
            m_entryResultsModel->setService(m_entryResultsModelName);
        }
    }
    return m_entryResultsModel;
}

QMap<QString, QVariant>
PlaceEntry::entryRendererHints() const
{
    return m_entryRendererHints;
}

QString
PlaceEntry::globalRendererName() const
{
    return m_globalRendererName;
}

QString
PlaceEntry::globalGroupsModelName() const
{
    return m_globalGroupsModelName;
}

DeeListModel*
PlaceEntry::globalGroupsModel()
{
    if (m_globalGroupsModel == NULL) {
        if (!m_globalGroupsModelName.isNull()) {
            m_globalGroupsModel = new DeeListModel;
            QString path = m_globalGroupsModelName;
            path.replace(".", "/");
            m_globalGroupsModel->setObjectPath("/com/canonical/dee/model/" + path);
            m_globalGroupsModel->setService(m_globalGroupsModelName);
        }
    }
    return m_globalGroupsModel;
}

QString
PlaceEntry::globalResultsModelName() const
{
    return m_globalResultsModelName;
}

DeeListModel*
PlaceEntry::globalResultsModel()
{
    if (m_globalResultsModel == NULL) {
        if (!m_globalResultsModelName.isNull()) {
            m_globalResultsModel = new DeeListModel;
            QString path = m_globalResultsModelName;
            path.replace(".", "/");
            m_globalResultsModel->setObjectPath("/com/canonical/dee/model/" + path);
            m_globalResultsModel->setService(m_globalResultsModelName);
        }
    }
    return m_globalResultsModel;
}

QMap<QString, QVariant>
PlaceEntry::globalRendererHints() const
{
    return m_globalRendererHints;
}

bool
PlaceEntry::online() const
{
    return m_online;
}

void
PlaceEntry::setDbusName(QString dbusName)
{
    m_dbusName = dbusName;
}

void
PlaceEntry::setDbusObjectPath(QString dbusObjectPath)
{
    m_dbusObjectPath = dbusObjectPath;
}

void
PlaceEntry::setPosition(uint position)
{
    if (position != m_position) {
        m_position = position;
        emit positionChanged(position);
    }
}

void
PlaceEntry::setMimetypes(QStringList mimetypes)
{
    m_mimetypes = mimetypes;
    emit mimetypesChanged();
}

void
PlaceEntry::setSensitive(bool sensitive)
{
    if (sensitive != m_sensitive) {
        m_sensitive = sensitive;
        emit sensitiveChanged(sensitive);
    }
}

void
PlaceEntry::setSections(DeeListModel* sections)
{
    if (sections == NULL) {
        return;
    }
    if (m_sections != NULL) {
        delete m_sections;
    }
    m_sections = sections;
    emit sectionsChanged();
}

void
PlaceEntry::setHints(QMap<QString, QVariant> hints)
{
    m_hints = hints;
    emit hintsChanged();
}

void
PlaceEntry::setEntryRendererName(QString entryRendererName)
{
    if (entryRendererName != m_entryRendererName) {
        m_entryRendererName = entryRendererName;
        emit entryRendererNameChanged();
    }
}

void
PlaceEntry::setEntryGroupsModelName(QString entryGroupsModelName)
{
    if (entryGroupsModelName != m_entryGroupsModelName) {
        m_entryGroupsModelName = entryGroupsModelName;
        delete m_entryGroupsModel;
        m_entryGroupsModel = NULL;
        emit entryGroupsModelNameChanged();
    }
}

void
PlaceEntry::setEntryGroupsModel(DeeListModel* entryGroupsModel)
{
    if (entryGroupsModel == NULL) {
        return;
    }
    if (m_entryGroupsModel != NULL) {
        delete m_entryGroupsModel;
    }
    m_entryGroupsModel = entryGroupsModel;
    emit entryGroupsModelChanged();
}

void
PlaceEntry::setEntryResultsModelName(QString entryResultsModelName)
{
    if (entryResultsModelName != m_entryResultsModelName) {
        m_entryResultsModelName = entryResultsModelName;
        delete m_entryResultsModel;
        m_entryResultsModel = NULL;
        emit entryResultsModelNameChanged();
    }
}

void
PlaceEntry::setEntryResultsModel(DeeListModel* entryResultsModel)
{
    if (entryResultsModel == NULL) {
        return;
    }
    if (m_entryResultsModel != NULL) {
        delete m_entryResultsModel;
    }
    m_entryResultsModel = entryResultsModel;
    emit entryResultsModelChanged();
}

void
PlaceEntry::setEntryRendererHints(QMap<QString, QVariant> entryRendererHints)
{
    m_entryRendererHints = entryRendererHints;
    emit entryRendererHintsChanged();
}

void
PlaceEntry::setGlobalRendererName(QString globalRendererName)
{
    if (globalRendererName != m_globalRendererName) {
        m_globalRendererName = globalRendererName;
        emit globalRendererNameChanged();
    }
}

void
PlaceEntry::setGlobalGroupsModelName(QString globalGroupsModelName)
{
    if (globalGroupsModelName != m_globalGroupsModelName) {
        m_globalGroupsModelName = globalGroupsModelName;
        delete m_globalGroupsModel;
        m_globalGroupsModel = NULL;
        emit globalGroupsModelNameChanged();
    }
}

void
PlaceEntry::setGlobalGroupsModel(DeeListModel* globalGroupsModel)
{
    if (globalGroupsModel == NULL) {
        return;
    }
    if (m_globalGroupsModel != NULL) {
        delete m_globalGroupsModel;
    }
    m_globalGroupsModel = globalGroupsModel;
    emit globalGroupsModelChanged();
}

void
PlaceEntry::setGlobalResultsModelName(QString globalResultsModelName)
{
    if (globalResultsModelName != m_globalResultsModelName) {
        m_globalResultsModelName = globalResultsModelName;
        delete m_globalResultsModel;
        m_globalResultsModel = NULL;
        emit globalResultsModelNameChanged();
    }
}

void
PlaceEntry::setGlobalResultsModel(DeeListModel* globalResultsModel)
{
    if (globalResultsModel == NULL) {
        return;
    }
    if (m_globalResultsModel != NULL) {
        delete m_globalResultsModel;
    }
    m_globalResultsModel = globalResultsModel;
    emit globalResultsModelChanged();
}

void
PlaceEntry::setGlobalRendererHints(QMap<QString, QVariant> globalRendererHints)
{
    m_globalRendererHints = globalRendererHints;
    emit globalRendererHintsChanged();
}

void
PlaceEntry::activate()
{
    activateEntry(0);
}

void
PlaceEntry::activateEntry(const int section)
{
    if (!m_sensitive) {
        return;
    }

    QDBusInterface iface("com.canonical.UnityQt", "/dash", "local.DashDeclarativeView");
    iface.call("activatePlaceEntry", m_fileName, m_groupName, section);
}

void
PlaceEntry::createMenuActions()
{
    if (!m_sensitive) {
        return;
    }

    for(int i = 0; i < m_sections->rowCount(); ++i) {
        QAction* section = new QAction(m_menu);
        section->setText(m_sections->data(m_sections->index(i)).toString());
        section->setProperty(SECTION_PROPERTY, QVariant(i));
        m_menu->addAction(section);
        QObject::connect(section, SIGNAL(triggered()), this, SLOT(onSectionTriggered()));
    }
}

void
PlaceEntry::onSectionTriggered()
{
    QAction* action = static_cast<QAction*>(sender());
    int section = action->property(SECTION_PROPERTY).toInt();
    hideMenu(true);
    activateEntry(section);
}

void
PlaceEntry::connectToRemotePlaceEntry()
{
    if (m_online) return;

    m_dbusIface = new QDBusInterface(m_dbusName, m_dbusObjectPath,
                                     UNITY_PLACE_ENTRY_INTERFACE);
    if (!m_dbusIface->isValid()) {
        m_online = false;
        return;
    }

    // Connect to RendererInfoChanged and PlaceEntryInfoChanged signals
    QDBusConnection connection = m_dbusIface->connection();
    connection.connect(m_dbusName, m_dbusObjectPath, UNITY_PLACE_ENTRY_INTERFACE,
                       "RendererInfoChanged", this,
                       SLOT(onRendererInfoChanged(const RendererInfoStruct&)));
    connection.connect(m_dbusName, m_dbusObjectPath, UNITY_PLACE_ENTRY_INTERFACE,
                       "PlaceEntryInfoChanged", this,
                       SLOT(onPlaceEntryInfoChanged(const PlaceEntryInfoStruct&)));

    m_online = true;
}

void
PlaceEntry::updateInfo(const PlaceEntryInfoStruct& info)
{
    if (info.name != "") {
        setName(info.name);
    }
    setIcon(info.icon);
    setPosition(info.position);
    setMimetypes(info.mimetypes);
    setSensitive(info.sensitive);
    setSection(info.sections_model);

    QMap<QString, QVariant> hints;
    QMap<QString, QString>::const_iterator i;
    for(i = info.hints.constBegin(); i != info.hints.constEnd(); ++i) {
        hints[i.key()] = QVariant(i.value());
    }
    setHints(hints);

    setEntryRendererName(info.entry_renderer_info.default_renderer);
    setEntryGroupsModelName(info.entry_renderer_info.groups_model);
    setEntryResultsModelName(info.entry_renderer_info.results_model);

    QMap<QString, QVariant> entryRendererHints;
    for(i = info.entry_renderer_info.renderer_hints.constBegin();
        i != info.entry_renderer_info.renderer_hints.constEnd(); ++i) {
        entryRendererHints[i.key()] = QVariant(i.value());
    }
    setEntryRendererHints(entryRendererHints);

    setGlobalRendererName(info.global_renderer_info.default_renderer);
    setGlobalGroupsModelName(info.global_renderer_info.groups_model);
    setGlobalResultsModelName(info.global_renderer_info.results_model);

    QMap<QString, QVariant> globalRendererHints;
    for(i = info.global_renderer_info.renderer_hints.constBegin();
        i != info.global_renderer_info.renderer_hints.constEnd(); ++i) {
        globalRendererHints[i.key()] = QVariant(i.value());
    }
    setGlobalRendererHints(globalRendererHints);

    //emit updated();
    //emit rendererInfoChanged();
}

void
PlaceEntry::setSection(const QString& sectionModelName)
{
    DeeListModel* sections = new DeeListModel;
    QString path = m_dbusName;
    path.replace(".", "/");
    sections->setObjectPath("/com/canonical/dee/model/" + path + "/SectionsModel");
    sections->setService(sectionModelName);
    setSections(sections);
}

void
PlaceEntry::onRendererInfoChanged(const RendererInfoStruct& r)
{
    // TODO
    //emit rendererInfoChanged();
}

void
PlaceEntry::onPlaceEntryInfoChanged(const PlaceEntryInfoStruct& p)
{
    updateInfo(p);
    //emit rendererInfoChanged();
}

