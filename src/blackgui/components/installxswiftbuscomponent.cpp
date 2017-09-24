/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "installxswiftbuscomponent.h"
#include "ui_installxswiftbuscomponent.h"
#include "blackgui/guiapplication.h"
#include "blackgui/overlaymessagesframe.h"
#include "blackmisc/simulation/xplane/xplaneutil.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/directoryutils.h"
#include "blackmisc/fileutils.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTimer>
#include <QDesktopServices>

using namespace BlackMisc;
using namespace BlackMisc::Network;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Simulation::Settings;
using namespace BlackMisc::Simulation::XPlane;

namespace BlackGui
{
    namespace Components
    {
        CInstallXSwiftBusComponent::CInstallXSwiftBusComponent(QWidget *parent) :
            COverlayMessagesFrame(parent),
            CLoadIndicatorEnabled(this),
            ui(new Ui::CInstallXSwiftBusComponent)
        {
            ui->setupUi(this);

            ui->le_XSwiftBusPluginDir->setText(this->getXPlanePluginDirectory());
            ui->le_DownloadDir->setText(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
            ui->cb_DownloadFile->setEnabled(false);

            connect(ui->pb_DialogInstallDir, &QPushButton::pressed, this, &CInstallXSwiftBusComponent::selectPluginDirectory);
            connect(ui->pb_DialogDownloadDir, &QPushButton::pressed, this, &CInstallXSwiftBusComponent::selectDownloadDirectory);
            connect(ui->pb_Download, &QPushButton::pressed, this, &CInstallXSwiftBusComponent::triggerDownloadingOfXSwiftBusFile);
            connect(ui->pb_OpenDownloadDir, &QPushButton::pressed, this, &CInstallXSwiftBusComponent::openDownloadDir);
            connect(ui->pb_OpenInstallDir, &QPushButton::pressed, this, &CInstallXSwiftBusComponent::openInstallDir);

            // lod metadata
            this->triggerLoadingXSwiftBusFileInfo();
        }

        CInstallXSwiftBusComponent::~CInstallXSwiftBusComponent()
        { }

        void CInstallXSwiftBusComponent::selectPluginDirectory()
        {
            QString xPlanePluginDir = ui->le_XSwiftBusPluginDir->text().trimmed();
            xPlanePluginDir = QFileDialog::getExistingDirectory(parentWidget(),
                              tr("Choose your X-Plane plugin directory"), xPlanePluginDir, m_fileDialogOptions);

            if (xPlanePluginDir.isEmpty()) { return; } // canceled
            if (!QDir(xPlanePluginDir).exists())
            {
                const CStatusMessage msg = CStatusMessage(this, CLogCategory::validation()).warning("'%1' is not a valid X-Plane plugin directory") << xPlanePluginDir;
                this->showOverlayMessage(msg, CInstallXSwiftBusComponent::OverlayMsgTimeoutMs);
                return;
            }
            ui->le_XSwiftBusPluginDir->setText(xPlanePluginDir);
        }

        void CInstallXSwiftBusComponent::selectDownloadDirectory()
        {
            QString downloadDir = ui->le_DownloadDir->text().trimmed();
            downloadDir = QFileDialog::getExistingDirectory(parentWidget(),
                          tr("Choose your X-Plane plugin directory"), downloadDir, m_fileDialogOptions);

            if (downloadDir.isEmpty()) { return; } // canceled
            if (!QDir(downloadDir).exists())
            {
                const CStatusMessage msg = CStatusMessage(this, CLogCategory::validation()).warning("'%1' is not a valid download directory") << downloadDir;
                this->showOverlayMessage(msg, CInstallXSwiftBusComponent::OverlayMsgTimeoutMs);
                return;
            }
            ui->le_DownloadDir->setText(downloadDir);
        }

        void CInstallXSwiftBusComponent::installXSwiftBus()
        {
            const CRemoteFile rf = this->getRemoteFileSelected();
            const QString sourceFileName = CFileUtils::appendFilePaths(ui->le_DownloadDir->text(), rf.getName());
            const QFile sourceFile(sourceFileName);
            if (!sourceFile.exists())
            {
                const CStatusMessage msg = CStatusMessage(this, CLogCategory::validation()).error("Cannot read downloaded file '%1'") << sourceFileName;
                this->showOverlayMessage(msg, CInstallXSwiftBusComponent::OverlayMsgTimeoutMs);
                return;
            }
            const QString destFileName = CFileUtils::appendFilePaths(ui->le_XSwiftBusPluginDir->text(), rf.getName());
            bool moved = QDir().rename(sourceFileName, destFileName);
            if (moved)
            {
                const CStatusMessage msg = CStatusMessage(this, CLogCategory::validation()).info("Installed '%1'") << destFileName;
                this->showOverlayMessage(msg, CInstallXSwiftBusComponent::OverlayMsgTimeoutMs);
            }
            else
            {
                const CStatusMessage msg = CStatusMessage(this, CLogCategory::validation()).error("Cannot move '%1' to '%2'") << sourceFileName << destFileName;
                this->showOverlayMessage(msg, CInstallXSwiftBusComponent::OverlayMsgTimeoutMs);
            }
        }

        void CInstallXSwiftBusComponent::triggerLoadingXSwiftBusFileInfo()
        {
            if (!sGui || !sGui->hasWebDataServices() || sGui->isShuttingDown()) { return; }
            const CUrl url = sGui->getGlobalSetup().getAlphaXSwiftBusFilesServiceUrl();
            if (url.isEmpty()) { return; }
            sGui->getFromNetwork(url, { this, &CInstallXSwiftBusComponent::loadedXSwiftBusFileInfo });
            CLogMessage(this).info("Trigger loading XSwiftBus file info from '%1'") << url.getHost();
        }

        void CInstallXSwiftBusComponent::loadedXSwiftBusFileInfo(QNetworkReply *reply)
        {
            QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> nwReply(reply); // clean up reply afterwards
            if (sGui && sGui->isShuttingDown()) { return; }
            m_remoteFiles.clear();

            if (nwReply->error() == QNetworkReply::NoError)
            {
                const QString json = reply->readAll();
                if (!json.isEmpty())
                {
                    const CRemoteFileList remoteFiles = CRemoteFileList::fromDatabaseJson(json);
                    m_remoteFiles = remoteFiles;
                    CLogMessage(this).info("Loaded %1 XSwiftBus file info entries from '%2'") << remoteFiles.size() << nwReply->url().toString();
                }
            }
            else if (sGui && !sGui->isShuttingDown())
            {
                QTimer::singleShot(30 * 1000, this, &CInstallXSwiftBusComponent::triggerLoadingXSwiftBusFileInfo);
            }

            ui->cb_DownloadFile->clear();
            if (!m_remoteFiles.isEmpty())
            {
                QStringList xSWiftBusFiles(m_remoteFiles.getNamesPlusSize(true));
                std::reverse(xSWiftBusFiles.begin(), xSWiftBusFiles.end()); // latest on top
                ui->cb_DownloadFile->addItems(xSWiftBusFiles);
                ui->cb_DownloadFile->setCurrentText(m_remoteFiles.backOrDefault().getNameAndSize()); // latest version
            }
            ui->cb_DownloadFile->setEnabled(!m_remoteFiles.isEmpty());
        }

        void CInstallXSwiftBusComponent::triggerDownloadingOfXSwiftBusFile()
        {
            if (!sGui || !sGui->hasWebDataServices() || sGui->isShuttingDown()) { return; }
            if (m_remoteFiles.isEmpty())
            {
                const CStatusMessage msg = CStatusMessage(this, CLogCategory::validation()).error("No remote file information");
                this->showOverlayMessage(msg, CInstallXSwiftBusComponent::OverlayMsgTimeoutMs);
                this->triggerLoadingXSwiftBusFileInfo(); // try to reload
                return;
            }
            if (!this->existsDownloadDir())
            {
                const CStatusMessage msg = CStatusMessage(this, CLogCategory::validation()).error("Invalid download directory");
                this->showOverlayMessage(msg, CInstallXSwiftBusComponent::OverlayMsgTimeoutMs);
                return;
            }

            const CRemoteFile rf = this->getRemoteFileSelected();
            const CUrl download = rf.getUrl();
            if (download.isEmpty())
            {
                const CStatusMessage msg = CStatusMessage(this, CLogCategory::validation()).error("No download URL for file name '%1'") << rf.getNameAndSize();
                this->showOverlayMessage(msg, CInstallXSwiftBusComponent::OverlayMsgTimeoutMs);
                return;
            }

            const QString saveAsFile = CFileUtils::appendFilePaths(ui->le_DownloadDir->text(), rf.getName());
            const QNetworkReply *r = sGui->downloadFromNetwork(download, saveAsFile, { this, &CInstallXSwiftBusComponent::downloadedXSwiftBusFile});
            if (r)
            {
                CLogMessage(this).info("Triggered downloading of XSwiftBus file from '%1'") << download.getHost();
                this->showLoading(10 * 1000);
            }
            else
            {
                const CStatusMessage msg = CStatusMessage(this, CLogCategory::validation()).error("Starting download for '%1' failed") << download.getFullUrl();
                this->showOverlayMessage(msg, CInstallXSwiftBusComponent::OverlayMsgTimeoutMs);
            }
        }

        void CInstallXSwiftBusComponent::downloadedXSwiftBusFile(const CStatusMessage &status)
        {
            this->hideLoading();
            if (sGui && sGui->isShuttingDown()) { return; }
            if (status.isWarningOrAbove() || !this->existsXSwiftBusPluginDir())
            {
                this->showOverlayMessage(status, CInstallXSwiftBusComponent::OverlayMsgTimeoutMs);
                return;
            }
            static const QString confirm("Install in '%1'?");
            this->showOverlayMessagesWithConfirmation(status, confirm.arg(ui->le_XSwiftBusPluginDir->text()), [ = ] { this->installXSwiftBus(); });
        }

        CRemoteFile CInstallXSwiftBusComponent::getRemoteFileSelected() const
        {
            const QString fileNameAndSize = ui->cb_DownloadFile->currentText();
            return m_remoteFiles.findFirstMatchingNameOrDefault(fileNameAndSize);
        }

        bool CInstallXSwiftBusComponent::existsDownloadDir() const
        {
            if (ui->le_DownloadDir->text().isEmpty()) { return false; }
            const QDir dir(ui->le_DownloadDir->text());
            return dir.exists() && dir.isReadable();
        }

        bool CInstallXSwiftBusComponent::existsXSwiftBusPluginDir() const
        {
            if (ui->le_XSwiftBusPluginDir->text().isEmpty()) { return false; }
            const QDir dir(ui->le_XSwiftBusPluginDir->text());
            return dir.exists() && dir.isReadable();
        }

        QString CInstallXSwiftBusComponent::getXPlanePluginDirectory() const
        {
            const CSimulatorSettings settings = m_simulatorSettings.getSettings(CSimulatorInfo::XPLANE);
            if (!settings.hasSimulatorDirectory()) { return CXPlaneUtil::xplanePluginDir(); }
            const QString dir = CFileUtils::appendFilePaths(settings.getSimulatorDirectory(), CXPlaneUtil::xplanePluginPath());
            return dir;
        }

        void CInstallXSwiftBusComponent::openInstallDir()
        {
            if (!this->existsXSwiftBusPluginDir()) { return; }
            QDesktopServices::openUrl(QUrl::fromLocalFile(ui->le_XSwiftBusPluginDir->text()));
        }

        void CInstallXSwiftBusComponent::openDownloadDir()
        {
            if (!this->existsDownloadDir()) { return; }
            QDesktopServices::openUrl(QUrl::fromLocalFile(ui->le_DownloadDir->text()));
        }

        bool CInstallXSwiftBusWizardPage::validatePage()
        {
            return true;
        }
    } // ns
} // ns
