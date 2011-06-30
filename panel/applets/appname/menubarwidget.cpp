/*
 * This file is part of unity-2d
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Self
#include <menubarwidget.h>

// Local
#include <debug_p.h>
#include <indicatorentrywidget.h>
#include <indicatorsmanager.h>

// Qt
#include <QHBoxLayout>

static const int MENU_ITEM_PADDING = 6;

MenuBarWidget::MenuBarWidget(IndicatorsManager* indicatorsManager, QWidget* parent)
: QWidget(parent)
, m_indicatorsManager(indicatorsManager)
, m_layout(new QHBoxLayout(this))
, m_isEmpty(true)
, m_isOpened(false)
{
    m_layout->setMargin(0);
    m_layout->setSpacing(0);
    indicatorsManager->indicators()->on_object_added.connect(
        sigc::mem_fun(this, &MenuBarWidget::onObjectAdded)
        );
    indicatorsManager->indicators()->on_entry_activated.connect(
        sigc::mem_fun(this, &MenuBarWidget::onEntryActivated)
        );
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_layout->addStretch();
}

bool MenuBarWidget::isEmpty() const
{
    return m_isEmpty;
}

bool MenuBarWidget::isOpened() const
{
    // FIXME
    return m_isOpened;
}

void MenuBarWidget::onObjectAdded(const unity::indicator::Indicator::Ptr& indicator)
{
    QString name = QString::fromStdString(indicator->name());
    if (name == "libappmenu.so") {
        indicator->on_entry_added.connect(sigc::mem_fun(this, &MenuBarWidget::onEntryAdded));
    }
}

void MenuBarWidget::onEntryAdded(const unity::indicator::Entry::Ptr& entry)
{
    IndicatorEntryWidget* widget = new IndicatorEntryWidget(entry);
    widget->setPadding(MENU_ITEM_PADDING);
    connect(widget, SIGNAL(isEmptyChanged()), SLOT(updateIsEmpty()));

    m_widgetList.append(widget);
    m_indicatorsManager->addIndicatorEntryWidget(widget);

    // Insert *before* stretch
    m_layout->insertWidget(m_layout->count() - 1, widget);
}

void MenuBarWidget::updateIsEmpty()
{
    bool empty = true;
    Q_FOREACH(IndicatorEntryWidget* widget, m_widgetList) {
        if (!widget->isEmpty()) {
            empty = false;
            break;
        }
    }
    if (m_isEmpty != empty) {
        m_isEmpty = empty;
        isEmptyChanged();
    }
}

void MenuBarWidget::onEntryActivated(const std::string& id)
{
    bool isOpened = false;
    if (!id.empty()) {
        // We only cares about menubar entries
        Q_FOREACH(IndicatorEntryWidget* widget, m_widgetList) {
            if (widget->entry()->id() == id) {
                isOpened = true;
                break;
            }
        }
    }
    if (m_isOpened != isOpened) {
        m_isOpened = isOpened;
        isOpenedChanged();
    }
}

#include "menubarwidget.moc"
