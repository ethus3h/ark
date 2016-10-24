/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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
 */

#include "cliplugin.h"
#include "ark_debug.h"
#include "kerfuffle/archiveentry.h"
#include "kerfuffle/cliinterface.h"
#include "kerfuffle/kerfuffle_export.h"

#include <KPluginFactory>

#include <QDateTime>
#include <QDir>
#include <QRegularExpression>

using namespace Kerfuffle;

K_PLUGIN_FACTORY_WITH_JSON(CliPluginFactory, "kerfuffle_clizip.json", registerPlugin<CliPlugin>();)

CliPlugin::CliPlugin(QObject *parent, const QVariantList & args)
    : CliInterface(parent, args)
    , m_parseState(ParseStateHeader)
    , m_linesComment(0)
{
    qCDebug(ARK) << "Loaded cli_zip plugin";
}

CliPlugin::~CliPlugin()
{
}

void CliPlugin::resetParsing()
{
    m_parseState = ParseStateHeader;
    m_tempComment.clear();
    m_comment.clear();
}

// #208091: infozip applies special meanings to some characters, so we
//          need to escape them with backslashes.see match.c in
//          infozip's source code
QString CliPlugin::escapeFileName(const QString &fileName) const
{
    const QString escapedCharacters(QStringLiteral("[]*?^-\\!"));

    QString quoted;
    const int len = fileName.length();
    const QLatin1Char backslash('\\');
    quoted.reserve(len * 2);

    for (int i = 0; i < len; ++i) {
        if (escapedCharacters.contains(fileName.at(i))) {
            quoted.append(backslash);
        }

        quoted.append(fileName.at(i));
    }

    return quoted;
}

void CliPlugin::setupCliParameters(CliParameters *params)
{
    qCDebug(ARK) << "Setting up parameters...";

    params->setProperty("addProgram", QStringLiteral("zip"));
    params->setProperty("addSwitch", QStringList({QStringLiteral("-r")}));

    params->setProperty("deleteProgram", QStringLiteral("zip"));
    params->setProperty("deleteSwitch", QStringLiteral("-d"));

    params->setProperty("extractProgram", QStringLiteral("unzip"));
    params->setProperty("extractSwitchNoPreserve", QStringList{QStringLiteral("-j")});

    params->setProperty("listProgram", QStringLiteral("zipinfo"));
    params->setProperty("listSwitch", QStringList{QStringLiteral("-l"),
                                                  QStringLiteral("-T"),
                                                  QStringLiteral("-z")});

    params->setProperty("testProgram", QStringLiteral("unzip"));
    params->setProperty("testSwitch", QStringLiteral("-t"));

    params->setProperty("passwordSwitch", QStringList{QStringLiteral("-P$Password")});

    params->setProperty("compressionLevelSwitch", QStringLiteral("-$CompressionLevel"));
    params->setProperty("compressionMethodSwitch", QHash<QString,QVariant>{{QStringLiteral("application/zip"), QStringLiteral("-Z$CompressionMethod")},
                                                                           {QStringLiteral("application/x-java-archive"), QStringLiteral("-Z$CompressionMethod")}});
    params->setProperty("multiVolumeSwitch", QStringLiteral("-v$VolumeSizek"));
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;

    if (p.isEmpty()) {
        p[CaptureProgress] = false;

        p[FileExistsExpression] = QStringList()
            << QStringLiteral("^replace (.+)\\? \\[y\\]es, \\[n\\]o, \\[A\\]ll, \\[N\\]one, \\[r\\]ename: $");
        p[FileExistsFileName] = QStringList() << p[FileExistsExpression].toString();
        p[FileExistsInput] = QStringList() << QStringLiteral("y")  //overwrite
                                           << QStringLiteral("n")  //skip
                                           << QStringLiteral("A")  //overwrite all
                                           << QStringLiteral("N"); //autoskip
        p[PasswordPromptPattern] = QStringLiteral(" password: ");
        p[WrongPasswordPatterns] = QStringList() << QStringLiteral("incorrect password");
        p[ExtractionFailedPatterns] = QStringList() << QStringLiteral("unsupported compression method");
        p[CorruptArchivePatterns] = QStringList() << QStringLiteral("End-of-central-directory signature not found");
        p[DiskFullPatterns] = QStringList() << QStringLiteral("write error \\(disk full\\?\\)")
                                            << QStringLiteral("No space left on device");
        p[TestPassedPattern] = QStringLiteral("^No errors detected in compressed data of ");
    }
    return p;
}

bool CliPlugin::readListLine(const QString &line)
{
    static const QRegularExpression entryPattern(QStringLiteral(
        "^(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\d{8}).(\\d{6})\\s+(.+)$") );

    // RegExp to identify the line preceding comments.
    const QRegularExpression commentPattern(QStringLiteral("^Archive:  .*$"));
    // RegExp to identify the line following comments.
    const QRegularExpression commentEndPattern(QStringLiteral("^Zip file size: .*$"));

    switch (m_parseState) {
    case ParseStateHeader:
        if (commentPattern.match(line).hasMatch()) {
            m_parseState = ParseStateComment;
        } else if (commentEndPattern.match(line).hasMatch()){
            m_parseState = ParseStateEntry;
        }
        break;
    case ParseStateComment:
        if (commentEndPattern.match(line).hasMatch()) {
            m_parseState = ParseStateEntry;
            if (!m_tempComment.trimmed().isEmpty()) {
                m_comment = m_tempComment.trimmed();
                m_linesComment = m_comment.count(QLatin1Char('\n')) + 1;
                qCDebug(ARK) << "Found a comment with" << m_linesComment << "lines";
            }
        } else {
            m_tempComment.append(line + QLatin1Char('\n'));
        }
    case ParseStateEntry:
        QRegularExpressionMatch rxMatch = entryPattern.match(line);
        if (rxMatch.hasMatch()) {
            Archive::Entry *e = new Archive::Entry(this);
            e->setProperty("permissions", rxMatch.captured(1));

            // #280354: infozip may not show the right attributes for a given directory, so an entry
            //          ending with '/' is actually more reliable than 'd' bein in the attributes.
            e->setProperty("isDirectory", rxMatch.captured(10).endsWith(QLatin1Char('/')));

            e->setProperty("size", rxMatch.captured(4));
            QString status = rxMatch.captured(5);
            if (status[0].isUpper()) {
                e->setProperty("isPasswordProtected", true);
            }
            e->setProperty("compressedSize", rxMatch.captured(6).toInt());
            e->setProperty("method", rxMatch.captured(7));

            QString method = convertCompressionMethod(rxMatch.captured(7));
            if (!m_compressionMethods.contains(method)) {
                m_compressionMethods.append(method);
                emit compressionMethodFound(m_compressionMethods);
            }

            const QDateTime ts(QDate::fromString(rxMatch.captured(8), QStringLiteral("yyyyMMdd")),
                               QTime::fromString(rxMatch.captured(9), QStringLiteral("hhmmss")));
            e->setProperty("timestamp", ts);

            e->setProperty("fullPath", rxMatch.captured(10));
            emit entry(e);
        }
        break;
    }

    return true;
}

bool CliPlugin::moveFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    qCDebug(ARK) << "Moving" << files.count() << "file(s) to destination:" << destination;

    m_oldWorkingDir = QDir::currentPath();
    m_tempExtractDir = new QTemporaryDir();
    m_tempAddDir = new QTemporaryDir();
    QDir::setCurrent(m_tempExtractDir->path());
    m_passedFiles = files;
    m_passedDestination = destination;
    m_passedOptions = options;

    m_subOperation = Extract;
    connect(this, &CliPlugin::finished, this, &CliPlugin::continueMoving);

    return extractFiles(files, QDir::currentPath(), ExtractionOptions());
}

int CliPlugin::moveRequiredSignals() const {
    return 4;
}

void CliPlugin::continueMoving(bool result)
{
    if (!result) {
        finishMoving(false);
        return;
    }

    switch (m_subOperation) {
    case Extract:
        m_subOperation = Delete;
        if (!deleteFiles(m_passedFiles)) {
            finishMoving(false);
        }
        break;
    case Delete:
        m_subOperation = Add;
        if (!setMovingAddedFiles() || !addFiles(m_tempAddedFiles, m_passedDestination, m_passedOptions)) {
            finishMoving(false);
        }
        break;
    case Add:
        finishMoving(true);
        break;
    default:
        Q_ASSERT(false);
    }
}

bool CliPlugin::setMovingAddedFiles()
{
    m_passedFiles = entriesWithoutChildren(m_passedFiles);
    // If there are more files being moved than 1, we have destination as a destination folder,
    // otherwise it's new entry full path.
    if (m_passedFiles.count() > 1) {
        return setAddedFiles();
    }

    QDir::setCurrent(m_tempAddDir->path());
    const Archive::Entry *file = m_passedFiles.at(0);
    const QString oldPath = m_tempExtractDir->path() + QLatin1Char('/') + file->fullPath(NoTrailingSlash);
    const QString newPath = m_tempAddDir->path() + QLatin1Char('/') + m_passedDestination->name();
    if (!QFile::rename(oldPath, newPath)) {
        return false;
    }
    m_tempAddedFiles << new Archive::Entry(Q_NULLPTR, m_passedDestination->name());

    // We have to exclude file name from destination path in order to pass it to addFiles method.
    const QString destinationPath = m_passedDestination->fullPath();
    if (!destinationPath.endsWith(QLatin1Char('/')) || destinationPath.count(QLatin1Char('/')) > 1) {
        int destinationLength = destinationPath.count();
        bool iteratedChar = false;
        do {
            destinationLength--;
            if (destinationPath.at(destinationLength) != QLatin1Char('/')) {
                iteratedChar = true;
            }
        } while (destinationLength > 0 && !(iteratedChar && destinationPath.at(destinationLength) == QLatin1Char('/')));
        m_passedDestination->setProperty("fullPath", destinationPath.left(destinationLength + 1));
   } else {
        // ...unless the destination path is already a single folder, e.g. "dir/".
        // In this case we're going to add to the root, so we just need to set a null destination.
        m_passedDestination = Q_NULLPTR;
    }

    return true;
}

void CliPlugin::finishMoving(bool result)
{
    disconnect(this, &CliPlugin::finished, this, &CliPlugin::continueMoving);
    emit progress(1.0);
    emit finished(result);
    cleanUp();
}

QString CliPlugin::convertCompressionMethod(const QString &method)
{
    if (method == QLatin1String("stor")) {
        return QStringLiteral("Store");
    } else if (method.startsWith(QLatin1String("def"))) {
        return QStringLiteral("Deflate");
    } else if (method == QLatin1String("bzp2")) {
        return QStringLiteral("BZip2");
    }
    return method;
}

#include "cliplugin.moc"
