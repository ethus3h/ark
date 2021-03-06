/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2011 Luke Shumaker <lukeshu@sbcglobal.net>
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
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

#include "cliplugin.h"
#include "ark_debug.h"
#include "queries.h"

#include <QJsonArray>
#include <QJsonParseError>

#include <KLocalizedString>
#include <KPluginFactory>
#include <KPtyProcess>

using namespace Kerfuffle;

K_PLUGIN_FACTORY_WITH_JSON(CliPluginFactory, "kerfuffle_cliunarchiver.json", registerPlugin<CliPlugin>();)

CliPlugin::CliPlugin(QObject *parent, const QVariantList &args)
        : CliInterface(parent, args)
{
    qCDebug(ARK) << "Loaded cli_unarchiver plugin";
    setupCliProperties();
}

CliPlugin::~CliPlugin()
{
}

bool CliPlugin::list()
{
    resetParsing();
    m_operationMode = List;

    return runProcess(m_cliProps->property("listProgram").toString(), m_cliProps->listArgs(filename(), password()));
}

bool CliPlugin::extractFiles(const QVector<Archive::Entry*> &files, const QString &destinationDirectory, const ExtractionOptions &options)
{
    ExtractionOptions newOptions = options;

    // unar has the following limitations:
    // 1. creates an empty file upon entering a wrong password.
    // 2. detects that the stdout has been redirected and blocks the stdin.
    //    This prevents Ark from executing unar's overwrite queries.
    // To prevent both, we always extract to a temporary directory
    // and then we move the files to the intended destination.

    qCDebug(ARK) << "Enabling extraction to temporary directory.";
    newOptions.setAlwaysUseTempDir(true);

    return CliInterface::extractFiles(files, destinationDirectory, newOptions);
}

void CliPlugin::resetParsing()
{
    m_jsonOutput.clear();
    m_numberOfVolumes = 0;
}

void CliPlugin::setupCliProperties()
{
    m_cliProps->setProperty("captureProgress", false);

    m_cliProps->setProperty("extractProgram", QStringLiteral("unar"));
    m_cliProps->setProperty("extractSwitch", QStringList{QStringLiteral("-D")});
    m_cliProps->setProperty("extractSwitchNoPreserve", QStringList{QStringLiteral("-D")});

    m_cliProps->setProperty("listProgram", QStringLiteral("lsar"));
    m_cliProps->setProperty("listSwitch", QStringList{QStringLiteral("-json")});

    m_cliProps->setProperty("passwordSwitch", QStringList{QStringLiteral("-password"),
                                                      QStringLiteral("$Password")});

    m_cliProps->setProperty("passwordPromptPatterns", QStringList{QStringLiteral("This archive requires a password to unpack. Use the -p option to provide one.")});
}

bool CliPlugin::readListLine(const QString &line)
{
    const QRegularExpression rx(QStringLiteral("Failed! \\((.+)\\)$"));

    if (rx.match(line).hasMatch()) {
        emit error(i18n("Listing the archive failed."));
        return false;
    }

    return true;
}

bool CliPlugin::readExtractLine(const QString &line)
{
    const QRegularExpression rx(QStringLiteral("Failed! \\((.+)\\)$"));

    if (rx.match(line).hasMatch()) {
        emit error(i18n("Extraction failed."));
        return false;
    }

    return true;
}

void CliPlugin::setJsonOutput(const QString &jsonOutput)
{
    m_jsonOutput = jsonOutput;
    readJsonOutput();
}

void CliPlugin::readStdout(bool handleAll)
{
    if (!handleAll) {
        CliInterface::readStdout(false);
        return;
    }

    // We are ready to read the json output.
    readJsonOutput();
}

bool CliPlugin::handleLine(const QString& line)
{
    // Collect the json output line by line.
    if (m_operationMode == List) {
        // #372210: lsar can generate huge JSONs for big archives.
        // We can at least catch a bad_alloc here in order to not crash.
        try {
            m_jsonOutput += line + QLatin1Char('\n');
        } catch (const std::bad_alloc&) {
            m_jsonOutput.clear();
            emit error(i18n("Not enough memory for loading the archive."));
            return false;
        }
    }

    if (m_operationMode == List) {
        // This can only be an header-encrypted archive.
        if (m_cliProps->isPasswordPrompt(line)) {
            qCDebug(ARK) << "Detected header-encrypted RAR archive";

            Kerfuffle::PasswordNeededQuery query(filename());
            emit userQuery(&query);
            query.waitForResponse();

            if (query.responseCancelled()) {
                emit cancelled();
                // Process is gone, so we emit finished() manually and we return true.
                emit finished(false);
                return true;
            }

            setPassword(query.password());
            CliPlugin::list();
        }
    }

    return true;
}

void CliPlugin::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qCDebug(ARK) << "Process finished, exitcode:" << exitCode << "exitstatus:" << exitStatus;

    if (m_process) {
        //handle all the remaining data in the process
        readStdout(true);

        delete m_process;
        m_process = Q_NULLPTR;
    }

    // #193908 - #222392
    // Don't emit finished() if the job was killed quietly.
    if (m_abortingOperation) {
        return;
    }

    if (!password().isEmpty()) {

        // lsar -json exits with error code 1 if the archive is header-encrypted and the password is wrong.
        if (exitCode == 1) {
            qCWarning(ARK) << "Wrong password, list() aborted";
            emit error(i18n("Wrong password."));
            emit finished(false);
            setPassword(QString());
            return;
        }
    }

    // lsar -json exits with error code 2 if the archive is header-encrypted and no password is given as argument.
    // At this point we are asking a password to the user and we are going to list() again after we get one.
    // This means that we cannot emit finished here.
    if (exitCode == 2) {
        return;
    }

    emit finished(true);
}

void CliPlugin::readJsonOutput()
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(m_jsonOutput.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        qCDebug(ARK) << "Could not parse json output:" << error.errorString();
        return;
    }

    const QJsonObject json = jsonDoc.object();

    const QJsonObject properties = json.value(QStringLiteral("lsarProperties")).toObject();
    const QJsonArray volumes = properties.value(QStringLiteral("XADVolumes")).toArray();
    if (volumes.count() > 1) {
        qCDebug(ARK) << "Detected multivolume archive";
        m_numberOfVolumes = volumes.count();
        setMultiVolume(true);
    }

    QString formatName = json.value(QStringLiteral("lsarFormatName")).toString();
    if (formatName == QLatin1String("RAR")) {
        emit compressionMethodFound(QStringLiteral("RAR4"));
    } else if (formatName == QLatin1String("RAR 5")) {
        emit compressionMethodFound(QStringLiteral("RAR5"));
    }
    const QJsonArray entries = json.value(QStringLiteral("lsarContents")).toArray();

    foreach (const QJsonValue& value, entries) {
        const QJsonObject currentEntryJson = value.toObject();

        Archive::Entry *currentEntry = new Archive::Entry(this);

        QString filename = currentEntryJson.value(QStringLiteral("XADFileName")).toString();

        currentEntry->setProperty("isDirectory", !currentEntryJson.value(QStringLiteral("XADIsDirectory")).isUndefined());
        if (currentEntry->isDir()) {
            filename += QLatin1Char('/');
        }

        currentEntry->setProperty("fullPath", filename);

        // FIXME: archives created from OSX (i.e. with the __MACOSX folder) list each entry twice, the 2nd time with size 0
        currentEntry->setProperty("size", currentEntryJson.value(QStringLiteral("XADFileSize")));
        currentEntry->setProperty("compressedSize", currentEntryJson.value(QStringLiteral("XADCompressedSize")));
        currentEntry->setProperty("timestamp", currentEntryJson.value(QStringLiteral("XADLastModificationDate")).toVariant());
        currentEntry->setProperty("size", currentEntryJson.value(QStringLiteral("XADFileSize")));
        currentEntry->setProperty("isPasswordProtected", (currentEntryJson.value(QStringLiteral("XADIsEncrypted")).toInt() == 1));
        // TODO: missing fields

        emit entry(currentEntry);
    }
}

#include "cliplugin.moc"
