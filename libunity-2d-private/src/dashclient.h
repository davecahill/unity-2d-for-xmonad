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
#ifndef DASHCLIENT_H
#define DASHCLIENT_H

// Local

// Qt
#include <QObject>
#include <QSize>

class QDBusInterface;

/**
 * Monitors the dash and provide a single point of entry to its status
 */
class DashClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool alwaysFullScreen READ alwaysFullScreen NOTIFY alwaysFullScreenChanged)

public:
    static DashClient* instance();

    bool active() const;
    /**
     * Returns the active page. This is either:
     * - The lens id of the active lens
     * - "home" if the home is visible
     * - "" if the dash is not visible
     */
    QString activePage() const;
    bool alwaysFullScreen() const;
    void setActivePage(const QString& page, const QString& lensId=QString());
    static QSize minimumSizeForDesktop();

Q_SIGNALS:
    void activeChanged(bool active);
    void activePageChanged(const QString&);
    void alwaysFullScreenChanged();

private Q_SLOTS:
    void connectToDash();
    void slotDashActiveChanged(bool);
    void slotDashActiveLensChanged(const QString&);
    void updateAlwaysFullScreen();

private:
    DashClient(QObject* parent = 0);
    void updateActivePage();

    QDBusInterface* m_dashDbusIface;
    bool m_dashActive;
    QString m_dashActiveLens;
    QString m_activePage;
    bool m_alwaysFullScreen;
};

#endif /* DASHCLIENT_H */
