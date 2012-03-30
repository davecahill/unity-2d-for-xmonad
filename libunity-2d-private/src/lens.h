/*
 * Copyright (C) 2011 Canonical, Ltd.
 *
 * Authors:
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

#ifndef LENS_H
#define LENS_H

// Local

// Qt
#include <QObject>
#include <QString>
#include <QMetaType>

// libunity-core
#include <UnityCore/Lens.h>

// dee-qt
#include "deelistmodel.h"

class Filters;

class Lens : public QObject
{
    Q_OBJECT
    Q_ENUMS(ViewType)

    Q_PROPERTY(QString id READ id NOTIFY idChanged)
    Q_PROPERTY(QString dbusName READ dbusName NOTIFY dbusNameChanged)
    Q_PROPERTY(QString dbusPath READ dbusPath NOTIFY dbusPathChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString iconHint READ iconHint NOTIFY iconHintChanged)
    Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
    Q_PROPERTY(QString searchHint READ searchHint NOTIFY searchHintChanged)
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
    Q_PROPERTY(bool searchInGlobal READ searchInGlobal NOTIFY searchInGlobalChanged)
    Q_PROPERTY(QString shortcut READ shortcut NOTIFY shortcutChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(DeeListModel* results READ results NOTIFY resultsChanged)
    Q_PROPERTY(DeeListModel* globalResults READ globalResults NOTIFY globalResultsChanged)
    Q_PROPERTY(DeeListModel* categories READ categories NOTIFY categoriesChanged)
    Q_PROPERTY(ViewType viewType READ viewType WRITE setViewType NOTIFY viewTypeChanged)
    Q_PROPERTY(Filters* filters READ filters NOTIFY filtersChanged)

    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(QString globalSearchQuery READ globalSearchQuery WRITE setGlobalSearchQuery NOTIFY globalSearchQueryChanged)
    Q_PROPERTY(QString noResultsHint READ noResultsHint WRITE setNoResultsHint NOTIFY noResultsHintChanged)

public:
    explicit Lens(QObject *parent = 0);

    enum ViewType {
        Hidden,
        HomeView,
        LensView
    };

    /* getters */
    QString id() const;
    QString dbusName() const;
    QString dbusPath() const;
    QString name() const;
    QString iconHint() const;
    QString description() const;
    QString searchHint() const;
    bool visible() const;
    bool searchInGlobal() const;
    QString shortcut() const;
    bool connected() const;
    DeeListModel* results() const;
    DeeListModel* globalResults() const;
    DeeListModel* categories() const;
    ViewType viewType() const;
    Filters* filters() const;
    QString searchQuery() const;
    QString globalSearchQuery() const;
    QString noResultsHint() const;

    /* setters */
    void setViewType(const ViewType& viewType);
    void setSearchQuery(const QString& search_query);
    void setGlobalSearchQuery(const QString& search_query);
    void setNoResultsHint(const QString& hint);

    Q_INVOKABLE void activate(const QString& uri);
    void setUnityLens(unity::dash::Lens::Ptr lens);

Q_SIGNALS:
    void idChanged(std::string);
    void dbusNameChanged(std::string);
    void dbusPathChanged(std::string);
    void nameChanged(std::string);
    void iconHintChanged(std::string);
    void descriptionChanged(std::string);
    void searchHintChanged(std::string);
    void visibleChanged(bool);
    void searchInGlobalChanged(bool);
    void shortcutChanged(std::string);
    void connectedChanged(bool);
    void resultsChanged();
    void globalResultsChanged();
    void categoriesChanged();
    void viewTypeChanged(ViewType);
    void filtersChanged();
    void searchFinished(unity::dash::Lens::Hints const&);
    void globalSearchFinished(unity::dash::Lens::Hints const&);
    void searchQueryChanged();
    void globalSearchQueryChanged();
    void noResultsHintChanged();

private Q_SLOTS:
    void synchronizeStates();
    void onSearchFinished(unity::dash::Lens::Hints const &);

private:
    void onResultsSwarmNameChanged(std::string);
    void onResultsChanged(unity::dash::Results::Ptr);
    void onGlobalResultsSwarmNameChanged(std::string);
    void onGlobalResultsChanged(unity::dash::Results::Ptr);
    void onCategoriesSwarmNameChanged(std::string);
    void onCategoriesChanged(unity::dash::Categories::Ptr);
    void onViewTypeChanged(unity::dash::ViewType);

    void onActivated(std::string const& uri, unity::dash::HandledType type, unity::dash::Lens::Hints const&);
    void fallbackActivate(const QString& uri);

    unity::dash::Lens::Ptr m_unityLens;
    DeeListModel* m_results;
    DeeListModel* m_globalResults;
    DeeListModel* m_categories;
    QString m_searchQuery;
    QString m_globalSearchQuery;
    QString m_noResultsHint;
    Filters* m_filters;
};

Q_DECLARE_METATYPE(Lens*)

#endif // LENS_H
