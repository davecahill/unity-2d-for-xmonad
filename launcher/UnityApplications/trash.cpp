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

#include "trash.h"
#include "launchermenu.h"
#include "launcherutility.h"

#include "bamf-application.h"
#include "bamf-window.h"
#include "bamf-matcher.h"

#include "config.h"

// libunity-2d
#include <unity2dtr.h>
#include <debug_p.h>

#include <QAction>

#define TRASH_URI "trash://"

Trash::Trash()
{
    m_trash = g_file_new_for_uri(TRASH_URI);
    setShortcutKey(Qt::Key_T);
    updateTrashIcon();
    startMonitoringTrash();
}

Trash::Trash(const Trash& other)
{
}

Trash::~Trash()
{
    g_object_unref(m_monitor);
    g_object_unref(m_trash);
}

bool
Trash::active() const
{
    return false;
}

bool
Trash::isTrashWindow(WnckWindow* window) const
{
    QString windowName = QString(wnck_window_get_name(window));
    int found = QString::compare(u2dTr("Trash", "nautilus"), windowName, Qt::CaseSensitive);
    return found == 0;
}

QList<WnckWindow*>
Trash::trashWindows() const
{
    QList<WnckWindow*> trashWindows;
    BamfMatcher& matcher = BamfMatcher::get_default();
    QScopedPointer<BamfApplicationList> running_applications(matcher.running_applications());
    BamfApplication* bamfApplication;

    for(int i=0; i<running_applications->size(); i++) {
        bamfApplication = running_applications->at(i);

        QScopedPointer<BamfWindowList> windowApplications(bamfApplication->windows());

        for (int j=0; j < windowApplications->size(); j++) {
            BamfWindow *bamfWindow = windowApplications->at(j);
            WnckWindow* wnckWindow = wnck_window_get(bamfWindow->xid());

            if (wnckWindow != NULL && isTrashWindow(wnckWindow)) {
                trashWindows.append(wnckWindow);
            }
        }
    }

    return trashWindows;
}

bool
Trash::running() const
{
    return trashWindows().length() > 0;
}

void
Trash::show()
{
    Q_FOREACH(WnckWindow* wnckWindow, trashWindows()) {
        LauncherUtility::showWindow(wnckWindow);
    }
}

int
Trash::windowCount() const
{
    return 0;
}

bool
Trash::urgent() const
{
    return false;
}

QString
Trash::name() const
{
    return u2dTr("Trash");
}

QString
Trash::icon() const
{
    return m_iconName;
}

bool
Trash::launching() const
{
    /* This basically means no launching animation when opening the trash.
       Unity behaves likes this. */
    return false;
}

void
Trash::activate()
{
    if (running()) {
        show();
    } else {
        open();
    }
}

void
Trash::launchNewInstance()
{
    open();
}

void
Trash::open() const
{
    GError* error = NULL;
    if (!g_app_info_launch_default_for_uri(TRASH_URI, NULL, &error)) {
        if (error != NULL) {
            UQ_WARNING << "Unable to open the trash folder:" << error->message;
            g_error_free(error);
        }
    }
}

void
Trash::empty() const
{
    recursiveDelete(m_trash);
}

int
Trash::count() const
{
    GError* error = NULL;
    GFileInfo* info = g_file_query_info(m_trash,
        G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT,
        G_FILE_QUERY_INFO_NONE, NULL, &error);
    if (error != NULL) {
        UQ_WARNING << "Unable to obtain the number of items in the trash:"
                   << error->message;
        g_error_free(error);
        return 0;
    }

    guint32 count = g_file_info_get_attribute_uint32(info,
        G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT);
    g_object_unref(info);

    return count;
}

void
Trash::recursiveDelete(GFile* dir)
{
    GError* error = NULL;
    QString attributes;
    attributes += G_FILE_ATTRIBUTE_STANDARD_NAME;
    attributes += ",";
    attributes += G_FILE_ATTRIBUTE_STANDARD_TYPE;
    GFileEnumerator* children = g_file_enumerate_children(dir,
        attributes.toAscii().constData(), G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
        NULL, &error);
    if (error != NULL) {
        char* uri = g_file_get_uri(dir);
        UQ_WARNING << "Unable to recursively delete files in" << uri << ":"
                   << error->message;
        g_free(uri);
        g_error_free(error);
        return;
    }

    GFileInfo* info = NULL;
    while ((info = g_file_enumerator_next_file(children, NULL, &error)) != NULL) {
        GFile* child = g_file_get_child(dir, g_file_info_get_name(info));
        if (g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY) {
            recursiveDelete(child);
        }

        /* If passed a GError* as third parameter, g_file_delete incorrectly
           reports an error when attempting to delete a file in a subfolder in
           the trash with the following error message:
           "Items in the trash cannot be modified".
           Despite the error, the file (or folder) is actually deleted.
           For now let’s just ignore errors…
           Note: the same happens in Unity. */
        g_file_delete(child, NULL, NULL);

        g_object_unref(child);
        g_object_unref(info);
    }
    g_object_unref(children);

    if (error != NULL) {
        char* uri = g_file_get_uri(dir);
        UQ_WARNING << "Unable to recursively delete files in" << uri << ":"
                   << error->message;
        g_free(uri);
        g_error_free(error);
    }
}

void
Trash::createMenuActions()
{
    int c = count();

    if (c == 0) return;

    QAction* contents = new QAction(m_menu);
    contents->setText(u2dTr("%n item in trash", "%n items in trash", c));
    contents->setEnabled(false);
    m_menu->addAction(contents);

    m_menu->addSeparator();

    QAction* empty = new QAction(m_menu);
    empty->setText(u2dTr("Empty Trash"));
    m_menu->addAction(empty);
    QObject::connect(empty, SIGNAL(triggered()), this, SLOT(onEmptyTriggered()));
}

void
Trash::onEmptyTriggered()
{
    m_menu->hide();
    empty();
}

void
Trash::onDragEnter(DeclarativeDragDropEvent* event)
{
    Q_FOREACH(QUrl url, event->mimeData()->urls()) {
        if (url.scheme() == "file") {
            event->setDropAction(Qt::MoveAction);
            event->setAccepted(true);
            return;
        }
    }
}

void
Trash::onDrop(DeclarativeDragDropEvent* event)
{
    Q_FOREACH(QUrl url, event->mimeData()->urls()) {
        if (url.scheme() == "file") {
            GFile* file = g_file_new_for_path(url.toLocalFile().toUtf8().constData());
            if (!g_file_trash(file, NULL, NULL)) {
                UQ_WARNING << "Unable to send" << url << "to the trash";
            }
            g_object_unref(file);
        }
    }
}


Trashes::Trashes(QObject* parent) :
    QAbstractListModel(parent)
{
    m_trash = new Trash;
}

Trashes::~Trashes()
{
    delete m_trash;
}

int
Trashes::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)

    return 1;
}

QVariant
Trashes::data(const QModelIndex& index, int role) const
{
    Q_UNUSED(role)

    if (!index.isValid()) {
        return QVariant();
    }

    return QVariant::fromValue(m_trash);
}

void
Trash::fileChangedProxy(GFileMonitor      *file_monitor,
              GFile             *child,
              GFile             *other_file,
              GFileMonitorEvent  event_type,
              gpointer           data)
{
    Trash* _this = static_cast<Trash*>(data);
    switch (event_type)
    {
        case (G_FILE_MONITOR_EVENT_DELETED || G_FILE_MONITOR_EVENT_CREATED):
        return _this->fileChanged(/*file_monitor, child, other_file, event_type */);
        break;
    default: ;
    }
    return _this->fileChanged(/*file_monitor, child, other_file, event_type */);
}

void
Trash::fileChanged(/*GFileMonitor      *file_monitor,
              GFile             *child,
              GFile             *other_file,
              GFileMonitorEvent  event_type*/)
{
    updateTrashIcon();
    emit iconChanged(icon());
}

void
Trash::startMonitoringTrash(void)
{
    GFile *file;

    file = g_file_new_for_uri ("trash://");
    m_monitor = g_file_monitor_directory (file, G_FILE_MONITOR_NONE, NULL, NULL);
    g_object_unref(file);

    g_signal_connect(m_monitor, "changed", G_CALLBACK(Trash::fileChangedProxy), this);
}

void
Trash::updateTrashIcon(void)
{
    if(count() == 0) {
        m_iconName = "unity-icon-theme/user-trash"; }
    else {
        m_iconName = "unity-icon-theme/user-trash-full"; }
}
