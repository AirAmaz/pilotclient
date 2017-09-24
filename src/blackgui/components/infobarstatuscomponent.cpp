/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackconfig/buildconfig.h"
#include "blackcore/context/contextapplication.h"
#include "blackcore/context/contextaudio.h"
#include "blackcore/context/contextnetwork.h"
#include "blackcore/context/contextsimulator.h"
#include "blackcore/simulator.h"
#include "blackgui/components/infobarstatuscomponent.h"
#include "blackgui/guiapplication.h"
#include "blackgui/led.h"
#include "blackmisc/audio/audioutils.h"
#include "blackmisc/network/server.h"
#include "blackmisc/simulation/simulatorplugininfo.h"
#include "ui_infobarstatuscomponent.h"

#include <QLabel>
#include <QList>
#include <QMenu>
#include <QPoint>
#include <QWidget>
#include <Qt>
#include <QtGlobal>

using namespace BlackConfig;
using namespace BlackCore;
using namespace BlackCore::Context;
using namespace BlackGui;
using namespace BlackMisc;

namespace BlackGui
{
    namespace Components
    {
        CInfoBarStatusComponent::CInfoBarStatusComponent(QWidget *parent) :
            QFrame(parent), ui(new Ui::CInfoBarStatusComponent)
        {
            ui->setupUi(this);
            this->initLeds();

            ui->lbl_Audio->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(ui->lbl_Audio, &QLabel::customContextMenuRequested, this, &CInfoBarStatusComponent::ps_customAudioContextMenuRequested);

            if (sGui->getIContextSimulator())
            {
                connect(sGui->getIContextSimulator(), &IContextSimulator::simulatorStatusChanged, this, &CInfoBarStatusComponent::ps_onSimulatorStatusChanged);
                connect(sGui->getIContextSimulator(), &IContextSimulator::modelSetChanged, this, &CInfoBarStatusComponent::ps_onMapperReady);

                // initial values
                this->ps_onMapperReady();
                this->ps_onSimulatorStatusChanged(sGui->getIContextSimulator()->getSimulatorStatus());
            }

            if (sGui->getIContextNetwork())
            {
                connect(sGui->getIContextNetwork(), &IContextNetwork::connectionStatusChanged, this, &CInfoBarStatusComponent::ps_onNetworkConnectionChanged);
            }

            if (sGui->getIContextApplication())
            {
                ui->led_DBus->setOn(sGui->getIContextApplication()->isUsingImplementingObject());
            }

            if (sGui->getIContextAudio())
            {
                ui->led_Audio->setOn(!sGui->getIContextAudio()->isMuted());
                connect(sGui->getIContextAudio(), &IContextAudio::changedMute, this, &CInfoBarStatusComponent::ps_onMuteChanged);
            }
        }

        CInfoBarStatusComponent::~CInfoBarStatusComponent()
        { }

        void CInfoBarStatusComponent::initLeds()
        {
            CLedWidget::LedShape shape = CLedWidget::Circle;
            ui->led_DBus->setValues(CLedWidget::Yellow, CLedWidget::Black, shape, "DBus connected", "DBus disconnected", 14);
            ui->led_Network->setValues(CLedWidget::Yellow, CLedWidget::Black, shape, "Network connected", "Network disconnected", 14);
            ui->led_Simulator->setValues(CLedWidget::Yellow, CLedWidget::Black, CLedWidget::Blue, shape, "Simulator running", "Simulator disconnected", "Simulator connected", 14);
            ui->led_MapperReady->setValues(CLedWidget::Yellow, CLedWidget::Black, CLedWidget::Blue, shape, "Mapper ready", "Mappings loading", "Mappings loading", 14);

            shape = CLedWidget::Rounded;
            ui->led_Ptt->setValues(CLedWidget::Yellow, CLedWidget::Black, shape, "Ptt", "Silence", 18);
            ui->led_Audio->setValues(CLedWidget::Yellow, CLedWidget::Black, shape, "On", "Muted", 18);
        }

        void CInfoBarStatusComponent::setDBusStatus(bool dbus)
        {
            ui->led_DBus->setOn(dbus);
        }

        void CInfoBarStatusComponent::setDBusTooltip(const QString &tooltip)
        {
            ui->led_DBus->setOnToolTip(tooltip);
        }

        void CInfoBarStatusComponent::ps_onSimulatorStatusChanged(int status)
        {
            ISimulator::SimulatorStatus simStatus = static_cast<ISimulator::SimulatorStatus>(status);
            if (simStatus.testFlag(ISimulator::Connected))
            {
                // at least connected
                const QString s(
                    sGui->getIContextSimulator()->getSimulatorPluginInfo().getDescription() + ": " +
                    ISimulator::statusToString(simStatus)
                );


                if (simStatus.testFlag(ISimulator::Paused))
                {
                    ui->led_Simulator->setTriState();
                    ui->led_Simulator->setTriStateToolTip(s);
                }
                else if (simStatus.testFlag(ISimulator::Simulating))
                {
                    ui->led_Simulator->setOn(true);
                    ui->led_Simulator->setOnToolTip(s);
                }
                else
                {
                    // connected only
                    ui->led_Simulator->setTriState();
                    ui->led_Simulator->setTriStateToolTip(s);
                }
            }
            else
            {
                ui->led_Simulator->setOn(false);
            }

            // simulator status has impact on model set available
            this->ps_onMapperReady();
        }

        void CInfoBarStatusComponent::ps_onNetworkConnectionChanged(BlackCore::INetwork::ConnectionStatus from, BlackCore::INetwork::ConnectionStatus to)
        {
            Q_UNUSED(from);

            switch (to)
            {
            case INetwork::Disconnected:
            case INetwork::DisconnectedError:
            case INetwork::DisconnectedFailed:
            case INetwork::DisconnectedLost:
                ui->led_Network->setOn(false);
                break;
            case INetwork::Connected:
                ui->led_Network->setOn(true);
                ui->led_Network->setOnToolTip("Connected: " + sGui->getIContextNetwork()->getConnectedServer().getName());
                break;
            case INetwork::Connecting:
                ui->led_Network->setTriStateColor(CLedWidget::Yellow);
                break;
            default:
                ui->led_Network->setOn(false);
                break;
            }
        }

        void CInfoBarStatusComponent::ps_customAudioContextMenuRequested(const QPoint &position)
        {
            QWidget *sender = qobject_cast<QWidget *>(QWidget::sender());
            Q_ASSERT(sender);
            QPoint globalPosition = sender->mapToGlobal(position);

            QMenu menuAudio(this);
            menuAudio.addAction("Toogle mute");

            if (CBuildConfig::isRunningOnWindowsNtPlatform())
            {
                menuAudio.addAction("Mixer");
            }

            QAction *selectedItem = menuAudio.exec(globalPosition);
            if (selectedItem)
            {
                // http://forum.technical-assistance.co.uk/sndvol32exe-command-line-parameters-vt1348.html
                const QList<QAction *> actions = menuAudio.actions();
                if (selectedItem == actions.at(0))
                {
                    sGui->getIContextAudio()->setMute(!sGui->getIContextAudio()->isMuted());
                }
                else if (actions.size() > 1 && selectedItem == actions.at(1))
                {
                    BlackMisc::Audio::startWindowsMixer();
                }
            }
        }

        void CInfoBarStatusComponent::ps_onMuteChanged(bool muted)
        {
            ui->led_Audio->setOn(!muted);
        }

        void CInfoBarStatusComponent::ps_onMapperReady()
        {
            if (!sGui || !sGui->getIContextSimulator())
            {
                ui->led_MapperReady->setOn(false);
                return;
            }

            const int models = sGui->getIContextSimulator()->getModelSetCount();
            const bool on = (models > 0);
            ui->led_MapperReady->setOn(on);
            if (on)
            {
                QString m = QString("Mapper with %1 models").arg(models);
                ui->led_MapperReady->setToolTip(m);
            }
        }

        void CInfoBarStatusComponent::ps_onPttChanged(bool enabled)
        {
            ui->led_Ptt->setOn(enabled);
        }
    } // namespace
} // namespace
