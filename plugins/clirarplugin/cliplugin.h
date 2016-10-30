/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2010 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (C) 2015-2016 Ragnar Thomsen <rthomsen6@gmail.com>
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
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

#ifndef CLIPLUGIN_H
#define CLIPLUGIN_H

#include "kerfuffle/cliinterface.h"
#include "kerfuffle/cliparameters.h"

class CliPlugin : public Kerfuffle::CliInterface
{
    Q_OBJECT

public:
    explicit CliPlugin(QObject *parent, const QVariantList &args);
    virtual ~CliPlugin();

    virtual void resetParsing() Q_DECL_OVERRIDE;
    virtual void setupCliParameters(Kerfuffle::CliParameters *params) Q_DECL_OVERRIDE;
    virtual bool readListLine(const QString &line) Q_DECL_OVERRIDE;

private:

    enum ParseState {
        ParseStateTitle = 0,
        ParseStateComment,
        ParseStateHeader,
        ParseStateEntryFileName,
        ParseStateEntryDetails,
        ParseStateLinkTarget
    } m_parseState;

    bool handleUnrar5Line(const QString &line);
    void handleUnrar5Entry();
    bool handleUnrar4Line(const QString &line);
    void handleUnrar4Entry();
    void ignoreLines(int lines, ParseState nextState);

    QStringList m_unrar4Details;
    QHash<QString, QString> m_unrar5Details;

    QString m_unrarVersion;
    bool m_isUnrar5;
    bool m_isPasswordProtected;
    bool m_isSolid;

    int m_remainingIgnoreLines;
    int m_linesComment;
};

#endif // CLIPLUGIN_H
