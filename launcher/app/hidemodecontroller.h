/*
 * This file is part of unity-2d
 *
 * Copyright 2011 Canonical Ltd.
 *
 * Authors:
 * - Aurélien Gâteau <aurelien.gateau@canonical.com>
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
#ifndef HIDEMODECONTROLLER_H
#define HIDEMODECONTROLLER_H

// Local

// Qt
#include <QObject>

class GConfItemQmlWrapper;

class AbstractHideBehavior;
class Unity2dPanel;

/**
 * This class monitors the hide_mode gconf key and set up an HideController
 * depending on its value
 */
class HideModeController : public QObject
{
Q_OBJECT
public:
    HideModeController(Unity2dPanel* panel);
    ~HideModeController();

    Q_INVOKABLE void beginForceVisible();
    Q_INVOKABLE void endForceVisible();

private Q_SLOTS:
    void update();

private:
    enum AutoHideMode {
        ManualHide,
        AutoHide,
        IntelliHide
    };
    Q_DISABLE_COPY(HideModeController);
    Unity2dPanel* m_panel;
    GConfItemQmlWrapper* m_hideModeKey;
    AbstractHideBehavior* m_hideBehavior;
    int m_forceVisibleCount;
};

#endif /* HIDEMODECONTROLLER_H */
