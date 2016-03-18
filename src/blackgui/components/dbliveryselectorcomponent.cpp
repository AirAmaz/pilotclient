/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "dbliveryselectorcomponent.h"
#include "ui_dbliveryselectorcomponent.h"
#include "blackgui/uppercasevalidator.h"
#include "blackgui/guiapplication.h"
#include "blackmisc/aviation/liverylist.h"
#include <QDragEnterEvent>

using namespace BlackGui;
using namespace BlackCore;
using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Network;

namespace BlackGui
{
    namespace Components
    {
        CDbLiverySelectorComponent::CDbLiverySelectorComponent(QWidget *parent) :
            QFrame(parent),
            ui(new Ui::CDbLiverySelectorComponent)
        {
            ui->setupUi(this);
            this->setAcceptDrops(true);
            this->setAcceptedMetaTypeIds({qMetaTypeId<CLivery>(), qMetaTypeId<CLiveryList>()});

            ui->le_Livery->setValidator(new CUpperCaseValidator(this));

            connect(ui->le_Livery, &QLineEdit::returnPressed, this, &CDbLiverySelectorComponent::ps_dataChanged);
            connect(ui->le_Livery, &QLineEdit::returnPressed, this, &CDbLiverySelectorComponent::ps_dataChanged);

            connect(sGui->getWebDataServices(), &CWebDataServices::dataRead, this, &CDbLiverySelectorComponent::ps_liveriesRead);
            this->ps_liveriesRead(CEntityFlags::LiveryEntity, CEntityFlags::ReadFinished, sGui->getWebDataServices()->getLiveriesCount());
        }

        CDbLiverySelectorComponent::~CDbLiverySelectorComponent()
        {
            // void
        }

        void CDbLiverySelectorComponent::setLivery(const CLivery &livery)
        {
            QString code(livery.getCombinedCode());
            if (code.isEmpty())
            {
                this->ui->le_Livery->clear();
                return;
            }
            if (livery != m_currentLivery)
            {
                this->ui->le_Livery->setText(code);
                m_currentLivery = livery;
                emit changedLivery(livery);
            }
        }

        void CDbLiverySelectorComponent::setlivery(const QString &code)
        {
            QString liveryCode(code.toUpper().trimmed());
            int s = liveryCode.indexOf(' ');
            if (s >= 1) { liveryCode = liveryCode.left(s); }
            s = liveryCode.indexOf('(');
            if (s >= 1) { liveryCode = liveryCode.left(s).trimmed(); }

            if (this->m_currentLivery.matchesCombinedCode(liveryCode)) { return; }
            CLivery d(sApp->getWebDataServices()->getLiveries().findByCombinedCode(liveryCode));
            if (d.hasCompleteData())
            {
                this->setLivery(d);
            }
            else
            {
                this->ui->lbl_Description->setText("");
                this->ui->le_Livery->setText(code);
            }
        }

        CLivery CDbLiverySelectorComponent::getLivery() const
        {
            if (!sApp) { return CLivery(); }
            const QString liveryCode(
                this->stripExtraInfo(this->ui->le_Livery->text())
            );
            const CLivery liv(sApp->getWebDataServices()->getLiveries().findByCombinedCode(liveryCode));
            if (liv.hasCompleteData() && liv.hasValidDbKey())
            {
                // full data fetched
                return liv;
            }
            else
            {
                return this->m_currentLivery;
            }
        }

        void CDbLiverySelectorComponent::setReadOnly(bool readOnly)
        {
            this->ui->le_Livery->setReadOnly(readOnly);
        }

        void CDbLiverySelectorComponent::withLiveryDescription(bool description)
        {
            this->ui->lbl_Description->setVisible(description);
        }

        bool CDbLiverySelectorComponent::isSet() const
        {
            return this->getLivery().hasCompleteData();
        }

        void CDbLiverySelectorComponent::clear()
        {
            this->ui->le_Livery->clear();
        }

        void CDbLiverySelectorComponent::dragEnterEvent(QDragEnterEvent *event)
        {
            if (!event || !acceptDrop(event->mimeData())) { return; }
            setBackgroundRole(QPalette::Highlight);
            event->acceptProposedAction();
        }

        void CDbLiverySelectorComponent::dragMoveEvent(QDragMoveEvent *event)
        {
            if (!event || !acceptDrop(event->mimeData())) { return; }
            event->acceptProposedAction();
        }

        void CDbLiverySelectorComponent::dragLeaveEvent(QDragLeaveEvent *event)
        {
            if (!event) { return; }
            event->accept();
        }

        void CDbLiverySelectorComponent::dropEvent(QDropEvent *event)
        {
            if (!event || !acceptDrop(event->mimeData())) { return; }
            CVariant valueVariant(toCVariant(event->mimeData()));
            if (valueVariant.isValid())
            {
                if (valueVariant.canConvert<CLivery>())
                {
                    CLivery livery(valueVariant.value<CLivery>());
                    if (!livery.hasValidDbKey()) { return; }
                    this->setLivery(livery);
                }
                else if (valueVariant.canConvert<CLiveryList>())
                {
                    CLiveryList liveries(valueVariant.value<CLiveryList>());
                    if (liveries.isEmpty()) { return; }
                    this->setLivery(liveries.front());
                }
            }
        }

        void CDbLiverySelectorComponent::ps_liveriesRead(CEntityFlags::Entity entity, CEntityFlags::ReadState readState, int count)
        {
            if (!sApp) { return; }
            if (entity.testFlag(CEntityFlags::LiveryEntity) && readState == CEntityFlags::ReadFinished)
            {
                if (count > 0)
                {
                    QStringList codes(sApp->getWebDataServices()->getLiveries().getCombinedCodesPlusInfo(true));
                    QCompleter *c = new QCompleter(codes, this);
                    c->setCaseSensitivity(Qt::CaseInsensitive);
                    c->setCompletionMode(QCompleter::PopupCompletion);
                    c->setMaxVisibleItems(10);
                    this->connect(c, static_cast<void (QCompleter::*)(const QString &)>(&QCompleter::activated), this, &CDbLiverySelectorComponent::ps_completerActivated);

                    this->ui->le_Livery->setCompleter(c);
                    m_completerLiveries.reset(c); // deletes any old completer
                }
                else
                {
                    this->m_completerLiveries.reset(nullptr);
                }
            }
        }

        void CDbLiverySelectorComponent::ps_dataChanged()
        {
            if (!sApp) { return; }
            const QString code(
                this->stripExtraInfo(this->ui->le_Livery->text())
            );
            if (code.isEmpty()) { return; }
            const CLivery livery(sApp->getWebDataServices()->getLiveries().findByCombinedCode(code));
            this->setLivery(livery);
        }

        void CDbLiverySelectorComponent::ps_completerActivated(const QString &liveryCode)
        {
            this->setlivery(liveryCode);
        }

        QString CDbLiverySelectorComponent::stripExtraInfo(const QString &liveryCode) const
        {
            if (liveryCode.isEmpty()) { return ""; }
            const QString l(liveryCode.trimmed().toUpper());
            int is = l.indexOf(' ');
            int ib = l.indexOf('(');
            int i = qMin(is, ib);
            if (i < 0) { return l; }
            return l.left(i);
        }

    } // ns
} // ns
