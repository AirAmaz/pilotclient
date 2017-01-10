/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_ATCSTATIONCOMPONENT_H
#define BLACKGUI_ATCSTATIONCOMPONENT_H

#include "blackcore/network.h"
#include "blackgui/blackguiexport.h"
#include "blackgui/components/enablefordockwidgetinfoarea.h"
#include "blackgui/settings/viewupdatesettings.h"
#include "blackgui/settings/atcstationssettings.h"
#include "blackmisc/aviation/atcstation.h"
#include "blackmisc/aviation/comsystem.h"
#include "blackmisc/identifiable.h"
#include "blackmisc/pq/frequency.h"

#include <QDateTime>
#include <QModelIndex>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QTabWidget>
#include <QtGlobal>
#include <QTimer>

class QWidget;

namespace BlackMisc { namespace Aviation { class CCallsign; } }
namespace Ui { class CAtcStationComponent; }
namespace BlackGui
{
    class CDockWidgetInfoArea;
    namespace Components
    {
        //! ATC stations component
        class BLACKGUI_EXPORT CAtcStationComponent :
            public QTabWidget,
            public CEnableForDockWidgetInfoArea,
            public BlackMisc::CIdentifiable
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CAtcStationComponent(QWidget *parent = nullptr);

            //! Destructor
            virtual ~CAtcStationComponent();

            //! Number of booked stations
            int countBookedStations() const;

            //! Number of online stations
            int countOnlineStations() const;

            //! \copydoc CEnableForDockWidgetInfoArea::setParentDockWidgetInfoArea
            virtual bool setParentDockWidgetInfoArea(BlackGui::CDockWidgetInfoArea *parentDockableWidget) override;

        signals:
            //! Request a text message
            void requestTextMessageWidget(const BlackMisc::Aviation::CCallsign &callsign);

        public slots:
            //! Update stations
            void update();

            //! Get METAR for given ICAO airport code
            void getMetar(const QString &airportIcaoCode);

            //! \copydoc Models::CAtcStationListModel::changedAtcStationConnectionStatus
            void changedAtcStationOnlineConnectionStatus(const BlackMisc::Aviation::CAtcStation &station, bool added);

        private slots:
            //! Get all METARs
            void ps_getMetarAsEntered();

            //! Request new ATIS
            void ps_requestAtis();

            //! Online ATC station selected
            void ps_onlineAtcStationSelected(QModelIndex index);

            //! Tab changed
            void ps_atcStationsTabChanged();

            //! Booked stations reloading
            void ps_reloadAtcStationsBooked();

            //! Booked stations changed
            void ps_changedAtcStationsBooked();

            //! Online stations changed
            void ps_changedAtcStationsOnline();

            //! Connection status has been changed
            void ps_connectionStatusChanged(BlackCore::INetwork::ConnectionStatus from, BlackCore::INetwork::ConnectionStatus to);

            //! Request dummy ATC online stations
            void ps_testCreateDummyOnlineAtcStations(int number);

            //! Request udpate
            void ps_requestOnlineStationsUpdate();

            //! Info area tab bar has changed
            void ps_infoAreaTabBarChanged(int index);

            //! Count has been changed
            void ps_onCountChanged(int count, bool withFilter);

            //! Set COM frequency
            void ps_setComFrequency(const BlackMisc::PhysicalQuantities::CFrequency &frequency, BlackMisc::Aviation::CComSystem::ComUnit unit);

            //! Airports read from web readers
            void ps_airportsRead();

        private:
            //! Build a tree view for ATC stations
            void updateTreeView();

            //! Init the completers
            void initCompleters();

            //! Settings have been changed
            void settingsChanged();

            QScopedPointer<Ui::CAtcStationComponent> ui;
            QTimer m_updateTimer { this };
            QDateTime m_timestampLastReadOnlineStations; //!< stations read
            QDateTime m_timestampOnlineStationsChanged;  //!< stations marked as changed
            QDateTime m_timestampLastReadBookedStations; //!< stations read
            QDateTime m_timestampBookedStationsChanged;  //!< stations marked as changed
            BlackMisc::CSettingReadOnly<BlackGui::Settings::TViewUpdateSettings>  m_settingsView { this, &CAtcStationComponent::settingsChanged };
            BlackMisc::CSettingReadOnly<BlackGui::Settings::TAtcStationsSettings> m_settingsAtc { this, &CAtcStationComponent::settingsChanged };
        };
    } // namespace
} // namespace
#endif // guard
