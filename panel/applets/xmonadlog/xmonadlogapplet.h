/*
 * This file is part of unity-2d
 *
 * Copyright 2011 Canonical Ltd.
 *
 * Authors:
 * - Ugo Riboni <ugo.riboni@canonical.com>
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

#ifndef XMONADLOGAPPLET_H
#define XMONADLOGAPPLET_H

// Qt
#include <QLabel>
#include <QtDBus>

// Unity-2d
#include <panelapplet.h>

using namespace Unity2d;

class XmonadLogApplet : public Unity2d::PanelApplet
{
Q_OBJECT
public:
    XmonadLogApplet(Unity2dPanel* panel);

public Q_SLOTS:
    void logReceived(const QDBusMessage &msg);

private:
    Q_DISABLE_COPY(XmonadLogApplet);
    QLabel* x_log;
};

#endif /* XMONADLOGAPPLET_H */
