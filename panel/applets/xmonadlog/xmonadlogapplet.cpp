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

// Self
#include "xmonadlogapplet.h"

// Local
#include <config.h>

// QT
#include <QPixmap>
#include <QHBoxLayout>

XmonadLogApplet::XmonadLogApplet(Unity2dPanel* panel) :
    Unity2d::PanelApplet(panel),
    x_log(new QLabel())
{
    x_log->setText("<b>trololo</b>");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(x_log);
}

#include "xmonadlogapplet.moc"
