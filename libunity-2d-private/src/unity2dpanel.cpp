/*
 * This file is part of unity-2d
 *
 * Copyright 2010 Canonical Ltd.
 *
 * Authors:
 * - Aurélien Gâteau <aurelien.gateau@canonical.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; version 3.
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

// Self
#include "unity2dpanel.h"
#include "strutmanager.h"
#include <debug_p.h>
#include <indicatorsmanager.h>

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QPainter>
#include <QPropertyAnimation>
#include <QHBoxLayout>
#include <QTimer>
#include <QX11Info>

// unity-2d
#include "screeninfo.h"

static const int SLIDE_DURATION = 125;
static const int REARRANGE_INTERVALL = 2000; // The intervall between fallback geometry updates in ms

struct Unity2dPanelPrivate
{
    Unity2dPanel* q;
    Unity2dPanel::Edge m_edge;
    mutable IndicatorsManager* m_indicatorsManager;
    QHBoxLayout* m_layout;
    int m_delta;
    bool m_manualSliding;
    StrutManager m_strutManager;
    ScreenInfo* m_screenInfo;

    void setStrut(ulong* struts)
    {
        static Atom atom = XInternAtom(QX11Info::display(), "_NET_WM_STRUT_PARTIAL", False);
        XChangeProperty(QX11Info::display(), q->effectiveWinId(), atom,
                        XA_CARDINAL, 32, PropModeReplace,
                        (unsigned char *) struts, 12);
    }

    void reserveStrut()
    {
        QDesktopWidget* desktop = QApplication::desktop();
        const int primscr = desktop->primaryScreen();
        const QRect screen = desktop->screenGeometry(primscr);
        const QRect available = desktop->availableGeometry(primscr);

        ulong struts[12] = {};
        switch (m_edge) {
        case Unity2dPanel::LeftEdge:
            if (QApplication::isLeftToRight()) {
                struts[0] = q->width();
                struts[4] = available.top();
                struts[5] = available.y() + available.height();
            } else {
                struts[1] = q->width();
                struts[6] = available.top();
                struts[7] = available.y() + available.height();
            }
            break;
        case Unity2dPanel::TopEdge:
            struts[2] = q->height();
            struts[8] = screen.left();
            struts[9] = screen.x() + screen.width() - 1; //otherwise xmonad thinks panel is on all screens
            break;
        }

        setStrut(struts);
    }

    void releaseStrut()
    {
        ulong struts[12];
        memset(struts, 0, sizeof struts);
        setStrut(struts);
    }

    void updateGeometry()
    {
        qDebug() << "-- Geometry Update! --";
        QDesktopWidget* desktop = QApplication::desktop();
        const int primscr = desktop->primaryScreen();
        const QRect screen = desktop->screenGeometry(primscr);
        const QRect available = desktop->availableGeometry(primscr);

        //unity 5.4
        const QRect screen = m_screenInfo->geometry();
        const QRect available = m_screenInfo->availableGeometry();

        QRect rect;
        switch (m_edge) {
        case Unity2dPanel::LeftEdge:
            if (QApplication::isLeftToRight()) {
                rect = QRect(screen.left(), available.top() + 24, q->width(), available.height() - 24);
                rect.moveLeft(m_delta);
            } else {
                rect = QRect(screen.right() - available.width(), available.top(), q->width(), available.height());
                rect.moveRight(screen.right() - m_delta);
            }
            break;
        case Unity2dPanel::TopEdge:
            rect = QRect(screen.left(), screen.top(), screen.width(), q->height());
            rect.moveTop(m_delta);
            break;
        }

        q->setGeometry(rect);
    }

    void updateLayoutDirection()
    {
        QBoxLayout::Direction direction;
        switch(m_edge) {
        case Unity2dPanel::TopEdge:
            direction = QBoxLayout::LeftToRight;
            break;
        case Unity2dPanel::LeftEdge:
            direction = QBoxLayout::TopToBottom;
            break;
        }
        m_layout->setDirection(direction);
    }

    void updateEdge()
    {
        updateGeometry();
        updateLayoutDirection();
    }
};

Unity2dPanel::Unity2dPanel(bool requiresTransparency, int screen, ScreenInfo::Corner corner, QWidget* parent)
: QWidget(parent)
, d(new Unity2dPanelPrivate)
{
    d->m_strutManager.setWidget(this);
    d->m_strutManager.setEdge(Unity2dPanel::TopEdge);
    d->q = this;
    d->m_edge = Unity2dPanel::TopEdge;
    d->m_indicatorsManager = 0;
    d->m_delta = 0;
    d->m_manualSliding = false;
    d->m_layout = new QHBoxLayout(this);
    d->m_layout->setMargin(0);
    d->m_layout->setSpacing(0);
    if (corner != ScreenInfo::InvalidCorner) {
        d->m_screenInfo = new ScreenInfo(corner, this);
    } else if (screen >= 0) {
        d->m_screenInfo = new ScreenInfo(screen, this);
    } else {
        d->m_screenInfo = new ScreenInfo(this, this);
    }

    setAttribute(Qt::WA_X11NetWmWindowTypeDock);
    setAttribute(Qt::WA_Hover);

    if (QX11Info::isCompositingManagerRunning() && requiresTransparency) {
        setAttribute(Qt::WA_TranslucentBackground);
    } else {
        setAutoFillBackground(true);
    }

    //unity 5.4
    connect(&d->m_strutManager, SIGNAL(enabledChanged(bool)), SIGNAL(useStrutChanged(bool)));

    /* Geometry Update Triggers */
    connect(QApplication::desktop(), SIGNAL(workAreaResized(int)), SLOT(slotWorkAreaResized(int)));
    connect(QApplication::desktop(), SIGNAL(screenCountChanged(int)), SLOT(slotScreenCountChanged(int)));

    /* Fallback intervall based trigger (If the ones above don't do it */
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(slotFallbackGeometryUpdate()));
    timer->start(REARRANGE_INTERVALL);
}

Unity2dPanel::~Unity2dPanel()
{
    delete d;
}

void Unity2dPanel::setEdge(Unity2dPanel::Edge edge)
{
    d->m_edge = edge;
    d->m_strutManager.setEdge(edge);
    if (isVisible()) {
        d->updateEdge();
    }
}

Unity2dPanel::Edge Unity2dPanel::edge() const
{
    return d->m_edge;
}

void Unity2dPanel::setScreen(int screen)
{
    d->m_screenInfo->setScreen(screen);
}

int Unity2dPanel::screen() const
{
    return d->m_screenInfo->screen();
}

IndicatorsManager* Unity2dPanel::indicatorsManager() const
{
    if (d->m_indicatorsManager == 0) {
        auto const_this = const_cast<Unity2dPanel*>(this);
        d->m_indicatorsManager = new IndicatorsManager(const_this, const_this);
    }

    return d->m_indicatorsManager;
}

void Unity2dPanel::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    d->updateEdge();
    d->m_slideOutAnimation->setEndValue(-panelSize());
}

//unity 5.4
/*
void Unity2dPanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    d->m_slideOutAnimation->setEndValue(-panelSize());
    d->updateEdge();
}

void Unity2dPanel::slotScreenCountChanged(int screenno) {
    d->m_slideOutAnimation->setEndValue(-panelSize());
    d->updateEdge();
}
*/

void Unity2dPanel::slotWorkAreaResized(int screen)
{
    if (x11Info().screen() == screen) {
        d->updateEdge();
    }
}

/**
 * Fallback update. In case the others do not work.
 * Currently uses a timer.
 */
void Unity2dPanel::slotFallbackGeometryUpdate()
{
    d->updateEdge();
}

void Unity2dPanel::paintEvent(QPaintEvent* event)
{
    // Necessary because Oxygen thinks it knows better what to paint in the background
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), palette().brush(QPalette::Background));
}

void Unity2dPanel::addWidget(QWidget* widget)
{
    d->m_layout->addWidget(widget);
}

void Unity2dPanel::addSpacer()
{
    d->m_layout->addStretch();
}

bool Unity2dPanel::useStrut() const
{
    return d->m_strutManager.enabled();
}

void Unity2dPanel::setUseStrut(bool value)
{
    d->m_strutManager.setEnabled(value);
}

int Unity2dPanel::delta() const
{
    return d->m_delta;
}

void Unity2dPanel::setDelta(int delta)
{
    /* Clamp delta to be between 0 and minus its size */
    int minDelta = -panelSize();
    int maxDelta = 0;

    d->m_delta = qMax(qMin(delta, maxDelta), minDelta);
    d->updateGeometry();
}

int Unity2dPanel::panelSize() const
{
    return (d->m_edge == Unity2dPanel::TopEdge) ? height() : width();
}

bool Unity2dPanel::manualSliding() const
{
    return d->m_manualSliding;
}

void Unity2dPanel::setManualSliding(bool manualSliding)
{
    if (d->m_manualSliding != manualSliding) {
        d->m_manualSliding = manualSliding;
        Q_EMIT manualSlidingChanged(d->m_manualSliding);
    }
}

QString Unity2dPanel::id() const
{
    int screen = QApplication::desktop()->screenNumber(this);
    return "Unity2DPanel" + QString::number(screen);
}

#include "unity2dpanel.moc"
