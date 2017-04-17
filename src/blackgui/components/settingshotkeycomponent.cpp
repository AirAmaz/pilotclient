/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackgui/components/configurationwizard.h"
#include "blackgui/components/settingshotkeycomponent.h"
#include "blackgui/components/hotkeydialog.h"
#include "blackgui/guiapplication.h"
#include "blackcore/context/contextapplication.h"
#include "blackcore/context/contextaudio.h"
#include "blackcore/inputmanager.h"
#include "ui_settingshotkeycomponent.h"

#include <QAbstractItemModel>
#include <QFlags>
#include <QItemSelectionModel>
#include <QList>
#include <QMessageBox>
#include <QModelIndex>
#include <QModelIndexList>
#include <QPushButton>
#include <QString>
#include <QTableView>
#include <QVariant>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackMisc::Input;
using namespace BlackGui::Models;
using namespace BlackCore;
using namespace BlackCore::Context;

namespace BlackGui
{
    namespace Components
    {
        CSettingsHotkeyComponent::CSettingsHotkeyComponent(QWidget *parent) :
            QFrame(parent),
            ui(new Ui::CSettingsHotkeyComponent)
        {
            ui->setupUi(this);
            ui->tv_Hotkeys->setModel(&m_model);

            connect(ui->pb_AddHotkey, &QPushButton::clicked, this, &CSettingsHotkeyComponent::ps_addEntry);
            connect(ui->pb_EditHotkey, &QPushButton::clicked, this, &CSettingsHotkeyComponent::ps_editEntry);
            connect(ui->pb_RemoveHotkey, &QPushButton::clicked, this, &CSettingsHotkeyComponent::ps_removeEntry);

            reloadHotkeysFromSettings();
            ui->tv_Hotkeys->selectRow(0);
        }

        CSettingsHotkeyComponent::~CSettingsHotkeyComponent()
        { }

        void CSettingsHotkeyComponent::saveSettings()
        {
            const CStatusMessage msg = m_actionHotkeys.save();
            CLogMessage(this).preformatted(msg);
        }

        void CSettingsHotkeyComponent::registerDummyPttEntry()
        {
            CInputManager::instance()->registerAction(IContextAudio::pttHotkeyAction(), IContextAudio::pttHotkeyIcon());
        }

        void CSettingsHotkeyComponent::ps_addEntry()
        {
            BlackMisc::CIdentifierList registeredApps;
            if (sGui->getIContextApplication()) registeredApps = sGui->getIContextApplication()->getRegisteredApplications();

            // add local application
            registeredApps.push_back(CIdentifier());
            const auto selectedActionHotkey = CHotkeyDialog::getActionHotkey(CActionHotkey(), registeredApps, this);
            if (selectedActionHotkey.isValid() && checkAndConfirmConflicts(selectedActionHotkey))
            {
                addHotkeytoSettings(selectedActionHotkey);
                int position = m_model.rowCount();
                m_model.insertRows(position, 1, QModelIndex());
                QModelIndex index = m_model.index(position, 0, QModelIndex());
                m_model.setData(index, QVariant::fromValue(selectedActionHotkey), CActionHotkeyListModel::ActionHotkeyRole);
            }
        }

        void CSettingsHotkeyComponent::ps_editEntry()
        {
            const auto index = ui->tv_Hotkeys->selectionModel()->currentIndex();
            if (!index.isValid()) return;

            const auto model = ui->tv_Hotkeys->model();
            const QModelIndex indexHotkey = model->index(index.row(), 0, QModelIndex());
            Q_ASSERT_X(indexHotkey.data(CActionHotkeyListModel::ActionHotkeyRole).canConvert<CActionHotkey>(), Q_FUNC_INFO, "No action hotkey");
            CActionHotkey actionHotkey = indexHotkey.data(CActionHotkeyListModel::ActionHotkeyRole).value<CActionHotkey>();
            BlackMisc::CIdentifierList registeredApps;
            Q_ASSERT_X(sGui, Q_FUNC_INFO, "Missing sGui");
            if (sGui->getIContextApplication()) registeredApps = sGui->getIContextApplication()->getRegisteredApplications();

            // add local application
            registeredApps.push_back(CIdentifier());
            const auto selectedActionHotkey = CHotkeyDialog::getActionHotkey(actionHotkey, registeredApps, this);
            if (selectedActionHotkey.isValid() && checkAndConfirmConflicts(selectedActionHotkey, { actionHotkey }))
            {
                updateHotkeyInSettings(actionHotkey, selectedActionHotkey);
                m_model.setData(indexHotkey, QVariant::fromValue(selectedActionHotkey), CActionHotkeyListModel::ActionHotkeyRole);
            }
        }

        void CSettingsHotkeyComponent::ps_removeEntry()
        {
            const QModelIndexList indexes = ui->tv_Hotkeys->selectionModel()->selectedRows();
            for (const auto &index : indexes)
            {
                CActionHotkey actionHotkey = index.data(CActionHotkeyListModel::ActionHotkeyRole).value<CActionHotkey>();
                removeHotkeyFromSettings(actionHotkey);
                m_model.removeRows(index.row(), 1, QModelIndex());
            }
        }

        void CSettingsHotkeyComponent::addHotkeytoSettings(const CActionHotkey &actionHotkey)
        {
            CActionHotkeyList actionHotkeyList(m_actionHotkeys.getThreadLocal());
            actionHotkeyList.push_back(actionHotkey);
            m_actionHotkeys.set(actionHotkeyList);
        }

        void CSettingsHotkeyComponent::updateHotkeyInSettings(const CActionHotkey &oldValue, const CActionHotkey &newValue)
        {
            CActionHotkeyList actionHotkeyList(m_actionHotkeys.getThreadLocal());
            actionHotkeyList.replace(oldValue, newValue);
            m_actionHotkeys.set(actionHotkeyList);
        }

        void CSettingsHotkeyComponent::removeHotkeyFromSettings(const CActionHotkey &actionHotkey)
        {
            CActionHotkeyList actionHotkeyList(m_actionHotkeys.getThreadLocal());
            actionHotkeyList.remove(actionHotkey);
            m_actionHotkeys.set(actionHotkeyList);
        }

        bool CSettingsHotkeyComponent::checkAndConfirmConflicts(const CActionHotkey &actionHotkey, const CActionHotkeyList &ignore)
        {
            const auto configuredHotkeys = m_actionHotkeys.getThreadLocal();
            CActionHotkeyList conflicts = configuredHotkeys.findSupersetsOf(actionHotkey);
            conflicts.push_back(configuredHotkeys.findSubsetsOf(actionHotkey));
            conflicts.removeIfIn(ignore);

            if (!conflicts.isEmpty())
            {
                QString message = QString("The selected combination conflicts with the following %1 combinations(s):\n\n").arg(conflicts.size());
                for (const auto &conflict : conflicts)
                {
                    message += conflict.getCombination().toQString();
                    message += "\n";
                }
                message += "\n Do you want to use it anway?";
                const auto reply = QMessageBox::warning(this, "SettingsHotkeyComponent", message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (reply == QMessageBox::No) { return false; }
            }
            return true;
        }

        void CSettingsHotkeyComponent::reloadHotkeysFromSettings()
        {
            const CActionHotkeyList hotkeys = m_actionHotkeys.getThreadLocal();
            m_model.clear();

            // list of all defined hotkeys (not the dialog)
            for (const CActionHotkey &hotkey : hotkeys)
            {
                const int position = m_model.rowCount();
                m_model.insertRows(position, 1, QModelIndex());
                const QModelIndex index = m_model.index(position, 0, QModelIndex());
                m_model.setData(index, QVariant::fromValue(hotkey), CActionHotkeyListModel::ActionHotkeyRole);
            }
        }

        void CSettingsHotkeyComponent::ps_hotkeySlot(bool keyDown)
        {
            if (keyDown) { QMessageBox::information(this, "Test", "Hotkey test"); }
        }

        bool CConfigHotkeyWizardPage::validatePage()
        {
            if (CConfigurationWizard::lastWizardStepSkipped(this->wizard())) { return true; }
            m_config->saveSettings();
            return true;
        }
    } // ns
} // ns
