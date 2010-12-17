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

#include <gtk/gtk.h>
#include <QApplication>
#include <QDesktopWidget>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QPaintEngine>
#include <QPixmap>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

#include "config.h"
#include "spreadview.h"
#include "dbusproxy.h"

#include <QDebug>
#include <QTimer>
#include <QX11Info>

int main(int argc, char *argv[])
{
    /* UnityApplications plugin uses GTK APIs to retrieve theme icons
       (gtk_icon_theme_get_default) and requires a call to gtk_init */
    gtk_init(&argc, &argv);

    QApplication::setGraphicsSystem("raster");
    QApplication::setColorSpec(QApplication::ManyColor);

    QApplication application(argc, argv);

    SpreadView view;

    view.setAttribute(Qt::WA_X11NetWmWindowTypeDock);
    view.setAttribute(Qt::WA_OpaquePaintEvent);
    view.setAttribute(Qt::WA_NoSystemBackground);
    view.setResizeMode(QDeclarativeView::SizeRootObjectToView);
    view.setFocus();
    view.engine()->addImportPath(unityQtImportPath());

    /* Always fit the available space on the desktop */
    view.fitToAvailableSpace(QApplication::desktop()->screenNumber(&view));
    QObject::connect(QApplication::desktop(), SIGNAL(workAreaResized(int)),
                     &view, SLOT(fitToAvailableSpace(int)));

    // This is needed for UnityApplications and UnityPlaces
    view.engine()->addImportPath(unityQtImportPath() + "/../launcher/");
    view.engine()->addImportPath(unityQtImportPath() + "/../places/");
    view.engine()->setBaseUrl(QUrl::fromLocalFile(unityQtDirectory() + "/spread/"));

    DBusProxy *proxy = new DBusProxy();
    proxy->setView(&view);
    view.rootContext()->setContextProperty("control", proxy);

    view.setSource(QUrl("./Spread.qml"));
    //view.showMaximized();

    application.connect(view.engine(), SIGNAL(quit()), SLOT(quit()));

    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.registerService("com.canonical.UnityQtSpread");
    bus.registerObject("/spread", proxy, QDBusConnection::ExportAllContents);

    return application.exec();
}

