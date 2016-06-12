/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef COMPRESSFILEITEMACTION_H
#define COMPRESSFILEITEMACTION_H

#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>

class QAction;
class QWidget;

class KFileItemListProperties;

namespace Kerfuffle
{
class PluginManager;
}

class CompressFileItemAction : public KAbstractFileItemActionPlugin
{

Q_OBJECT

public:
    CompressFileItemAction(QObject* parent, const QVariantList& args);

    virtual QList<QAction*> actions(const KFileItemListProperties& fileItemInfos, QWidget* parentWidget) Q_DECL_OVERRIDE;

private:
    QAction *createAction(const QIcon& icon, const QString& name, QWidget *parent, const QList<QUrl>& urls, const QString& exec);

    Kerfuffle::PluginManager *m_pluginManager;
};

#endif
