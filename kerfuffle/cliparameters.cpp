/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2016 Ragnar Thomsen <rthomsen6@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cliparameters.h"
#include "ark_debug.h"
#include "archiveformat.h"
#include "pluginmanager.h"

namespace Kerfuffle
{

CliParameters::CliParameters(QObject *parent, const KPluginMetaData &metaData, const QMimeType &archiveType)
        : QObject(parent)
        , m_mimeType(archiveType)
        , m_metaData(metaData)
{
}

bool CliParameters::isInitialized() const
{
    // If listProgram is empty we assume uninitialized state.
    return !m_listProgram.isEmpty();
}

QStringList CliParameters::addArgs(const QString &archive, const QStringList &files, const QString &password, bool headerEncryption, int compressionLevel, const QString &compressionMethod, uint volumeSize)
{
    QStringList args;
    foreach (const QString &s, m_addSwitch) {
        args << s;
    }
    if (!password.isEmpty()) {
        args << substitutePasswordSwitch(password, headerEncryption);
    }
    if (compressionLevel > -1) {
        args << substituteCompressionLevelSwitch(compressionLevel);
    }
    if (!compressionMethod.isEmpty()) {
        args << substituteCompressionMethodSwitch(compressionMethod);
    }
    if (volumeSize > 0) {
        args << substituteMultiVolumeSwitch(volumeSize);
    }
    args << archive;
    args << files;

    args.removeAll(QString());
    return args;
}

QStringList CliParameters::commentArgs(const QString &archive, const QString &commentfile)
{
    QStringList args;
    foreach (const QString &s, substituteCommentSwitch(commentfile)) {
        args << s;
    }
    args << archive;

    args.removeAll(QString());
    return args;
}

QStringList CliParameters::deleteArgs(const QString &archive, const QVector<Archive::Entry*> &files, const QString &password)
{
    QStringList args;
    args << m_deleteSwitch;
    if (!password.isEmpty()) {
        args << substitutePasswordSwitch(password);
    }
    args << archive;
    foreach (const Archive::Entry *e, files) {
        args << e->fullPath(NoTrailingSlash);
    }

    args.removeAll(QString());
    return args;
}

QStringList CliParameters::extractArgs(const QString &archive, const QStringList &files, bool preservePaths, const QString &password)
{
    QStringList args;

    if (preservePaths && !m_extractSwitch.isEmpty()) {
        args << m_extractSwitch;
    } else if (!preservePaths && !m_extractSwitchNoPreserve.isEmpty()) {
        args << m_extractSwitchNoPreserve;
    }

    if (!password.isEmpty()) {
        args << substitutePasswordSwitch(password);
    }
    args << archive;
    args << files;

    args.removeAll(QString());
    return args;
}

QStringList CliParameters::listArgs(const QString &archive, const QString &password)
{
    QStringList args;
    foreach (const QString &s, m_listSwitch) {
        args << s;
    }
    if (!password.isEmpty()) {
        args << substitutePasswordSwitch(password);
    }
    args << archive;

    args.removeAll(QString());
    return args;
}

QStringList CliParameters::moveArgs(const QString &archive, const QVector<Archive::Entry*> &entries, Archive::Entry *destination, const QString &password)
{
    QStringList args;
    args << m_moveSwitch;
    if (!password.isEmpty()) {
        args << substitutePasswordSwitch(password);
    }
    args << archive;
    if (entries.count() > 1) {
        foreach (const Archive::Entry *file, entries) {
            args << file->fullPath(NoTrailingSlash) << destination->fullPath() + file->name();
        }
    } else {
        args << entries.at(0)->fullPath(NoTrailingSlash) << destination->fullPath(NoTrailingSlash);
    }

    args.removeAll(QString());
    return args;
}

QStringList CliParameters::testArgs(const QString &archive, const QString &password)
{
    QStringList args;
    foreach (const QString &s, m_testSwitch) {
        args << s;
    }
    if (!password.isEmpty()) {
        args << substitutePasswordSwitch(password);
    }
    args << archive;

    args.removeAll(QString());
    return args;
}

QStringList CliParameters::substituteCommentSwitch(const QString &commentfile)
{
    Q_ASSERT(!commentfile.isEmpty());

    Q_ASSERT(ArchiveFormat::fromMetadata(m_mimeType, m_metaData).supportsWriteComment());

    QStringList commentSwitches = m_commentSwitch;
    Q_ASSERT(!commentSwitches.isEmpty());

    QMutableListIterator<QString> i(commentSwitches);
    while (i.hasNext()) {
        i.next();
        i.value().replace(QLatin1String("$CommentFile"), commentfile);
    }

    return commentSwitches;
}

QStringList CliParameters::substitutePasswordSwitch(const QString &password, bool headerEnc)
{
    if (password.isEmpty()) {
        return QStringList();
    }

    Archive::EncryptionType encryptionType = ArchiveFormat::fromMetadata(m_mimeType, m_metaData).encryptionType();
    Q_ASSERT(encryptionType != Archive::EncryptionType::Unencrypted);

    QStringList passwordSwitch;
    if (headerEnc) {
        passwordSwitch = m_passwordSwitchHeaderEnc;
    } else {
        passwordSwitch = m_passwordSwitch;
    }
    Q_ASSERT(!passwordSwitch.isEmpty());

    QMutableListIterator<QString> i(passwordSwitch);
    while (i.hasNext()) {
        i.next();
        i.value().replace(QLatin1String("$Password"), password);
    }

    return passwordSwitch;
}

QString CliParameters::substituteCompressionLevelSwitch(int level)
{
    if (level < 0 || level > 9) {
        return QString();
    }

    Q_ASSERT(ArchiveFormat::fromMetadata(m_mimeType, m_metaData).maxCompressionLevel() != -1);

    QString compLevelSwitch = m_compressionLevelSwitch;
    Q_ASSERT(!compLevelSwitch.isEmpty());

    compLevelSwitch.replace(QLatin1String("$CompressionLevel"), QString::number(level));

    return compLevelSwitch;
}

QString CliParameters::substituteCompressionMethodSwitch(const QString &method)
{   
    if (method.isEmpty()) {
        return QString();
    }

    Q_ASSERT(!ArchiveFormat::fromMetadata(m_mimeType, m_metaData).compressionMethods().isEmpty());

    QString compMethodSwitch = m_compressionMethodSwitch[m_mimeType.name()].toString();
    Q_ASSERT(!compMethodSwitch.isEmpty());

    QString cliMethod = ArchiveFormat::fromMetadata(m_mimeType, m_metaData).compressionMethods().value(method).toString();

    compMethodSwitch.replace(QLatin1String("$CompressionMethod"), cliMethod);

    return compMethodSwitch;
}

QString CliParameters::substituteMultiVolumeSwitch(uint volumeSize)
{
    // The maximum value we allow in the QDoubleSpinBox is 1000MB. Converted to
    // KB this is 1024000.
    if (volumeSize <= 0 || volumeSize > 1024000) {
        return QString();
    }

    Q_ASSERT(ArchiveFormat::fromMetadata(m_mimeType, m_metaData).supportsMultiVolume());

    QString multiVolumeSwitch = m_multiVolumeSwitch;
    Q_ASSERT(!multiVolumeSwitch.isEmpty());

    multiVolumeSwitch.replace(QLatin1String("$VolumeSize"), QString::number(volumeSize));

    return multiVolumeSwitch;
}

bool CliParameters::isPasswordPrompt(const QString &line)
{
    foreach(const QString &rx, m_passwordPromptPatterns) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliParameters::isWrongPasswordMsg(const QString &line)
{
    foreach(const QString &rx, m_wrongPasswordPatterns) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliParameters::isTestPassedMsg(const QString &line)
{
    foreach(const QString &rx, m_testPassedPatterns) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliParameters::isfileExistsMsg(const QString &line)
{
    foreach(const QString &rx, m_fileExistsPatterns) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliParameters::isFileExistsFileName(const QString &line)
{
    foreach(const QString &rx, m_fileExistsFileName) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliParameters::isExtractionFailedMsg(const QString &line)
{
    foreach(const QString &rx, m_extractionFailedPatterns) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliParameters::isCorruptArchiveMsg(const QString &line)
{
    foreach(const QString &rx, m_corruptArchivePatterns) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliParameters::isDiskFullMsg(const QString &line)
{
    foreach(const QString &rx, m_diskFullPatterns) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

}
