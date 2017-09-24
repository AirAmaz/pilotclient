/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackconfig/buildconfig.h"
#include "blackcore/context/contextnetwork.h"
#include "blackcore/data/globalsetup.h"
#include "blackgui/components/applicationclosedialog.h"
#include "blackgui/components/downloadandinstalldialog.h"
#include "blackgui/components/aboutdialog.h"
#include "blackgui/guiapplication.h"
#include "blackgui/guiutility.h"
#include "blackgui/registermetadata.h"
#include "blackmisc/directoryutils.h"
#include "blackmisc/datacache.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/metadatautils.h"
#include "blackmisc/registermetadata.h"
#include "blackmisc/settingscache.h"
#include "blackmisc/verify.h"

#include <QAction>
#include <QCloseEvent>
#include <QApplication>
#include <QCommandLineParser>
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QGuiApplication>
#include <QIcon>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QSplashScreen>
#include <QStyleFactory>
#include <QStringList>
#include <QStyle>
#include <QSysInfo>
#include <QUrl>
#include <QWidget>
#include <QMainWindow>
#include <QtGlobal>

using namespace BlackConfig;
using namespace BlackMisc;
using namespace BlackMisc::Db;
using namespace BlackMisc::Network;
using namespace BlackGui::Components;
using namespace BlackCore::Data;

BlackGui::CGuiApplication *sGui = nullptr; // set by constructor

namespace BlackGui
{
    CGuiApplication *CGuiApplication::instance()
    {
        return qobject_cast<CGuiApplication *>(CApplication::instance());
    }

    const BlackMisc::CLogCategoryList &CGuiApplication::getLogCategories()
    {
        static const CLogCategoryList l(CApplication::getLogCategories().join({ CLogCategory::guiComponent() }));
        return l;
    }

    CGuiApplication::CGuiApplication(const QString &applicationName, CApplicationInfo::Application application, const QPixmap &icon) :
        CApplication(applicationName, application, false)
    {
        if (!sGui)
        {
            CGuiApplication::registerMetadata();
            CApplication::init(false); // base class without metadata
            CGuiApplication::adjustPalette();
            this->setWindowIcon(icon);
            this->settingsChanged();
            sGui = this;
            connect(&this->m_styleSheetUtility, &CStyleSheetUtility::styleSheetsChanged, this, &CGuiApplication::styleSheetsChanged);
        }
    }

    CGuiApplication::~CGuiApplication()
    {
        sGui = nullptr;
    }

    void CGuiApplication::registerMetadata()
    {
        CApplication::registerMetadata();
        BlackGui::registerMetadata();
    }

    void CGuiApplication::addWindowModeOption()
    {
        this->m_cmdWindowMode = QCommandLineOption(QStringList() << "w" << "window",
                                QCoreApplication::translate("main", "Windows: (n)ormal, (f)rameless, (t)ool."),
                                "windowtype");
        this->addParserOption(this->m_cmdWindowMode);
    }

    void CGuiApplication::addWindowStateOption()
    {
        this->m_cmdWindowStateMinimized =  QCommandLineOption({{"m", "minimized"}, QCoreApplication::translate("main", "Start minimized in system tray.")});
        this->addParserOption(this->m_cmdWindowStateMinimized);
    }

    Qt::WindowState CGuiApplication::getWindowState() const
    {
        if (this->m_cmdWindowStateMinimized.valueName() == "empty") { return Qt::WindowNoState; }
        if (this->m_parser.isSet(this->m_cmdWindowStateMinimized)) { return Qt::WindowMinimized; }
        return Qt::WindowNoState;
    }

    CEnableForFramelessWindow::WindowMode CGuiApplication::getWindowMode() const
    {
        if (this->isParserOptionSet(m_cmdWindowMode))
        {
            const QString v(this->getParserValue(this->m_cmdWindowMode));
            return CEnableForFramelessWindow::stringToWindowMode(v);
        }
        else
        {
            return CEnableForFramelessWindow::WindowNormal;
        }
    }

    void CGuiApplication::splashScreen(const QString &resource)
    {
        if (this->m_splashScreen)
        {
            // delete old one
            this->m_splashScreen.reset();
        }
        if (!resource.isEmpty())
        {
            const QPixmap pm(resource);
            this->splashScreen(pm);
        }
    }

    void CGuiApplication::splashScreen(const QPixmap &pixmap)
    {
        if (this->m_splashScreen)
        {
            // delete old one
            this->m_splashScreen.reset();
        }
        this->m_splashScreen.reset(new QSplashScreen(pixmap.scaled(256, 256)));
        this->m_splashScreen->show();
        this->processEventsToRefreshGui();
    }

    void CGuiApplication::processEventsToRefreshGui() const
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    QWidget *CGuiApplication::mainApplicationWindow()
    {
        return CGuiUtility::mainApplicationWindow();
    }

    IMainWindowAccess *CGuiApplication::mainWindowAccess()
    {
        IMainWindowAccess *m = qobject_cast<IMainWindowAccess *>(mainApplicationWindow());
        return m;
    }

    void CGuiApplication::initMainApplicationWindow(QWidget *mainWindow)
    {
        if (!mainWindow) { return; }
        if (this->m_uiSetupCompleted) { return; }
        this->m_uiSetupCompleted = true;
        const QString name(this->getApplicationNameVersionBetaDev());
        mainWindow->setObjectName(QCoreApplication::applicationName());
        mainWindow->setWindowTitle(name);
        mainWindow->setWindowIcon(m_windowIcon);
        mainWindow->setWindowIconText(name);
        CStyleSheetUtility::setQSysInfoProperties(CGuiApplication::mainApplicationWindow(), true);
        emit uiObjectTreeReady();
    }

    void CGuiApplication::setWindowIcon(const QPixmap &icon)
    {
        instance()->m_windowIcon = icon;
        QApplication::setWindowIcon(icon);
    }

    void CGuiApplication::exit(int retcode)
    {
        CApplication::exit(retcode);
    }

    void CGuiApplication::highDpiScreenSupport()
    {
        qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
    }

    void CGuiApplication::ps_startupCompleted()
    {
        CApplication::ps_startupCompleted();
        if (this->m_splashScreen)
        {
            this->m_splashScreen->close();
            this->m_splashScreen.reset();
        }
    }

    QString CGuiApplication::beautifyHelpMessage(const QString &helpText)
    {
        // just formatting Qt help message into HTML table
        if (helpText.isEmpty()) { return ""; }
        const QStringList lines(helpText.split('\n'));
        QString html;
        bool tableMode = false;
        bool pendingTr = false;
        for (const QString &l : lines)
        {
            QString lt(l.trimmed());
            if (!tableMode && lt.startsWith("-"))
            {
                tableMode = true;
                html += "<table>\n";
            }
            if (!tableMode)
            {
                html += l.toHtmlEscaped();
                html += "<br>";
            }
            else
            {
                // in table mode
                if (lt.startsWith("-"))
                {
                    if (pendingTr)
                    {
                        html += "</td></tr>\n";
                    }
                    html += "<tr><td>";
                    thread_local const QRegularExpression reg("[ ]{2,}");
                    html += lt.replace(reg, "</td><td>");
                    pendingTr = true;
                }
                else
                {
                    html += " ";
                    html += l.simplified().toHtmlEscaped();
                }
            }
        }
        html += "</table>\n";
        return html;
    }

    bool CGuiApplication::cmdLineErrorMessage(const QString &errorMessage, bool retry) const
    {
        const QString helpText(beautifyHelpMessage(this->m_parser.helpText()));
        constexpr int MaxLength = 60;

        QString htmlMsg;
        if (errorMessage.length() > MaxLength)
        {
            htmlMsg = "<html><head/><body><h4>" + errorMessage.left(MaxLength) + "..." + "</h4>" +
                      "Details: " + errorMessage + "<br><br>";
        }
        else
        {
            htmlMsg = "<html><head/><body><h4>" + errorMessage + "</h4>";
        }
        htmlMsg += helpText + "</body></html>";

        const int r = QMessageBox::warning(nullptr,
                                           QGuiApplication::applicationDisplayName(),
                                           htmlMsg, QMessageBox::Abort, retry ? QMessageBox::Retry : QMessageBox::NoButton);
        return (r == QMessageBox::Retry);
    }

    bool CGuiApplication::cmdLineErrorMessage(const CStatusMessageList &msgs, bool retry) const
    {
        if (msgs.isEmpty()) { return false; }
        if (!msgs.hasErrorMessages()) { return false; }
        static const CPropertyIndexList propertiesSingle({ CStatusMessage::IndexMessage });
        static const CPropertyIndexList propertiesMulti({ CStatusMessage::IndexSeverityAsString, CStatusMessage::IndexMessage });
        const QString helpText(CGuiApplication::beautifyHelpMessage(this->m_parser.helpText()));
        const QString msgsHtml = msgs.toHtml(msgs.size() > 1 ? propertiesMulti : propertiesSingle);
        const int r = QMessageBox::critical(nullptr,
                                            QGuiApplication::applicationDisplayName(),
                                            "<html><head><body>" + msgsHtml + "<br><br>" + helpText + "</body></html>", QMessageBox::Abort, retry ? QMessageBox::Retry : QMessageBox::NoButton);
        return (r == QMessageBox::Retry);
    }

    bool CGuiApplication::displayInStatusBar(const CStatusMessage &message)
    {
        IMainWindowAccess *m = mainWindowAccess();
        BLACK_VERIFY_X(m, Q_FUNC_INFO, "No access interface");
        if (!m) { return false; }
        return m->displayInStatusBar(message);
    }

    bool CGuiApplication::displayInOverlayWindow(const CStatusMessage &message, int timeOutMs)
    {
        IMainWindowAccess *m = mainWindowAccess();
        BLACK_VERIFY_X(m, Q_FUNC_INFO, "No access interface");
        if (!m) { return false; }
        return m->displayInOverlayWindow(message, timeOutMs);
    }

    bool CGuiApplication::displayTextInConsole(const QString &text)
    {
        IMainWindowAccess *m = mainWindowAccess();
        BLACK_VERIFY_X(m, Q_FUNC_INFO, "No access interface");
        if (!m) { return false; }
        return m->displayTextInConsole(text);
    }

    void CGuiApplication::addMenuForSettingsAndCache(QMenu &menu)
    {
        QMenu *sm = menu.addMenu(CIcons::appSettings16(), "Settings");
        sm->setIcon(CIcons::appSettings16());
        QAction *a = sm->addAction(CIcons::disk16(), "Settings directory");
        bool c = connect(a, &QAction::triggered, this, [a, this]()
        {
            const QString path(QDir::toNativeSeparators(CSettingsCache::persistentStore()));
            if (QDir(path).exists())
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            }
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = sm->addAction("Reset settings");
        c = connect(a, &QAction::triggered, this, [this]()
        {
            CSettingsCache::instance()->clearAllValues();
            this->displayTextInConsole("Cleared all settings!");
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = sm->addAction("List settings files");
        c = connect(a, &QAction::triggered, this, [this]()
        {
            const QStringList files(CSettingsCache::instance()->enumerateStore());
            this->displayTextInConsole(files.join("\n"));
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        sm = menu.addMenu("Cache");
        sm->setIcon(CIcons::appSettings16());
        a = sm->addAction(CIcons::disk16(), "Cache directory");
        c = connect(a, &QAction::triggered, this, [this]()
        {
            const QString path(QDir::toNativeSeparators(CDataCache::persistentStore()));
            if (QDir(path).exists())
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            }
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = sm->addAction("Reset cache");
        c = connect(a, &QAction::triggered, this, [this]()
        {
            const QStringList files = CApplication::clearCaches();
            this->displayTextInConsole("Cleared caches! " + QString::number(files.size()) + " files");
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = sm->addAction("List cache files");
        c = connect(a, &QAction::triggered, this, [this]()
        {
            const QStringList files(CDataCache::instance()->enumerateStore());
            this->displayTextInConsole(files.join("\n"));
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = menu.addAction(CIcons::disk16(), "Log directory");
        c = connect(a, &QAction::triggered, this, [this]()
        {
            const QString path(QDir::toNativeSeparators(CDirectoryUtils::logDirectory()));
            if (QDir(path).exists())
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            }
        });

        a = menu.addAction(CIcons::swift24(), "Check for updates");
        c = connect(a, &QAction::triggered, this, &CGuiApplication::checkNewVersionMenu);

        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);
    }

    void CGuiApplication::addMenuForStyleSheets(QMenu &menu)
    {
        QMenu *sm = menu.addMenu("Style sheet");
        QAction *a = sm->addAction(CIcons::refresh16(), "Reload");
        bool c = connect(a, &QAction::triggered, this, [a, this]()
        {
            this->reloadStyleSheets();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);
    }

    void CGuiApplication::addMenuFile(QMenu &menu)
    {
        addMenuForSettingsAndCache(menu);
        addMenuForStyleSheets(menu);
        QAction *a = nullptr;
        bool c = false;
        if (this->getApplicationInfo().application() != CApplicationInfo::Laucher)
        {
            menu.addSeparator();
            a = menu.addAction(CIcons::swiftLauncher24(), "Start swift launcher");
            c = connect(a, &QAction::triggered, this, &CGuiApplication::startLauncher);
            Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        }

        menu.addSeparator();
        a = menu.addAction("E&xit");
        a->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
        c = connect(a, &QAction::triggered, this, [a, this]()
        {
            // a close event might already trigger a shutdown
            this->mainApplicationWindow()->close();
            this->gracefulShutdown();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);
    }

    void CGuiApplication::addMenuInternals(QMenu &menu)
    {
        QMenu *sm = menu.addMenu("Templates");
        QAction *a = sm->addAction("JSON bootstrap");
        bool c = connect(a, &QAction::triggered, this, [a, this]()
        {
            const CGlobalSetup s = this->getGlobalSetup();
            this->displayTextInConsole(s.toJsonString());
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = sm->addAction("JSON distributions (info only)");
        c = connect(a, &QAction::triggered, this, [a, this]()
        {
            const CDistributionList d = this->getDistributionInfo();
            this->displayTextInConsole(d.toJsonString());
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = menu.addAction("Metadata (slow)");
        c = connect(a, &QAction::triggered, this, [a, this]()
        {
            this->displayTextInConsole(getAllUserMetatypesTypes());
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);
    }

    void CGuiApplication::addMenuWindow(QMenu &menu)
    {
        QWidget *w = mainApplicationWindow();
        if (!w) { return; }
        const QSize iconSize = CIcons::empty16().size();
        QPixmap icon = w->style()->standardIcon(QStyle::SP_TitleBarMaxButton).pixmap(iconSize);
        QAction *a = menu.addAction(icon.scaled(iconSize), "Fullscreen");
        bool c = connect(a, &QAction::triggered, this, [a, w]()
        {
            w->showFullScreen();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        icon = w->style()->standardIcon(QStyle::SP_TitleBarMinButton).pixmap(iconSize);
        a = menu.addAction(icon.scaled(iconSize), "Minimize");
        c = connect(a, &QAction::triggered, this, [a, w]()
        {
            w->showMinimized();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        icon = w->style()->standardIcon(QStyle::SP_TitleBarNormalButton).pixmap(iconSize);
        a = menu.addAction(icon.scaled(iconSize), "Normal");
        c = connect(a, &QAction::triggered, this, [a, w]()
        {
            w->showNormal();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = menu.addAction("Toggle stay on top");
        c = connect(a, &QAction::triggered, this, [a, w]()
        {
            if (CGuiUtility::toggleStayOnTop(w))
            {
                CLogMessage(w).info("Window on top");
            }
            else
            {
                CLogMessage(w).info("Window not always on top");
            }
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);
    }

    void CGuiApplication::addMenuHelp(QMenu &menu)
    {
        QWidget *w = mainApplicationWindow();
        if (!w) { return; }
        QAction *a = menu.addAction(w->style()->standardIcon(QStyle::SP_TitleBarContextHelpButton), "Online help");
        bool c = connect(a, &QAction::triggered, this, [this]()
        {
            this->showHelp();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = menu.addAction(QApplication::windowIcon(), "About swift");
        c = connect(a, &QAction::triggered, this, [w]()
        {
            CAboutDialog dialog(w);
            dialog.exec();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);

        // https://joekuan.wordpress.com/2015/09/23/list-of-qt-icons/
        a = menu.addAction(QApplication::style()->standardIcon(QStyle::SP_TitleBarMenuButton), "About Qt");
        c = connect(a, &QAction::triggered, this, []()
        {
            QApplication::aboutQt();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);
    }

    void CGuiApplication::showHelp()
    {
        const CGlobalSetup gs = this->getGlobalSetup();
        const CUrl helpPage = gs.getHelpPageUrl();
        if (helpPage.isEmpty())
        {
            CLogMessage(this).warning("No help page");
            return;
        }
        QDesktopServices::openUrl(helpPage);
    }

    const CStyleSheetUtility &CGuiApplication::getStyleSheetUtility() const
    {
        return this->m_styleSheetUtility;
    }

    QString CGuiApplication::getWidgetStyle() const
    {
        QString currentWidgetStyle(QApplication::style()->metaObject()->className());
        if (currentWidgetStyle.startsWith('Q')) { currentWidgetStyle.remove(0, 1); }
        return currentWidgetStyle.replace("Style", "");
    }

    bool CGuiApplication::reloadStyleSheets()
    {
        return m_styleSheetUtility.read();
    }

    bool CGuiApplication::updateFont(const QString &fontFamily, const QString &fontSize, const QString &fontStyle, const QString &fontWeight, const QString &fontColor)
    {
        return m_styleSheetUtility.updateFont(fontFamily, fontSize, fontStyle, fontWeight, fontColor);
    }

    bool CGuiApplication::updateFont(const QString &qss)
    {
        return m_styleSheetUtility.updateFont(qss);
    }

    bool CGuiApplication::resetFont()
    {
        return m_styleSheetUtility.resetFont();
    }

    QDialog::DialogCode CGuiApplication::showCloseDialog(QMainWindow *mainWindow, QCloseEvent *closeEvent)
    {
        const bool needsDialog = this->hasUnsavedSettings();
        if (!needsDialog) { return QDialog::Accepted; }
        if (!this->m_closeDialog)
        {
            this->m_closeDialog = new CApplicationCloseDialog(mainWindow);
            if (mainWindow && !mainWindow->windowTitle().isEmpty())
            {
                this->setSettingsAutoSave(false); // will be handled by dialog
                this->m_closeDialog->setWindowTitle(mainWindow->windowTitle());
                this->m_closeDialog->setModal(true);
            }
        }
        const QDialog::DialogCode c = static_cast<QDialog::DialogCode>(this->m_closeDialog->exec());
        if (c == QDialog::Rejected)
        {
            if (closeEvent) { closeEvent->ignore(); }
        }
        return c;
    }

    void CGuiApplication::cmdLineHelpMessage()
    {
        if (CBuildConfig::isRunningOnWindowsNtPlatform())
        {
            const QString helpText(CGuiApplication::beautifyHelpMessage(this->m_parser.helpText()));
            QMessageBox::information(nullptr, QGuiApplication::applicationDisplayName(),
                                     "<html><head/><body>" + helpText + "</body></html>");
        }
        else
        {
            CApplication::cmdLineHelpMessage();
        }
    }

    void CGuiApplication::cmdLineVersionMessage() const
    {
        if (CBuildConfig::isRunningOnWindowsNtPlatform())
        {
            QMessageBox::information(nullptr, QGuiApplication::applicationDisplayName(),
                                     QGuiApplication::applicationDisplayName() + ' ' + QCoreApplication::applicationVersion());
        }
        else
        {
            CApplication::cmdLineVersionMessage();
        }
    }

    bool CGuiApplication::parsingHookIn()
    {
        return true;
    }

    void CGuiApplication::checkNewVersion(bool onlyIfNew)
    {
        if (!m_installDialog)
        {
            // without parent stylesheet is not inherited
            m_installDialog = new CDownloadAndInstallDialog(this->mainApplicationWindow());
        }

        if (onlyIfNew && !m_installDialog->isNewVersionAvailable()) return;
        const int result = m_installDialog->exec();
        if (result != QDialog::Accepted) { return; }
    }

    void CGuiApplication::triggerNewVersionCheck(int delayedMs)
    {
        if (!m_updateSetting.get()) { return; }
        QTimer::singleShot(delayedMs, this, [ = ]
        {
            if (this->m_installDialog) { return; }
            this->checkNewVersion(true);
        });
    }

    void CGuiApplication::settingsChanged()
    {
        // changing widget style is slow, so I try to prevent setting it when nothing changed
        const QString widgetStyle = m_guiSettings.get().getWidgetStyle();
        const QString currentWidgetStyle(this->getWidgetStyle());
        if (!(currentWidgetStyle.length() == widgetStyle.length() && currentWidgetStyle.startsWith(widgetStyle, Qt::CaseInsensitive)))
        {
            const auto availableStyles = QStyleFactory::keys();
            if (availableStyles.contains(widgetStyle))
            {
                // changing style freezes the application, so it must not be done in flight mode
                if (this->getIContextNetwork() && this->getIContextNetwork()->isConnected())
                {
                    CLogMessage(this).validationError("Cannot change style while connected to network");
                }
                else
                {
                    QApplication::setStyle(QStyleFactory::create(widgetStyle));
                }
            }
        }
    }

    void CGuiApplication::checkNewVersionMenu()
    {
        this->checkNewVersion(false);
    }

    void CGuiApplication::adjustPalette()
    {
        // only way to change link color
        // https://stackoverflow.com/q/5497799/356726
        // Ref T84
        QPalette newPalette(qApp->palette());
        const QColor linkColor(135, 206, 250);
        newPalette.setColor(QPalette::Link, linkColor);
        newPalette.setColor(QPalette::LinkVisited, linkColor);
        qApp->setPalette(newPalette);
    }
} // ns
