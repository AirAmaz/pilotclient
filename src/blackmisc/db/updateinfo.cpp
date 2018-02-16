/* Copyright (C) 2017
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "updateinfo.h"
#include "blackconfig/buildconfig.h"
#include <QStringBuilder>

using namespace BlackConfig;

namespace BlackMisc
{
    namespace Db
    {
        CUpdateInfo::CUpdateInfo(const CArtifactList &artifacts, const CDistributionList &distributions) :
            m_artifactsPilotClient(artifacts.findByType(CArtifact::PilotClientInstaller)),
            m_artifactsXSwiftBus(artifacts.findByType(CArtifact::XSwiftBus)),
            m_distributions(distributions)
        {
            // void
        }

        CDistributionList CUpdateInfo::getDistributionsPilotClientForCurrentPlatform() const
        {
            const CArtifactList artifacts = this->getArtifactsPilotClient().findMatchingForCurrentPlatform();
            CDistributionList distributions = artifacts.getDistributions();
            distributions.sortByStability();
            return distributions;
        }

        CArtifactList CUpdateInfo::getArtifactsPilotClientForCurrentPlatform() const
        {
            CArtifactList artifacts = m_artifactsPilotClient.findMatchingForCurrentPlatform();
            artifacts.sortByVersion(Qt::DescendingOrder);
            return artifacts;
        }

        CArtifactList CUpdateInfo::getArtifactsXSwiftBusLatestVersionFirst() const
        {
            CArtifactList artifacts(m_artifactsXSwiftBus);
            artifacts.sortByVersion(Qt::DescendingOrder);
            return artifacts;
        }

        CArtifactList CUpdateInfo::getArtifactsXSwiftBusOldestVersionFirst() const
        {
            CArtifactList artifacts(m_artifactsXSwiftBus);
            artifacts.sortByVersion(Qt::DescendingOrder);
            return artifacts;
        }

        CArtifactList CUpdateInfo::getArtifactsXSwiftBusForCurrentPlatform() const
        {
            CArtifactList artifacts = m_artifactsXSwiftBus.findMatchingForCurrentPlatform();
            artifacts.sortByVersion(Qt::DescendingOrder);
            return artifacts;
        }

        CDistribution CUpdateInfo::anticipateOwnDistribution() const
        {
            if (this->isEmpty()) { return CDistribution(); }
            const CArtifactList ownArtifacts = this->getArtifactsPilotClientForCurrentPlatform();
            if (ownArtifacts.isEmpty()) { return CDistribution(); }

            const QVersionNumber myVersion = CBuildConfig::getVersion();
            const QVersionNumber latestVersion = ownArtifacts.getLatestQVersion();
            if (myVersion > latestVersion)
            {
                // dev. version
                return ownArtifacts.getDistributions().getLeastStableOrDefault();
            }

            const CArtifact exactVersion = ownArtifacts.findFirstByVersionOrDefault(myVersion);
            if (!exactVersion.isUnknown()) { return exactVersion.getDistributions().getMostStableOrDefault(); }

            return CDistribution::localDeveloperBuild();
        }

        QStringList CUpdateInfo::anticipateMyDefaultChannelAndPlatform() const
        {
            const CArtifactList myArtifacts = this->getArtifactsPilotClient().findMatchingForCurrentPlatform();
            const CDistribution mostStable = myArtifacts.getDistributions().getMostStableOrDefault();
            return QStringList({ mostStable.getClassName(), CPlatform::currentPlatform().getPlatformName() });
        }

        QString CUpdateInfo::convertToQString(bool i18n) const
        {
            return this->convertToQString(", ", i18n);
        }

        QString CUpdateInfo::convertToQString(const QString &separator, bool i18n) const
        {
            Q_UNUSED(i18n);
            return QLatin1String("artifacts (PC): ") %
                   this->getArtifactsPilotClient().toQString(i18n) %
                   separator %
                   QLatin1String("artifacts (XSB): ") %
                   this->getArtifactsXSwiftBus().toQString(i18n) %
                   separator %
                   QLatin1String("distributions: ") %
                   this->getDistributions().toQString(i18n);
        }

        CVariant CUpdateInfo::propertyByIndex(const CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexArtifactsPilotClient: return CVariant::fromValue(m_artifactsPilotClient);
            case IndexArtifactsXSwiftBus: CVariant::fromValue(m_artifactsXSwiftBus);
            case IndexDistributions: return CVariant::fromValue(m_distributions);
            default: return CValueObject::propertyByIndex(index);
            }
        }

        void CUpdateInfo::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CUpdateInfo>(); return; }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexArtifactsPilotClient: m_artifactsPilotClient = variant.value<CArtifactList>(); break;
            case IndexArtifactsXSwiftBus: m_artifactsXSwiftBus = variant.value<CArtifactList>(); break;
            case IndexDistributions: m_distributions = variant.value<CDistributionList>(); break;
            default: CValueObject::setPropertyByIndex(index, variant); break;
            }
        }

        CUpdateInfo CUpdateInfo::fromDatabaseJson(const QJsonObject &json, const QString &prefix)
        {
            Q_UNUSED(prefix); // not nested
            const QJsonArray jsonDistributions = json.value("distributions").toArray();
            const QJsonArray jsonArtifacts = json.value("artifacts").toArray();
            const CDistributionList distributions = CDistributionList::fromDatabaseJson(jsonDistributions);
            const CArtifactList artifacts = CArtifactList::fromDatabaseJson(jsonArtifacts);
            Q_ASSERT_X(jsonDistributions.size() == distributions.size(), Q_FUNC_INFO, "size mismatch");
            Q_ASSERT_X(artifacts.size() == artifacts.size(), Q_FUNC_INFO, "size mismatch");
            return CUpdateInfo(artifacts, distributions);
        }

        CUpdateInfo CUpdateInfo::fromDatabaseJson(const QString &jsonString)
        {
            if (jsonString.isEmpty()) { return CUpdateInfo(); }
            return CUpdateInfo::fromDatabaseJson(Json::jsonObjectFromString(jsonString));
        }
    } // ns
} // ns
