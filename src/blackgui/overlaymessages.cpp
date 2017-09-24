/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackgui/overlaymessages.h"
#include "blackgui/components/statusmessageform.h"
#include "blackgui/components/statusmessageformsmall.h"
#include "blackgui/guiapplication.h"
#include "blackgui/models/statusmessagelistmodel.h"
#include "blackgui/stylesheetutility.h"
#include "blackgui/views/statusmessageview.h"
#include "blackgui/views/viewbase.h"
#include "blackmisc/aviation/callsign.h"
#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/network/textmessage.h"
#include "blackcore/context/contextownaircraft.h"
#include "blackcore/application.h"
#include "ui_overlaymessages.h"

#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QSize>
#include <QStackedWidget>
#include <QStyle>
#include <QGraphicsDropShadowEffect>
#include <QTextEdit>
#include <QToolButton>
#include <Qt>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackMisc::Network;
using namespace BlackMisc::Simulation;
using namespace BlackCore;
using namespace BlackCore::Context;
using namespace BlackGui::Models;
using namespace BlackGui::Views;
using namespace BlackGui::Settings;

namespace BlackGui
{
    COverlayMessages::COverlayMessages(int w, int h, QWidget *parent) :
        QFrame(parent),
        ui(new Ui::COverlayMessages)
    {
        this->init(w, h);
        this->showKillButton(false);
        connect(sGui, &CGuiApplication::styleSheetsChanged, this, &COverlayMessages::ps_onStyleSheetsChanged);
        connect(ui->pb_Ok, &QPushButton::clicked, this, &COverlayMessages::ps_okClicked);
        connect(ui->pb_Cancel, &QPushButton::clicked, this, &COverlayMessages::ps_cancelClicked);
        connect(ui->tb_Kill, &QPushButton::clicked, this, &COverlayMessages::ps_killClicked);

        ui->tvp_StatusMessages->setResizeMode(CStatusMessageView::ResizingAlways);
        ui->tvp_StatusMessages->setForceColumnsToMaxSize(false); // problems with multinline entries, with T138 we need multiline messages
        ui->tvp_StatusMessages->setWordWrap(true);
        ui->tvp_StatusMessages->menuAddItems(CStatusMessageView::MenuSave);
        ui->fr_Confirmation->setVisible(false);
        this->setDefaultConfirmationButton(QMessageBox::Cancel);
    }

    COverlayMessages::COverlayMessages(const QString &headerText, int w, int h, QWidget *parent) :
        QFrame(parent),
        ui(new Ui::COverlayMessages),
        m_header(headerText)
    {
        this->init(w, h);
    }

    void COverlayMessages::init(int w, int h)
    {
        ui->setupUi(this);
        this->resize(w, h);
        this->setAutoFillBackground(true);
        m_autoCloseTimer.setObjectName(objectName() + ":autoCloseTimer");
        ui->tvp_StatusMessages->setMode(CStatusMessageListModel::Simplified);
        connect(ui->tb_Close, &QToolButton::released, this, &COverlayMessages::close);
        connect(&m_autoCloseTimer, &QTimer::timeout, this, &COverlayMessages::close);
    }

    void COverlayMessages::setHeader(const QString &header)
    {
        ui->lbl_Header->setText(m_header.isEmpty() ? header : m_header);
    }

    void COverlayMessages::ps_onStyleSheetsChanged()
    {
        // stlye sheet
    }

    void COverlayMessages::ps_okClicked()
    {
        m_lastConfirmation = QMessageBox::Ok;
        if (m_okLambda) { m_okLambda(); }
        this->close();
    }

    void COverlayMessages::ps_cancelClicked()
    {
        m_lastConfirmation = QMessageBox::Cancel;
        this->close();
    }

    void COverlayMessages::ps_killClicked()
    {
        QMessageBox msgBox;
        msgBox.setText("Shutdown the application.");
        msgBox.setInformativeText("Do you want to terminate " + sGui->getApplicationNameAndVersion() + "?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        if (QMessageBox::Ok == msgBox.exec())
        {
            sGui->gracefulShutdown();
            sGui->exit();
        }
    }

    bool COverlayMessages::useSmall() const
    {
        return (this->width() < 400);
    }

    void COverlayMessages::showKill(bool show)
    {
        ui->tb_Kill->setVisible(m_hasKillButton && show);
        ui->tb_Kill->setEnabled(m_hasKillButton && show);
    }

    bool COverlayMessages::displayTextMessage(const CTextMessage &textMessage) const
    {
        if (!sApp || sApp->isShuttingDown() || !sApp->getIContextOwnAircraft()) { return false; }
        const CTextMessageSettings s = m_messageSettings.getThreadLocal();
        if (s.popup(textMessage)) { return true; } // fast check without needing own aircraft
        const CSimulatedAircraft ownAircraft(sApp->getIContextOwnAircraft()->getOwnAircraft());
        return s.popup(textMessage, ownAircraft);
    }

    COverlayMessages::~COverlayMessages()
    {}

    void COverlayMessages::showOverlayMessages(const BlackMisc::CStatusMessageList &messages, int timeOutMs)
    {
        if (messages.isEmpty()) { return; }
        if (!sApp || sApp->isShuttingDown()) { return; }
        if (this->hasPendingConfirmation())
        {
            // defer message
            m_pendingMessageCalls.push_back([ = ]()
            {
                this->showOverlayMessages(messages, timeOutMs);
            });
            return;
        }

        ui->tvp_StatusMessages->updateContainerMaybeAsync(messages);
        ui->tvp_StatusMessages->resizeRowsToContents();
        this->showKill(false);
        this->setModeToMessages(messages.hasErrorMessages());
        this->display(timeOutMs);
    }

    void COverlayMessages::showOverlayMessage(const BlackMisc::CStatusMessage &message, int timeOutMs)
    {
        if (message.isEmpty()) { return; }
        if (!sApp || sApp->isShuttingDown()) { return; }
        if (this->hasPendingConfirmation())
        {
            // defer message
            m_pendingMessageCalls.push_back([ = ]()
            {
                this->showOverlayMessage(message, timeOutMs);
            });
            return;
        }

        if (this->useSmall())
        {
            this->setModeToMessageSmall(message.isFailure());
            ui->form_StatusMessageSmall->setValue(message);
        }
        else
        {
            this->setModeToMessage(message.isFailure());
            ui->form_StatusMessage->setValue(message);
        }
        this->display(timeOutMs);
    }

    void COverlayMessages::showOverlayTextMessage(const CTextMessage &textMessage, int timeOutMs)
    {
        if (textMessage.isEmpty()) { return; }
        if (!displayTextMessage(textMessage)) { return; }
        if (!sApp || sApp->isShuttingDown()) { return; }

        if (this->hasPendingConfirmation())
        {
            // defer message
            m_pendingMessageCalls.push_back([ = ]()
            {
                this->showOverlayTextMessage(textMessage, timeOutMs);
            });
            return;
        }

        this->setModeToTextMessage();

        // message and display
        ui->le_TmFrom->setText(textMessage.getSenderCallsign().asString());
        ui->le_TmTo->setText(textMessage.getRecipientCallsign().asString());
        ui->le_TmReceived->setText(textMessage.getFormattedUtcTimestampHms());
        ui->te_TmText->setText(textMessage.getMessage());

        this->display(timeOutMs);
    }

    void COverlayMessages::showOverlayImage(const CPixmap &image, int timeOutMs)
    {
        this->showOverlayImage(image.toPixmap(), timeOutMs);
    }

    void COverlayMessages::showOverlayImage(const QPixmap &image, int timeOutMs)
    {
        if (this->hasPendingConfirmation())
        {
            // defer message
            m_pendingMessageCalls.push_back([ = ]()
            {
                this->showOverlayImage(image, timeOutMs);
            });
            return;
        }

        this->setModeToImage();
        QSize sizeAvailable = ui->fr_StatusMessagesComponentsInner->size();
        if (sizeAvailable.width() < 300)
        {
            // first time inner frame is not giving correct size, workaround
            sizeAvailable = this->size() * 0.9;
        }

        ui->lbl_Image->setText("");
        if (image.isNull())
        {
            static const QPixmap e;
            ui->lbl_Image->setPixmap(e);
        }
        else
        {
            ui->lbl_Image->setPixmap(
                image.scaled(sizeAvailable, Qt::KeepAspectRatio, Qt::FastTransformation)
            );
        }
        this->display(timeOutMs);
    }

    void COverlayMessages::showOverlayVariant(const BlackMisc::CVariant &variant, int timeOutMs)
    {
        if (variant.canConvert<CStatusMessageList>())
        {
            this->showOverlayMessages(variant.value<CStatusMessageList>(), timeOutMs);
        }
        else if (variant.canConvert<CStatusMessage>())
        {
            this->showOverlayMessage(variant.value<CStatusMessage>(), timeOutMs);
        }
        else if (variant.canConvert<CTextMessage>())
        {
            this->showOverlayTextMessage(variant.value<CTextMessage>(), timeOutMs);
        }
        else if (variant.canConvert<QPixmap>())
        {
            this->showOverlayImage(variant.value<QPixmap>(), timeOutMs);
        }
        else if (variant.canConvert<CPixmap>())
        {
            this->showOverlayImage(variant.value<CPixmap>(), timeOutMs);
        }
        else
        {
            Q_ASSERT_X(false, Q_FUNC_INFO, "Unsupported type");
        }
    }

    void COverlayMessages::showKillButton(bool killButton)
    {
        m_hasKillButton = killButton;
    }

    void COverlayMessages::setModeToMessages(bool withKillButton)
    {
        ui->sw_StatusMessagesComponent->setCurrentWidget(ui->pg_StatusMessages);
        this->showKill(withKillButton);
        this->setHeader("Messages");
    }

    void COverlayMessages::setModeToMessage(bool withKillButton)
    {
        ui->sw_StatusMessagesComponent->setCurrentWidget(ui->pg_StatusMessage);
        this->showKill(withKillButton);
        this->setHeader("Message");
    }

    void COverlayMessages::setModeToMessageSmall(bool withKillButton)
    {
        ui->sw_StatusMessagesComponent->setCurrentWidget(ui->pg_StatusMessageSmall);
        this->showKill(withKillButton);
        this->setHeader("Message");
    }

    void COverlayMessages::setModeToTextMessage()
    {
        ui->sw_StatusMessagesComponent->setCurrentWidget(ui->pg_TextMessage);
        this->setHeader("Text message");
    }

    void COverlayMessages::setModeToImage()
    {
        ui->sw_StatusMessagesComponent->setCurrentWidget(ui->pg_Image);
        this->setHeader("Image");
        this->showKillButton(false);
    }

    void COverlayMessages::setConfirmationMessage(const QString &message)
    {
        if (message.isEmpty())
        {
            ui->fr_Confirmation->setVisible(false);
        }
        else
        {
            ui->fr_Confirmation->setVisible(true);
            ui->lbl_Confirmation->setText(message);
        }
    }

    void COverlayMessages::showOverlayMessagesWithConfirmation(const CStatusMessageList &messages, const QString &confirmationMessage, std::function<void ()> okLambda, int defaultButton, int timeOutMs)
    {
        if (this->hasPendingConfirmation())
        {
            // defer message
            m_pendingMessageCalls.push_back([ = ]()
            {
                this->showOverlayMessagesWithConfirmation(messages, confirmationMessage, okLambda, defaultButton, timeOutMs);
            });
            return;
        }
        this->setConfirmationMessage(confirmationMessage);
        this->showOverlayMessages(messages, timeOutMs);
        m_awaitingConfirmation = true; // needs to be after showOverlayMessages
        m_okLambda = okLambda;
        this->setDefaultConfirmationButton(defaultButton);
    }

    void COverlayMessages::setDefaultConfirmationButton(int button)
    {
        switch (button)
        {
        case QMessageBox::Ok:
            ui->pb_Cancel->setDefault(false);
            ui->pb_Cancel->setAutoDefault(false);
            ui->pb_Ok->setDefault(true);
            ui->pb_Ok->setAutoDefault(true);
            ui->pb_Ok->setFocus();
            m_lastConfirmation = QMessageBox::Ok;
            break;
        case QMessageBox::Cancel:
            ui->pb_Ok->setDefault(false);
            ui->pb_Ok->setAutoDefault(false);
            ui->pb_Cancel->setDefault(true);
            ui->pb_Cancel->setAutoDefault(true);
            ui->pb_Cancel->setFocus();
            m_lastConfirmation = QMessageBox::Cancel;
            break;
        default:
            break;
        }
    }

    bool COverlayMessages::hasPendingConfirmation() const
    {
        return m_awaitingConfirmation;
    }

    void COverlayMessages::addShadow()
    {
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        QColor color(Qt::blue);
        color.setAlpha(96);
        shadow->setColor(color);
        this->setGraphicsEffect(shadow);
    }

    void COverlayMessages::keyPressEvent(QKeyEvent *event)
    {
        if (!this->isVisible()) { QFrame::keyPressEvent(event); }
        if (event->key() ==  Qt::Key_Escape)
        {
            this->close();
            event->accept();
        }
        else
        {
            QFrame::keyPressEvent(event);
        }
    }

    void COverlayMessages::close()
    {
        this->hide();
        this->setEnabled(false);
        ui->fr_Confirmation->setVisible(false);
        if (m_awaitingConfirmation)
        {
            emit this->confirmationCompleted();
        }
        else
        {
            m_lastConfirmation = QMessageBox::Cancel;
        }
        m_awaitingConfirmation = false;
        m_okLambda = nullptr;

        if (!m_pendingMessageCalls.isEmpty())
        {
            std::function<void()> f = m_pendingMessageCalls.front();
            m_pendingMessageCalls.removeFirst();
            QTimer::singleShot(500, this, f);
        }
    }

    void COverlayMessages::display(int timeOutMs)
    {
        this->show();
        this->setEnabled(true);
        if (timeOutMs > 250)
        {
            m_autoCloseTimer.start(timeOutMs);
        }
        else
        {
            m_autoCloseTimer.stop();
        }
    }
} // ns
