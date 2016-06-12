/*
 * ark: A program for modifying archives via a GUI.
 *
 * Copyright (C) 2004-2008 Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include "arkviewer.h"
#include "ark_debug.h"

#include <KLocalizedString>
#include <KMimeTypeTrader>
#include <KIconLoader>
#include <KMessageBox>
#include <KRun>
#include <KSharedConfig>
#include <KWindowConfig>

#include <QProgressDialog>
#include <QDebug>
#include <QFile>
#include <QMimeDatabase>

ArkViewer::ArkViewer()
        : QDialog()
{
    qCDebug(ARK) << "ArkViewer opened";

    setAttribute(Qt::WA_DeleteOnClose);

    setupUi(this);

    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(this, &ArkViewer::finished, this, &ArkViewer::dialogClosed);
}

ArkViewer::~ArkViewer()
{
}

void ArkViewer::dialogClosed()
{
    // Save viewer dialog window size
    KConfigGroup group(KSharedConfig::openConfig(), "Viewer");
    KWindowConfig::saveWindowSize(windowHandle(), group, KConfigBase::Persistent);

    if (m_part) {
        QProgressDialog progressDialog(this);
        progressDialog.setWindowTitle(i18n("Closing preview"));
        progressDialog.setLabelText(i18n("Please wait while the preview is being closed..."));

        progressDialog.setMinimumDuration(500);
        progressDialog.setModal(true);
        progressDialog.setCancelButton(0);
        progressDialog.setRange(0, 0);

        // #261785: this preview dialog is not modal, so we need to delete
        //          the previewed file ourselves when the dialog is closed;
        //          we used to remove it at the end of ArkViewer::view() when
        //          QDialog::exec() was called instead of QDialog::show().
        const QString previewedFilePath(m_part.data()->url().toDisplayString(QUrl::PreferLocalFile));

        m_part.data()->closeUrl();

        if (!previewedFilePath.isEmpty()) {
            QFile::remove(previewedFilePath);
        }
    }
}

void ArkViewer::view(const QString& fileName)
{
    QMimeDatabase db;
    QMimeType mimeType = db.mimeTypeForFile(fileName);
    qCDebug(ARK) << "viewing" << fileName << "with mime type:" << mimeType.name();
    KService::Ptr viewer = ArkViewer::getViewer(mimeType.name());

    const bool needsExternalViewer = (viewer &&
                                      !viewer->hasServiceType(QStringLiteral("KParts/ReadOnlyPart")));
    if (needsExternalViewer) {
        // We have already resolved the MIME type and the service above.
        // So there is no point in using KRun::runUrl() which would need
        // to do the same again.

        qCDebug(ARK) << "Using external viewer";

        const QList<QUrl> fileUrlList = {QUrl::fromLocalFile(fileName)};
        // The last argument (tempFiles) set to true means that the temporary
        // file will be removed when the viewer application exits.
        KRun::runService(*viewer, fileUrlList, Q_NULLPTR, true);
        return;
    }

    qCDebug(ARK) << "Using internal viewer";
    bool viewInInternalViewer = true;
    if (!viewer) {
        // No internal viewer available for the file.  Ask the user if it
        // should be previewed as text/plain.

        qCDebug(ARK) << "Internal viewer not available";

        int response;
        if (!mimeType.isDefault()) {
            // File has a defined MIME type, and not the default
            // application/octet-stream.  So it could be viewable as
            // plain text, ask the user.
            response = KMessageBox::warningContinueCancel(Q_NULLPTR,
                xi18n("The internal viewer cannot preview this type of file<nl/>(%1).<nl/><nl/>Do you want to try to view it as plain text?", mimeType.name()),
                i18nc("@title:window", "Cannot Preview File"),
                KGuiItem(i18nc("@action:button", "Preview as Text"), QIcon::fromTheme(QStringLiteral("text-plain"))),
                KStandardGuiItem::cancel(),
                QStringLiteral("PreviewAsText_%1").arg(mimeType.name()));
        }
        else {
            // No defined MIME type, or the default application/octet-stream.
            // There is still a possibility that it could be viewable as plain
            // text, so ask the user.  Not the same as the message/question
            // above, because the wording and default are different.
            response = KMessageBox::warningContinueCancel(Q_NULLPTR,
                xi18n("The internal viewer cannot preview this unknown type of file.<nl/><nl/>Do you want to try to view it as plain text?"),
                i18nc("@title:window", "Cannot Preview File"),
                KGuiItem(i18nc("@action:button", "Preview as Text"), QIcon::fromTheme(QStringLiteral("text-plain"))),
                KStandardGuiItem::cancel(),
                QString(),
                KMessageBox::Dangerous);
        }

        if (response == KMessageBox::Cancel) {
            viewInInternalViewer = false;
        }
        else {						// set for viewer later
            mimeType = db.mimeTypeForName(QStringLiteral("text/plain"));
        }
    }

    if (viewInInternalViewer) {
        qCDebug(ARK) << "Opening internal viewer";
        ArkViewer *internalViewer = new ArkViewer();
        internalViewer->show();
        if (internalViewer->viewInInternalViewer(fileName, mimeType)) {
            // The internal viewer is showing the file, and will
            // remove the temporary file in dialogClosed().  So there
            // is no more to do here.
            return;
        }
        else {
            KMessageBox::sorry(Q_NULLPTR, i18n("The internal viewer cannot preview this file."));
            delete internalViewer;
        }
    }

    // Only get here if there is no internal viewer available or could be
    // used for the file, and no external viewer was opened.  Nothing can be
    // done with the temporary file, so remove it now.
    qCDebug(ARK) << "Removing temporary file:" << fileName;
    QFile::remove(fileName);
}

bool ArkViewer::viewInInternalViewer(const QString& fileName, const QMimeType &mimeType)
{
    setWindowFilePath(fileName);

    // Load viewer dialog window size from config file
    KConfigGroup group(KSharedConfig::openConfig(), "Viewer");
    KWindowConfig::restoreWindowSize(windowHandle(), group);


    // Set icon and comment for the mimetype.
    m_iconLabel->setPixmap(QIcon::fromTheme(mimeType.iconName()).pixmap(IconSize(KIconLoader::Desktop), IconSize(KIconLoader::Desktop)));
    m_commentLabel->setText(mimeType.comment());

    // Create the ReadOnlyPart instance.
    m_part = KMimeTypeTrader::self()->createPartInstanceFromQuery<KParts::ReadOnlyPart>(mimeType.name(), this, this);

    // Drop the KHTMLPart, if necessary.
    const KService::Ptr service = KMimeTypeTrader::self()->preferredService(mimeType.name(), QStringLiteral("KParts/ReadOnlyPart"));
    if (service.constData()->desktopEntryName() == QLatin1String("khtml")) {
        KService::List offers = KMimeTypeTrader::self()->query(mimeType.name(), QStringLiteral("KParts/ReadOnlyPart"));
        offers.removeFirst();
        qCDebug(ARK) << "Removed KHTMLPart from the offers for mimetype" << mimeType.name()
                     << ". Using" << offers.first().constData()->desktopEntryName() << "instead.";
        m_part = offers.first().constData()->createInstance<KParts::ReadOnlyPart>(this, this);
    }

    if (!m_part.data()) {
        return false;
    }

    // Insert the KPart into its placeholder.
    layout()->replaceWidget(m_partPlaceholder, m_part.data()->widget());

    m_part.data()->openUrl(QUrl::fromLocalFile(fileName));

    return true;
}

KService::Ptr ArkViewer::getViewer(const QString &mimeType)
{
    // No point in even trying to find anything for application/octet-stream
    if (mimeType == QStringLiteral("application/octet-stream")) {
        return KService::Ptr();
    }

    // Try to get a read-only kpart for the internal viewer
    KService::List offers = KMimeTypeTrader::self()->query(mimeType, QStringLiteral("KParts/ReadOnlyPart"));

    // If we can't find a kpart, try to get an external application
    if (offers.isEmpty()) {
        offers = KMimeTypeTrader::self()->query(mimeType, QStringLiteral("Application"));
    }

    if (!offers.isEmpty()) {
        return offers.first();
    } else {
        return KService::Ptr();
    }
}
