/*
 * Copyright (c) 2016 Ragnar Thomsen <rthomsen6@gmail.com>
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

#include "adddialog.h"
#include "archiveformat.h"
#include "pluginmanager.h"

#include <KCollapsibleGroupBox>

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QMimeDatabase>
#include <QTest>

using namespace Kerfuffle;

class AddDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testBasicWidgets_data();
    void testBasicWidgets();

private:
    PluginManager m_pluginManager;
};

void AddDialogTest::testBasicWidgets_data()
{
    QTest::addColumn<QString>("mimeType");
    QTest::addColumn<bool>("supportsCompLevel");
    QTest::addColumn<int>("compLevel");

    QTest::newRow("tar") << QStringLiteral("application/x-tar") << false << 3;
    QTest::newRow("targzip") << QStringLiteral("application/x-compressed-tar") << true << 3;
    QTest::newRow("tarbzip") << QStringLiteral("application/x-bzip-compressed-tar") << true << 3;
    QTest::newRow("tarZ") << QStringLiteral("application/x-tarz") << false << 3;
    QTest::newRow("tarxz") << QStringLiteral("application/x-xz-compressed-tar") << true << 3;
    QTest::newRow("tarlzma") << QStringLiteral("application/x-lzma-compressed-tar") << true << 6;
    QTest::newRow("tarlzop") << QStringLiteral("application/x-tzo") << true << 4;
    QTest::newRow("tarlzip") << QStringLiteral("application/x-lzip-compressed-tar") << true << 7;

    const auto writeMimeTypes = m_pluginManager.supportedWriteMimeTypes();

    if (writeMimeTypes.contains(QStringLiteral("application/zip"))) {
        QTest::newRow("zip") << QStringLiteral("application/zip") << true << 3;
    } else {
        qDebug() << "zip format not available, skipping test.";
    }

    if (writeMimeTypes.contains(QStringLiteral("application/x-7z-compressed"))) {
        QTest::newRow("7z") << QStringLiteral("application/x-7z-compressed") << true << 3;
    } else {
        qDebug() << "7z format not available, skipping test.";
    }

    if (writeMimeTypes.contains(QStringLiteral("application/x-rar"))) {
        QTest::newRow("rar") << QStringLiteral("application/x-rar") << true << 3;
    } else {
        qDebug() << "rar format not available, skipping test.";
    }

    if (writeMimeTypes.contains(QStringLiteral("application/x-lrzip-compressed-tar"))) {
        QTest::newRow("tarlrzip") << QStringLiteral("application/x-lrzip-compressed-tar") << true << 3;
    } else {
        qDebug() << "tar.lrzip format not available, skipping test.";
    }
}

void AddDialogTest::testBasicWidgets()
{
    QFETCH(QString, mimeType);
    const QMimeType mime = QMimeDatabase().mimeTypeForName(mimeType);
    QFETCH(bool, supportsCompLevel);
    QFETCH(int, compLevel);

    AddDialog *dialog = new AddDialog(Q_NULLPTR, QString(), QUrl(), mime);

    dialog->slotOpenOptions();

    auto collapsibleCompression = dialog->optionsDialog->findChild<KCollapsibleGroupBox*>(QStringLiteral("collapsibleCompression"));
    QVERIFY(collapsibleCompression);

    if (supportsCompLevel) {
        // Test that collapsiblegroupbox is enabled for mimetypes that support compression levels.
        QVERIFY(collapsibleCompression->isEnabled());

        auto compLevelSlider = dialog->optionsDialog->findChild<QSlider*>(QStringLiteral("compLevelSlider"));
        QVERIFY(compLevelSlider);

        const KPluginMetaData metadata = PluginManager().preferredPluginFor(mime)->metaData();
        const ArchiveFormat archiveFormat = ArchiveFormat::fromMetadata(mime, metadata);
        QVERIFY(archiveFormat.isValid());

        // Test that min/max of slider are correct.
        QCOMPARE(compLevelSlider->minimum(), archiveFormat.minCompressionLevel());
        QCOMPARE(compLevelSlider->maximum(), archiveFormat.maxCompressionLevel());

        // Set the compression level slider.
        compLevelSlider->setValue(compLevel);
    } else {
        // Test that collapsiblegroupbox is disabled for mimetypes that don't support compression levels.
        QVERIFY(!collapsibleCompression->isEnabled());
    }

    dialog->optionsDialog->accept();

    if (supportsCompLevel) {
        // Test that the value set by slider is exported from AddDialog.
        QCOMPARE(dialog->compressionOptions()[QStringLiteral("CompressionLevel")].toInt(), compLevel);
    }
}

QTEST_MAIN(AddDialogTest)

#include "adddialogtest.moc"
