/* Copyright (C) 2016
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "simulatorsettings.h"
#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/simulation/fscommon/fscommonutil.h"
#include "blackmisc/simulation/xplane/xplaneutil.h"
#include "blackmisc/stringutils.h"

using namespace BlackMisc;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Simulation::FsCommon;
using namespace BlackMisc::Simulation::XPlane;

namespace BlackMisc
{
    namespace Simulation
    {
        CSimulatorSettings::CSimulatorSettings()
        { }

        void CSimulatorSettings::setSimulatorDirectory(const QString &simulatorDirectory)
        {
            this->m_simulatorDirectory = simulatorDirectory.trimmed();
        }

        const QString &CSimulatorSettings::getSimulatorDirectory() const
        {
            return this->m_simulatorDirectory;
        }

        void CSimulatorSettings::setModelDirectories(const QStringList &modelDirectories)
        {
            this->m_modelDirectories = modelDirectories;
        }

        void CSimulatorSettings::setModelDirectory(const QString &modelDirectory)
        {
            this->m_modelDirectories = QStringList({ modelDirectory });
        }

        const QStringList &CSimulatorSettings::getModelDirectories() const
        {
            return this->m_modelDirectories;
        }

        void CSimulatorSettings::setModelExcludeDirectories(const QStringList &excludeDirectories)
        {
            this->m_excludeDirectoryPatterns = excludeDirectories;
        }

        const QStringList &CSimulatorSettings::getModelExcludeDirectoryPatterns() const
        {
            return m_excludeDirectoryPatterns;
        }

        void CSimulatorSettings::resetPaths()
        {
            this->m_excludeDirectoryPatterns.clear();
            this->m_modelDirectories.clear();
            this->m_simulatorDirectory.clear();
        }

        QString CSimulatorSettings::convertToQString(bool i18n) const
        {
            return convertToQString(", ", i18n);
        }

        QString CSimulatorSettings::convertToQString(const QString &separator, bool i18n) const
        {
            Q_UNUSED(i18n);
            QString s("model directories: ");
            s.append(this->m_modelDirectories.join(','));
            s.append(separator);
            s.append("exclude directories: ");
            s.append(this->m_excludeDirectoryPatterns.join(','));
            return s;
        }

        CVariant CSimulatorSettings::propertyByIndex(const CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexSimulatorDirectory:
                return CVariant::fromValue(this->m_simulatorDirectory);
            case IndexModelDirectory:
                return CVariant::fromValue(this->m_modelDirectories);
            case IndexModelExcludeDirectoryPatterns:
                return CVariant::fromValue(this->m_excludeDirectoryPatterns);
            default:
                return CValueObject::propertyByIndex(index);
            }
        }

        void CSimulatorSettings::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CSimulatorSettings>(); return; }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexSimulatorDirectory:
                this->setSimulatorDirectory(variant.toQString());
                break;
            case IndexModelDirectory:
                this->setSimulatorDirectory(variant.toQString());
                break;
            case IndexModelExcludeDirectoryPatterns:
                this->m_excludeDirectoryPatterns = variant.value<QStringList>();
                break;
            default:
                CValueObject::setPropertyByIndex(index, variant);
                break;
            }
        }

        CMultiSimulatorSettings::CMultiSimulatorSettings(QObject *parent) : QObject(parent)
        {
            // void
        }

        CSimulatorSettings CMultiSimulatorSettings::getSettings(const CSimulatorInfo &simulator) const
        {
            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
            switch (simulator.getSimulator())
            {
            case CSimulatorInfo::FS9: return this->m_simSettingsFs9.get();
            case CSimulatorInfo::FSX: return this->m_simSettingsFsx.get();
            case CSimulatorInfo::P3D: return this->m_simSettingsP3D.get();
            case CSimulatorInfo::XPLANE: return this->m_simSettingsXP.get();
            default:
                Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
                break;
            }
            return CSimulatorSettings();
        }

        CStatusMessage CMultiSimulatorSettings::setSettings(const CSimulatorSettings &settings, const CSimulatorInfo &simulator)
        {
            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
            switch (simulator.getSimulator())
            {
            case CSimulatorInfo::FS9: return this->m_simSettingsFs9.set(settings);
            case CSimulatorInfo::FSX: return this->m_simSettingsFsx.set(settings);
            case CSimulatorInfo::P3D: return this->m_simSettingsP3D.set(settings);
            case CSimulatorInfo::XPLANE: return this->m_simSettingsXP.set(settings);
            default:
                Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
                break;
            }
            return CStatusMessage({ CLogCategory::settings() }, CStatusMessage::SeverityError, "wrong simulator");
        }

        CStatusMessage CMultiSimulatorSettings::setAndSaveSettings(const CSimulatorSettings &settings, const CSimulatorInfo &simulator)
        {
            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
            switch (simulator.getSimulator())
            {
            case CSimulatorInfo::FS9: return this->m_simSettingsFs9.setAndSave(settings);
            case CSimulatorInfo::FSX: return this->m_simSettingsFsx.setAndSave(settings);
            case CSimulatorInfo::P3D: return this->m_simSettingsP3D.setAndSave(settings);
            case CSimulatorInfo::XPLANE: return this->m_simSettingsXP.setAndSave(settings);
            default:
                Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
                break;
            }
            return CStatusMessage({ CLogCategory::settings() }, CStatusMessage::SeverityError, "wrong simulator");
        }

        CStatusMessage CMultiSimulatorSettings::saveSettings(const CSimulatorInfo &simulator)
        {
            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
            switch (simulator.getSimulator())
            {
            case CSimulatorInfo::FS9: return this->m_simSettingsFs9.save();
            case CSimulatorInfo::FSX: return this->m_simSettingsFsx.save();
            case CSimulatorInfo::P3D: return this->m_simSettingsP3D.save();
            case CSimulatorInfo::XPLANE: return this->m_simSettingsXP.save();
            default:
                Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
                break;
            }
            return CStatusMessage({ CLogCategory::settings() }, CStatusMessage::SeverityError, "wrong simulator");
        }

        QString CMultiSimulatorSettings::getSimulatorDirectoryOrDefault(const CSimulatorInfo &simulator) const
        {
            const CSimulatorSettings s = this->getSettings(simulator);
            if (s.getSimulatorDirectory().isEmpty())
            {
                return this->getDefaultSimulatorDirectory(simulator);
            }
            return s.getSimulatorDirectory();
        }

        QString CMultiSimulatorSettings::getDefaultSimulatorDirectory(const CSimulatorInfo &simulator) const
        {
            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
            switch (simulator.getSimulator())
            {
            case CSimulatorInfo::FS9: return CFsCommonUtil::fs9Dir();
            case CSimulatorInfo::FSX: return CFsCommonUtil::fsxDir();
            case CSimulatorInfo::P3D: return CFsCommonUtil::p3dDir();
            case CSimulatorInfo::XPLANE: return CXPlaneUtil::xplaneRootDir(); //! check XP
            default:
                Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
                break;
            }
            return "";
        }

        QStringList CMultiSimulatorSettings::getModelDirectoriesOrDefault(const CSimulatorInfo &simulator) const
        {
            const CSimulatorSettings s = this->getSettings(simulator);
            if (s.getModelDirectories().isEmpty())
            {
                return this->getDefaultModelDirectories(simulator);
            }
            return s.getModelDirectories();
        }

        QString CMultiSimulatorSettings::getFirstModelDirectoryOrDefault(const CSimulatorInfo &simulator) const
        {
            const QStringList models(getModelDirectoriesOrDefault(simulator));
            if (models.isEmpty()) { return ""; }
            return models.first();
        }

        QStringList CMultiSimulatorSettings::getDefaultModelDirectories(const CSimulatorInfo &simulator) const
        {
            static const QStringList e;
            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
            switch (simulator.getSimulator())
            {
            case CSimulatorInfo::FS9: return CFsCommonUtil::fs9AircraftDir().isEmpty()   ? e : QStringList({ CFsCommonUtil::fs9AircraftDir() });
            case CSimulatorInfo::FSX: return CFsCommonUtil::fsxSimObjectsDir().isEmpty() ? e : QStringList({ CFsCommonUtil::fsxSimObjectsDir() });
            case CSimulatorInfo::P3D: return CFsCommonUtil::p3dSimObjectsDir().isEmpty() ? e : QStringList({ CFsCommonUtil::p3dSimObjectsDir()});
            case CSimulatorInfo::XPLANE: return CXPlaneUtil::xplaneModelDirectories();
            default:
                Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
                break;
            }
            return QStringList();
        }

        QStringList CMultiSimulatorSettings::getModelExcludeDirectoryPatternsOrDefault(const CSimulatorInfo &simulator) const
        {
            const CSimulatorSettings s = this->getSettings(simulator);
            QStringList exclude(s.getModelExcludeDirectoryPatterns());
            if (!exclude.isEmpty()) { return exclude; }
            exclude = this->getDefaultModelExcludeDirectoryPatterns(simulator);
            return exclude;
        }

        QStringList CMultiSimulatorSettings::getDefaultModelExcludeDirectoryPatterns(const CSimulatorInfo &simulator) const
        {
            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
            switch (simulator.getSimulator())
            {
            case CSimulatorInfo::FS9: return CFsCommonUtil::fs9AircraftObjectsExcludeDirectoryPatterns();
            case CSimulatorInfo::FSX: return CFsCommonUtil::fsxSimObjectsExcludeDirectoryPatterns();
            case CSimulatorInfo::P3D: return CFsCommonUtil::p3dSimObjectsExcludeDirectoryPatterns();
            case CSimulatorInfo::XPLANE: return CXPlaneUtil::xplaneModelExcludeDirectoryPatterns();
            default:
                Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "No single simulator");
                break;
            }
            return QStringList();
        }

        void CMultiSimulatorSettings::resetToDefaults(const CSimulatorInfo &simulator)
        {
            CSimulatorSettings s = this->getSettings(simulator);
            s.resetPaths();
            this->setAndSaveSettings(s, simulator);
        }

        CSimulatorMessagesSettings::CSimulatorMessagesSettings()
        {
            // void
        }

        void CSimulatorMessagesSettings::setTechnicalLogSeverity(CStatusMessage::StatusSeverity severity)
        {
            this->m_technicalLogLevel = static_cast<int>(severity);
        }

        void CSimulatorMessagesSettings::disableTechnicalMessages()
        {
            this->m_technicalLogLevel = -1;
        }

        bool CSimulatorMessagesSettings::isRelayedErrorsMessages() const
        {
            if (this->m_technicalLogLevel < 0) { return false; }
            return (this->m_technicalLogLevel <= CStatusMessage::SeverityError);
        }

        bool CSimulatorMessagesSettings::isRelayedWarningMessages() const
        {
            if (this->m_technicalLogLevel < 0) { return false; }
            return (this->m_technicalLogLevel <= CStatusMessage::SeverityWarning);
        }

        bool CSimulatorMessagesSettings::isRelayedInfoMessages() const
        {
            if (this->m_technicalLogLevel < 0) { return false; }
            return (this->m_technicalLogLevel <= CStatusMessage::SeverityInfo);
        }

        bool CSimulatorMessagesSettings::isRelayedTechnicalMessages() const
        {
            return (this->m_technicalLogLevel >= 0);
        }

        void CSimulatorMessagesSettings::setRelayedTextMessages(CSimulatorMessagesSettings::TextMessageType messageType)
        {
            this->m_messageType = static_cast<int>(messageType);
        }

        bool CSimulatorMessagesSettings::isRelayedSupervisorTextMessages() const
        {
            return this->getRelayedTextMessageTypes().testFlag(TextMessageSupervisor);
        }

        bool CSimulatorMessagesSettings::isRelayedPrivateTextMessages() const
        {
            return this->getRelayedTextMessageTypes().testFlag(TextMessagePrivate);
        }

        bool CSimulatorMessagesSettings::isRelayedUnicomTextMessages() const
        {
            return this->getRelayedTextMessageTypes().testFlag(TextMessagesUnicom);
        }

        bool CSimulatorMessagesSettings::isRelayedCom1TextMessages() const
        {
            return this->getRelayedTextMessageTypes().testFlag(TextMessagesCom1);
        }

        bool CSimulatorMessagesSettings::isRelayedCom2TextMessages() const
        {
            return this->getRelayedTextMessageTypes().testFlag(TextMessagesCom2);
        }

        bool CSimulatorMessagesSettings::relayThisStatusMessage(const CStatusMessage &message) const
        {
            if (message.isEmpty()) { return false; }
            if (!this->isGloballyEnabled()) { return false; }
            if (!this->isRelayedTechnicalMessages()) { return false; }
            int s = static_cast<int>(message.getSeverity());
            return (s >= this->m_technicalLogLevel);
        }

        bool CSimulatorMessagesSettings::relayThisTextMessage(const Network::CTextMessage &msg, const BlackMisc::Simulation::CSimulatedAircraft &aircraft) const
        {
            if (msg.isEmpty()) { return false; }
            if (!this->isGloballyEnabled()) { return false; }
            if (this->m_messageType == NoTextMessages) { return false; }

            const TextMessageType mt = static_cast<TextMessageType>(this->m_messageType);
            if (msg.isPrivateMessage() && mt.testFlag(TextMessagePrivate)) { return true; }
            if (msg.isSupervisorMessage() && (mt.testFlag(TextMessagePrivate) || mt.testFlag(TextMessageSupervisor))) { return true; }
            if (msg.isSendToUnicom() && mt.testFlag(TextMessagesUnicom)) { return true; }

            if (msg.isRadioMessage())
            {
                const CFrequency f(msg.getFrequency());
                if (mt.testFlag(TextMessagesCom1))
                {
                    if (aircraft.getCom1System().isActiveFrequencyWithin8_33kHzChannel(f)) { return true; }
                }
                if (mt.testFlag(TextMessagesCom2))
                {
                    if (aircraft.getCom2System().isActiveFrequencyWithin8_33kHzChannel(f)) { return true; }
                }
            }
            return false;
        }

        CSimulatorMessagesSettings::TextMessageType CSimulatorMessagesSettings::getRelayedTextMessageTypes() const
        {
            return static_cast<CSimulatorMessagesSettings::TextMessageType>(this->m_messageType);
        }

        QString CSimulatorMessagesSettings::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n);
            QString s("Enabled %1, text messages: %2, severity: %3");
            QString severity;
            if (this->isRelayedTechnicalMessages())
            {
                severity = "No tech. msgs";
            }
            else
            {
                severity = CStatusMessage::severityToString(static_cast<CStatusMessage::StatusSeverity>(this->m_technicalLogLevel));
            }
            return s.arg(boolToOnOff(this->m_globallyEnabled)).arg(this->m_messageType).arg(severity);
        }

        CVariant CSimulatorMessagesSettings::propertyByIndex(const CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexTechnicalLogSeverity:
                return CVariant::fromValue(this->m_technicalLogLevel);
            case IndexTextMessageRelay:
                return CVariant::from(this->m_messageType);
            case IndexGloballyEnabled:
                return CVariant::from(this->m_globallyEnabled);
            default:
                return CValueObject::propertyByIndex(index);
            }
        }

        void CSimulatorMessagesSettings::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CSimulatorMessagesSettings>(); return; }
            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexTechnicalLogSeverity:
                this->setTechnicalLogSeverity(static_cast<CStatusMessage::StatusSeverity>(variant.toInt()));
                break;
            case IndexTextMessageRelay:
                this->setRelayedTextMessages(static_cast<CSimulatorMessagesSettings::TextMessageType>(variant.toInt()));
                break;
            case IndexGloballyEnabled:
                this->setGloballyEnabled(variant.toBool());
                break;
            default:
                CValueObject::setPropertyByIndex(index, variant);
                break;
            }
        }
    } // ns
} // ns
