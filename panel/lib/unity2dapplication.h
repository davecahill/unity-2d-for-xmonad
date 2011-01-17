/*
 * Unity2d
 *
 * Copyright 2010 Canonical Ltd.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef UNITY2DAPPLICATION_H
#define UNITY2DAPPLICATION_H

// Qt
#include <QApplication>

class Unity2dApplication;

class AbstractX11EventFilter
{
public:
    virtual ~AbstractX11EventFilter();

protected:
    virtual bool x11EventFilter(XEvent*) = 0;

    friend class Unity2dApplication;
};

class Unity2dApplication : public QApplication
{
Q_OBJECT
public:
    Unity2dApplication(int& argc, char** argv);

    void installX11EventFilter(AbstractX11EventFilter*);
    void removeX11EventFilter(AbstractX11EventFilter*);

    /**
     * Note: This function will return a null pointer if you did not use a Unity2dApplication in your application!
     */
    static Unity2dApplication* instance();

protected:
    bool x11EventFilter(XEvent*);

private:
    QList<AbstractX11EventFilter*> m_x11EventFilters;
};

#endif // UNITY2DAPPLICATION_H
