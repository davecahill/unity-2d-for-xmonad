/*
 * This file is part of unity-qt
 *
 * Copyright 2010 Canonical Ltd.
 *
 * Authors:
 * - Aurélien Gâteau <aurelien.gateau@canonical.com>
 *
 * License: GPL v3
 */
#ifndef UNITYQTSTYLE_H
#define UNITYQTSTYLE_H

// Local

// Qt
#include <QProxyStyle>

class UnityQtStyle : public QProxyStyle
{
public:
    UnityQtStyle();

    virtual void drawControl(ControlElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget = 0 ) const;
};

#endif /* UNITYQTSTYLE_H */