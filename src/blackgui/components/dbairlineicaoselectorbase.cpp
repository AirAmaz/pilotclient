/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "dbairlineicaoselectorbase.h"
#include "blackgui/guiapplication.h"
#include "blackmisc/datastoreutility.h"
#include <QMimeData>
#include <QStringList>

using namespace BlackGui;
using namespace BlackCore;
using namespace BlackMisc;
using namespace BlackMisc::Network;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Network;

namespace BlackGui
{
    namespace Components
    {
        CDbAirlineIcaoSelectorBase::CDbAirlineIcaoSelectorBase(QWidget *parent) :
            QFrame(parent)
        {
            this->setAcceptDrops(true);
            this->setAcceptedMetaTypeIds({qMetaTypeId<CAirlineIcaoCode>(), qMetaTypeId<CAirlineIcaoCodeList>()});

            connect(sGui->getWebDataServices(), &CWebDataServices::dataRead, this, &CDbAirlineIcaoSelectorBase::ps_codesRead);
            this->ps_codesRead(CEntityFlags::AirlineIcaoEntity, CEntityFlags::ReadFinished, sGui->getWebDataServices()->getAirlineIcaoCodesCount());
        }

        CDbAirlineIcaoSelectorBase::~CDbAirlineIcaoSelectorBase()
        { }

        bool CDbAirlineIcaoSelectorBase::setAirlineIcao(const CAirlineIcaoCode &icao)
        {
            if (icao == m_currentIcao) { return false; }
            m_currentIcao = icao;
            emit changedAirlineIcao(icao);
            return true;
        }

        bool CDbAirlineIcaoSelectorBase::setAirlineIcao(int key)
        {
            CAirlineIcaoCode icao(sGui->getWebDataServices()->getAirlineIcaoCodeForDbKey(key));
            if (icao.hasCompleteData())
            {
                this->setAirlineIcao(icao);
                return true;
            }
            else
            {
                return false;
            }
        }

        void CDbAirlineIcaoSelectorBase::dragEnterEvent(QDragEnterEvent *event)
        {
            if (!event || !acceptDrop(event->mimeData())) { return; }
            setBackgroundRole(QPalette::Highlight);
            event->acceptProposedAction();
        }

        void CDbAirlineIcaoSelectorBase::dragMoveEvent(QDragMoveEvent *event)
        {
            if (!event || !acceptDrop(event->mimeData())) { return; }
            event->acceptProposedAction();
        }

        void CDbAirlineIcaoSelectorBase::dragLeaveEvent(QDragLeaveEvent *event)
        {
            if (!event) { return; }
            event->accept();
        }

        void CDbAirlineIcaoSelectorBase::dropEvent(QDropEvent *event)
        {
            if (!event || !acceptDrop(event->mimeData())) { return; }
            CVariant valueVariant(toCVariant(event->mimeData()));
            if (valueVariant.isValid())
            {
                if (valueVariant.canConvert<CAirlineIcaoCode>())
                {
                    CAirlineIcaoCode icao(valueVariant.value<CAirlineIcaoCode>());
                    if (!icao.hasValidDbKey()) { return; }
                    this->setAirlineIcao(icao);
                }
                else if (valueVariant.canConvert<CAirlineIcaoCodeList>())
                {
                    CAirlineIcaoCodeList icaos(valueVariant.value<CAirlineIcaoCodeList>());
                    if (icaos.isEmpty()) { return; }
                    this->setAirlineIcao(icaos.front());
                }
            }
        }

        void CDbAirlineIcaoSelectorBase::ps_codesRead(CEntityFlags::Entity entity, CEntityFlags::ReadState readState, int count)
        {
            if (!sGui) { return; }
            if (entity.testFlag(CEntityFlags::AirlineIcaoEntity) && readState == CEntityFlags::ReadFinished)
            {
                if (count > 0)
                {
                    QCompleter *c = this->createCompleter();
                    Q_ASSERT_X(c, Q_FUNC_INFO, "missing converter");
                    this->connect(c, static_cast<void (QCompleter::*)(const QString &)>(&QCompleter::activated), this, &CDbAirlineIcaoSelectorBase::ps_completerActivated);
                    m_completer.reset(c); // deletes any old completer
                }
                else
                {
                    this->m_completer.reset(nullptr);
                }
            }
        }

        void CDbAirlineIcaoSelectorBase::ps_completerActivated(const QString &icaoString)
        {
            int dbKey = CDatastoreUtility::extractIntegerKey(icaoString);
            if (dbKey < 0) { return; }
            this->setAirlineIcao(dbKey);
        }

    }// class
} // ns
