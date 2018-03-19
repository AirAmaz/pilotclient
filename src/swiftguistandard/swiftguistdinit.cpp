/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "swiftguistd.h"
#include "ui_swiftguistd.h"
#include "blackgui/components/aircraftcomponent.h"
#include "blackgui/components/atcstationcomponent.h"
#include "blackgui/components/cockpitcomponent.h"
#include "blackgui/components/flightplancomponent.h"
#include "blackgui/components/logcomponent.h"
#include "blackgui/components/logincomponent.h"
#include "blackgui/components/interpolationcomponent.h"
#include "blackgui/components/maininfoareacomponent.h"
#include "blackgui/components/mainkeypadareacomponent.h"
#include "blackgui/components/mappingcomponent.h"
#include "blackgui/components/navigatordialog.h"
#include "blackgui/components/settingscomponent.h"
#include "blackgui/components/textmessagecomponent.h"
#include "blackgui/components/usercomponent.h"
#include "blackgui/dockwidgetinfobar.h"
#include "blackgui/guiapplication.h"
#include "blackgui/managedstatusbar.h"
#include "blackgui/overlaymessagesframe.h"
#include "blackgui/stylesheetutility.h"
#include "blackcore/webdataservices.h"
#include "blackcore/context/contextnetwork.h"
#include "blackcore/context/contextsimulator.h"
#include "blackmisc/network/networkutils.h"
#include "blackmisc/loghandler.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/logpattern.h"
#include "blackmisc/slot.h"
#include "blackmisc/statusmessage.h"
#include "blackconfig/buildconfig.h"

#include <QAction>
#include <QHBoxLayout>
#include <QScopedPointer>
#include <QStackedWidget>
#include <QStatusBar>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>

class QHBoxLayout;

using namespace BlackConfig;
using namespace BlackCore;
using namespace BlackCore::Context;
using namespace BlackMisc;
using namespace BlackMisc::Network;
using namespace BlackMisc::Input;
using namespace BlackGui;
using namespace BlackGui::Components;

void SwiftGuiStd::init()
{
    // POST(!) GUI init
    Q_ASSERT_X(sGui, Q_FUNC_INFO, "Missing sGui");
    Q_ASSERT_X(sGui->getWebDataServices(), Q_FUNC_INFO, "Missing web services");
    Q_ASSERT_X(sGui->supportsContexts(), Q_FUNC_INFO, "Missing contexts");

    if (m_init) { return; }

    this->setVisible(false); // hide all, so no flashing windows during init
    m_mwaStatusBar = &m_statusBar;
    m_mwaOverlayFrame = ui->fr_CentralFrameInside;
    m_mwaLogComponent = ui->comp_MainInfoArea->getLogComponent();

    sGui->initMainApplicationWidget(this);

    // log messages
    m_logSubscriber.changeSubscription(CLogPattern().withSeverityAtOrAbove(CStatusMessage::SeverityInfo));

    // style
    this->initStyleSheet();

    // with frameless window, we shift menu and statusbar into central widget
    // http://stackoverflow.com/questions/18316710/frameless-and-transparent-window-qt5
    if (this->isFrameless())
    {
        // wrap menu in layout, add button to menu bar and insert on top
        QHBoxLayout *menuBarLayout = this->addFramelessCloseButton(ui->mb_MainMenuBar);
        ui->vl_CentralWidgetOutside->insertLayout(0, menuBarLayout, 0);

        // now insert the dock widget info bar into the widget
        ui->vl_CentralWidgetOutside->insertWidget(1, ui->dw_InfoBarStatus);

        // move the status bar into the frame
        // (otherwise it is dangling outside the frame as it belongs to the window)
        ui->sb_MainStatusBar->setParent(ui->wi_CentralWidgetOutside);
        ui->vl_CentralWidgetOutside->addWidget(ui->sb_MainStatusBar, 0);

        // grip
        this->addFramelessSizeGripToStatusBar(ui->sb_MainStatusBar);
    }

    // timers
    m_timerContextWatchdog.setObjectName(this->objectName().append(":m_timerContextWatchdog"));

    // info bar and status bar
    m_statusBar.initStatusBar(ui->sb_MainStatusBar);
    ui->dw_InfoBarStatus->allowStatusBar(false);
    ui->dw_InfoBarStatus->setPreferredSizeWhenFloating(ui->dw_InfoBarStatus->size()); // set floating size

    // navigator
    m_navigator->addAction(this->getToggleWindowVisibilityAction(m_navigator.data()));
    m_navigator->addActions(ui->comp_MainInfoArea->getInfoAreaToggleFloatingActions(m_navigator.data()));
    m_navigator->addAction(this->getWindowNormalAction(m_navigator.data()));
    m_navigator->addAction(this->getWindowMinimizeAction(m_navigator.data()));
    m_navigator->addAction(this->getToggleStayOnTopAction(m_navigator.data()));
    m_navigator->buildNavigator(1);

    // wire GUI signals
    this->initGuiSignals();

    // signal / slots contexts / timers
    Q_ASSERT_X(sGui->getIContextNetwork(), Q_FUNC_INFO, "Missing network context");
    Q_ASSERT_X(sGui->getIContextSimulator(), Q_FUNC_INFO, "Missing simulator context");

    bool s = connect(sGui->getIContextNetwork(), &IContextNetwork::connectionStatusChanged, this, &SwiftGuiStd::onConnectionStatusChanged, Qt::QueuedConnection);
    Q_ASSERT(s);
    s = connect(sGui->getIContextNetwork(), &IContextNetwork::kicked, this, &SwiftGuiStd::onKickedFromNetwork, Qt::QueuedConnection);
    Q_ASSERT(s);
    s = connect(sGui->getIContextNetwork(), &IContextNetwork::textMessagesReceived, ui->comp_MainInfoArea->getTextMessageComponent(), &CTextMessageComponent::onTextMessageReceived, Qt::QueuedConnection);
    Q_ASSERT(s);
    s = connect(sGui->getIContextNetwork(), &IContextNetwork::textMessageSent, ui->comp_MainInfoArea->getTextMessageComponent(), &CTextMessageComponent::onTextMessageSent, Qt::QueuedConnection);
    Q_ASSERT(s);
    s = connect(sGui->getIContextSimulator(), &IContextSimulator::requestUiConsoleMessage, this, [ = ](const QString & logMsg, bool clear)
    {
        if (logMsg.isEmpty()) { return; }
        CLogComponent *log = ui->comp_MainInfoArea->getLogComponent();
        Q_ASSERT_X(log, Q_FUNC_INFO, "Missing log component");
        if (clear) { log->clearConsole(); }
        log->appendPlainTextToConsole(logMsg);
    },
    Qt::QueuedConnection);
    Q_ASSERT(s);
    s = connect(&m_timerContextWatchdog, &QTimer::timeout, this, &SwiftGuiStd::handleTimerBasedUpdates);
    Q_ASSERT(s);
    Q_UNUSED(s);

    // check if DB data have been loaded
    // only check once, so data can be loaded and
    connectOnce(sGui->getWebDataServices(), &CWebDataServices::sharedInfoObjectsRead, this, &SwiftGuiStd::checkDbDataLoaded, Qt::QueuedConnection);

    // start timers, update timers will be started when network is connected
    m_timerContextWatchdog.start(2500);

    // init availability
    this->setContextAvailability();

    // data
    this->initialContextDataReads();

    // start screen and complete menu
    this->setMainPageToInfoArea();
    this->initMenus();

    // info
    ui->comp_MainInfoArea->getLogComponent()->appendPlainTextToConsole(sGui->swiftVersionString());
    ui->comp_MainInfoArea->getLogComponent()->appendPlainTextToConsole(CBuildConfig::compiledWithInfo());

    // Show kill button
    ui->fr_CentralFrameInside->showKillButton(true);

    // do this as last statement, so it can be used as flag
    // whether init has been completed
    this->setVisible(true);

    emit sGui->startUpCompleted(true);
    m_init = true;

    QTimer::singleShot(2500, this, &SwiftGuiStd::verifyPrerequisites);
}

void SwiftGuiStd::initStyleSheet()
{
    const QString s = sGui->getStyleSheetUtility().styles(
    {
        CStyleSheetUtility::fileNameFonts(),
        CStyleSheetUtility::fileNameStandardWidget(),
        CStyleSheetUtility::fileNameSwiftStandardGui()
    }
    );
    this->setStyleSheet(s);
}

void SwiftGuiStd::initGuiSignals()
{
    // Remark: With new style, only methods of same signature can be connected
    // This is why we still have some "old" SIGNAL/SLOT connections here

    // main window
    connect(ui->sw_MainMiddle, &QStackedWidget::currentChanged, this, &SwiftGuiStd::onCurrentMainWidgetChanged);

    // main keypad
    connect(ui->comp_MainKeypadArea, &CMainKeypadAreaComponent::selectedMainInfoAreaDockWidget, this, &SwiftGuiStd::setMainPageInfoArea);
    connect(ui->comp_MainKeypadArea, &CMainKeypadAreaComponent::connectPressed, this, &SwiftGuiStd::loginRequested);
    connect(ui->comp_MainKeypadArea, &CMainKeypadAreaComponent::changedOpacity, this, &SwiftGuiStd::onChangedWindowOpacity);
    connect(ui->comp_MainKeypadArea, &CMainKeypadAreaComponent::identPressed, ui->comp_MainInfoArea->getCockpitComponent(), &CCockpitComponent::setSelectedTransponderModeStateIdent);
    connect(ui->comp_MainKeypadArea, &CMainKeypadAreaComponent::textEntered, ui->comp_MainInfoArea->getTextMessageComponent(), &CTextMessageComponent::handleGlobalCommandLineText);
    connect(ui->comp_MainKeypadArea, &CMainKeypadAreaComponent::audioPressed, [ = ]
    {
        ui->comp_MainInfoArea->getCockpitComponent()->showAudio();
        ui->comp_MainInfoArea->selectArea(CMainInfoAreaComponent::InfoAreaCockpit);
    });
    connect(ui->comp_MainInfoArea, &CMainInfoAreaComponent::changedInfoAreaStatus, ui->comp_MainKeypadArea, &CMainKeypadAreaComponent::onMainInfoAreaChanged);

    // menu
    connect(ui->menu_TestLocationsEDDF, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_TestLocationsEDDM, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_TestLocationsEDNX, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_TestLocationsEDRY, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_TestLocationsLOWW, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);

    connect(ui->menu_WindowFont, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_WindowMinimize, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_WindowToggleOnTop, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_WindowToggleNavigator, &QAction::triggered, m_navigator.data(), &CNavigatorDialog::toggleNavigator);
    connect(ui->menu_InternalsPage, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_MovingMap, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(m_navigator.data(), &CNavigatorDialog::navigatorClosed, this, &SwiftGuiStd::navigatorClosed);

    // command line / text messages
    connect(ui->comp_MainInfoArea->getTextMessageComponent(), &CTextMessageComponent::displayInInfoWindow, ui->fr_CentralFrameInside, &COverlayMessagesFrame::showOverlayVariant);

    // settings (GUI component), styles
    connect(ui->comp_MainInfoArea->getSettingsComponent(), &CSettingsComponent::changedWindowsOpacity, this, &SwiftGuiStd::onChangedWindowOpacity);
    connect(sGui, &CGuiApplication::styleSheetsChanged, this, &SwiftGuiStd::onStyleSheetsChanged);

    // login
    connect(ui->comp_Login, &CLoginComponent::loginOrLogoffCancelled, this, &SwiftGuiStd::setMainPageToInfoArea);
    connect(ui->comp_Login, &CLoginComponent::loginOrLogoffSuccessful, this, &SwiftGuiStd::setMainPageToInfoArea);
    connect(ui->comp_Login, &CLoginComponent::loginOrLogoffSuccessful, ui->comp_MainInfoArea->getFlightPlanComponent(), &CFlightPlanComponent::loginDataSet);
    connect(ui->comp_Login, &CLoginComponent::loginDataChangedDigest, ui->comp_MainInfoArea->getFlightPlanComponent(), &CFlightPlanComponent::loginDataSet);
    connect(ui->comp_Login, &CLoginComponent::requestNetworkSettings, [ this ]()
    {
        this->setMainPageInfoArea(CMainInfoAreaComponent::InfoAreaSettings);
        ui->comp_MainInfoArea->getSettingsComponent()->setSettingsTab(CSettingsComponent::SettingTabServers);
    });
    connect(this, &SwiftGuiStd::currentMainInfoAreaChanged, ui->comp_Login, &CLoginComponent::mainInfoAreaChanged);

    // text messages
    connect(ui->comp_MainInfoArea->getAtcStationComponent(), &CAtcStationComponent::requestTextMessageWidget, ui->comp_MainInfoArea->getTextMessageComponent(), &CTextMessageComponent::showCorrespondingTab);
    connect(ui->comp_MainInfoArea->getMappingComponent(), &CMappingComponent::requestTextMessageWidget, ui->comp_MainInfoArea->getTextMessageComponent(), &CTextMessageComponent::showCorrespondingTab);
    connect(ui->comp_MainInfoArea->getAircraftComponent(), &CAircraftComponent::requestTextMessageWidget, ui->comp_MainInfoArea->getTextMessageComponent(), &CTextMessageComponent::showCorrespondingTab);

    // interpolation
    connect(ui->comp_MainInfoArea->getInterpolationComponent(), &CInterpolationComponent::requestRenderingRestrictionsWidget, [ = ]
    {
        this->setSettingsPage(CSettingsComponent::SettingTabSimulator);
    });

    // main info area
    connect(ui->comp_MainInfoArea, &CMainInfoAreaComponent::changedWholeInfoAreaFloating, this, &SwiftGuiStd::onChangedMainInfoAreaFloating);
}

void SwiftGuiStd::initialContextDataReads()
{
    this->setContextAvailability();
    if (!m_coreAvailable)
    {
        CLogMessage(this).error("No initial data read as network context is not available");
        return;
    }

    this->reloadOwnAircraft(); // init read, independent of traffic network
    CLogMessage(this).info("Initial data read");
}

void SwiftGuiStd::stopAllTimers(bool disconnectSignalSlots)
{
    m_timerContextWatchdog.stop();
    if (!disconnectSignalSlots) { return; }
    this->disconnect(&m_timerContextWatchdog);
}
