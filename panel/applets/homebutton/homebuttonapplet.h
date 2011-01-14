/*
 * This file is part of unity-2d
 *
 * Copyright 2010 Canonical Ltd.
 *
 * Authors:
 * - Aurélien Gâteau <aurelien.gateau@canonical.com>
 *
 * License: GPL v3
 */
#ifndef HOMEBUTTONAPPLET_H
#define HOMEBUTTONAPPLET_H

// Local
#include <applet.h>

// Qt
#include <QToolButton>

class QDBusInterface;

class HomeButtonApplet : public Unity2d::Applet
{
Q_OBJECT
public:
    HomeButtonApplet();

private Q_SLOTS:
    void toggleDash();
    void connectToDash();

private:
    Q_DISABLE_COPY(HomeButtonApplet)
    QToolButton* m_button;
    QDBusInterface* m_dashInterface;
};

#endif /* HOMEBUTTONAPPLET_H */
