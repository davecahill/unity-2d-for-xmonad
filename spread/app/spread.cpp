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

#include <gtk/gtk.h>
#include <QApplication>
#include <QDesktopWidget>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>

#include "spreadview.h"
#include "spreadcontrol.h"

#include "config.h"

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

    if (!isRunningInstalled()) {
        /* Spread.qml imports UnityApplications, which is part of the launcher
           component */
        view.engine()->addImportPath(unityQtDirectory() + "/launcher/");
        /* Spread.qml imports UnityPlaces, which is part of the places
           component */
        view.engine()->addImportPath(unityQtDirectory() + "/places/");
    }

    view.engine()->setBaseUrl(QUrl::fromLocalFile(unityQtDirectory() + "/spread/"));

    /* FIXME: the SpreadControl class should be exposed to QML by a plugin and
              instantiated on the QML side */
    SpreadControl control;
    control.connectToBus();

    view.rootContext()->setContextProperty("spreadView", &view);
    view.rootContext()->setContextProperty("control", &control);

    view.setSource(QUrl("./Spread.qml"));

    application.connect(view.engine(), SIGNAL(quit()), SLOT(quit()));

    return application.exec();
}

