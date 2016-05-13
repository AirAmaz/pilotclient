/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "dbdistributorselectorcomponent.h"
#include "ui_dbdistributorselectorcomponent.h"
#include "blackgui/guiapplication.h"
#include "blackgui/uppercasevalidator.h"
#include "blackcore/webdataservices.h"
#include "blackmisc/simulation/distributorlist.h"
#include <QMimeData>

using namespace BlackGui;
using namespace BlackCore;
using namespace BlackMisc;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Network;

namespace BlackGui
{
    namespace Components
    {
        CDbDistributorSelectorComponent::CDbDistributorSelectorComponent(QWidget *parent) :
            QFrame(parent),
            ui(new Ui::CDbDistributorSelectorComponent)
        {
            Q_ASSERT_X(sGui, Q_FUNC_INFO, "Missing sGui");
            ui->setupUi(this);
            this->setAcceptDrops(true);
            this->setAcceptedMetaTypeIds({qMetaTypeId<CDistributor>(), qMetaTypeId<CDistributorList>()});
            this->ui->le_Distributor->setValidator(new CUpperCaseValidator(this));

            bool c = connect(ui->le_Distributor, &QLineEdit::returnPressed, this, &CDbDistributorSelectorComponent::ps_dataChanged);
            Q_ASSERT_X(c, Q_FUNC_INFO, "Missing connect");
            c = connect(sApp->getWebDataServices(), &CWebDataServices::dataRead, this, &CDbDistributorSelectorComponent::ps_distributorsRead);
            Q_ASSERT_X(c, Q_FUNC_INFO, "Missing connect");
            Q_UNUSED(c);

            this->ps_distributorsRead(CEntityFlags::DistributorEntity, CEntityFlags::ReadFinished, sApp->getWebDataServices()->getDistributorsCount());
        }

        CDbDistributorSelectorComponent::~CDbDistributorSelectorComponent()
        {
            // void
        }

        void CDbDistributorSelectorComponent::setDistributor(const CDistributor &distributor)
        {
            QString key(distributor.getDbKey());
            if (key.isEmpty()) { return; }
            if (distributor != m_currentDistributor)
            {
                this->ui->le_Distributor->setText(key);
                m_currentDistributor = distributor;
                emit changedDistributor(distributor);
            }
        }

        void CDbDistributorSelectorComponent::setDistributor(const QString &distributorKeyOrAlias)
        {
            QString keyOrAlias(distributorKeyOrAlias.toUpper().trimmed());
            if (this->m_currentDistributor.matchesKeyOrAlias(keyOrAlias)) { return; }
            CDistributor d(sGui->getWebDataServices()->getDistributors().findByKeyOrAlias(keyOrAlias));
            if (d.hasCompleteData())
            {
                this->setDistributor(d);
            }
            else
            {
                this->ui->lbl_Description->setText("");
                this->ui->le_Distributor->setText(keyOrAlias);
            }
        }

        CDistributor CDbDistributorSelectorComponent::getDistributor() const
        {
            if (!sGui) { return CDistributor(); }
            QString distributorKeyOrAlias(this->ui->le_Distributor->text().trimmed().toUpper());
            if (distributorKeyOrAlias.isEmpty()) { return CDistributor(); }
            if (this->m_currentDistributor.matchesKeyOrAlias(distributorKeyOrAlias)) { return this->m_currentDistributor; }

            CDistributor d(sGui->getWebDataServices()->getDistributors().findByKey(distributorKeyOrAlias));
            if (d.hasValidDbKey())
            {
                // for some reason we have a new distributor here
                // normally that should not happen
            }
            return d;
        }

        void CDbDistributorSelectorComponent::setReadOnly(bool readOnly)
        {
            this->ui->le_Distributor->setReadOnly(readOnly);
        }

        void CDbDistributorSelectorComponent::withDistributorDescription(bool description)
        {
            this->ui->lbl_Description->setVisible(description);
        }

        bool CDbDistributorSelectorComponent::isSet() const
        {
            return this->getDistributor().hasCompleteData();
        }

        void CDbDistributorSelectorComponent::clear()
        {
            this->ui->le_Distributor->clear();
        }

        void CDbDistributorSelectorComponent::dragEnterEvent(QDragEnterEvent *event)
        {
            if (!event || !acceptDrop(event->mimeData())) { return; }
            setBackgroundRole(QPalette::Highlight);
            event->acceptProposedAction();
        }

        void CDbDistributorSelectorComponent::dragMoveEvent(QDragMoveEvent *event)
        {
            if (!event || !acceptDrop(event->mimeData())) { return; }
            event->acceptProposedAction();
        }

        void CDbDistributorSelectorComponent::dragLeaveEvent(QDragLeaveEvent *event)
        {
            if (!event) { return; }
            event->accept();
        }

        void CDbDistributorSelectorComponent::dropEvent(QDropEvent *event)
        {
            if (!event || !acceptDrop(event->mimeData())) { return; }
            CVariant valueVariant(toCVariant(event->mimeData()));
            if (valueVariant.isValid())
            {
                if (valueVariant.canConvert<CDistributor>())
                {
                    CDistributor distributor(valueVariant.value<CDistributor>());
                    if (!distributor.hasValidDbKey()) { return; }
                    this->setDistributor(distributor);
                }
                else if (valueVariant.canConvert<CDistributorList>())
                {
                    CDistributorList distributors(valueVariant.value<CDistributorList>());
                    if (distributors.isEmpty()) { return; }
                    this->setDistributor(distributors.front());
                }
            }
        }

        void CDbDistributorSelectorComponent::ps_distributorsRead(CEntityFlags::Entity entity, CEntityFlags::ReadState readState, int count)
        {
            if (!sGui) { return; }
            if (entity.testFlag(CEntityFlags::DistributorEntity) && readState == CEntityFlags::ReadFinished)
            {
                if (count > 0)
                {
                    const QStringList keysAndAliases(sGui->getWebDataServices()->getDistributors().getDbKeysAndAliases(true));
                    QCompleter *c = new QCompleter(keysAndAliases, this);
                    c->setCaseSensitivity(Qt::CaseInsensitive);
                    c->setCompletionMode(QCompleter::PopupCompletion);
                    c->setMaxVisibleItems(10);
                    this->connect(c, static_cast<void (QCompleter::*)(const QString &)>(&QCompleter::activated), this, &CDbDistributorSelectorComponent::ps_completerActivated);

                    this->ui->le_Distributor->setCompleter(c);
                    m_completerDistributors.reset(c); // deletes any old completer
                    this->setReadOnly(false);
                }
                else
                {
                    this->m_completerDistributors.reset(nullptr);
                    this->setReadOnly(true);
                }
            }
        }

        void CDbDistributorSelectorComponent::ps_dataChanged()
        {
            if (!sGui) { return; }
            QString keyOrAlias(this->ui->le_Distributor->text().trimmed().toUpper());
            if (keyOrAlias.isEmpty()) { return; }
            CDistributor d(sGui->getWebDataServices()->getDistributors().findByKeyOrAlias(keyOrAlias));
            this->setDistributor(d);
        }

        void CDbDistributorSelectorComponent::ps_completerActivated(const QString &distributorKeyOrAlias)
        {
            this->setDistributor(distributorKeyOrAlias);
        }

    } // ns
} // ns
