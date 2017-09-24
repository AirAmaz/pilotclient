/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_CONTEXT_CONTEXTAPPLICATION_PROXY_H
#define BLACKCORE_CONTEXT_CONTEXTAPPLICATION_PROXY_H

#include <QObject>
#include <QString>
#include <QStringList>

#include "blackcore/blackcoreexport.h"
#include "blackcore/context/contextapplication.h"
#include "blackcore/corefacadeconfig.h"
#include "blackmisc/identifier.h"
#include "blackmisc/identifierlist.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/valuecache.h"

class QDBusConnection;

namespace BlackMisc
{
    class CGenericDBusInterface;
    class CLogPattern;
}

namespace BlackCore
{
    class CCoreFacade;
    namespace Context
    {
        //! Application context proxy
        //! \ingroup dbus
        class BLACKCORE_EXPORT CContextApplicationProxy : public IContextApplication
        {
            Q_OBJECT
            friend class IContextApplication;

        public:
            //! Destructor
            virtual ~CContextApplicationProxy() {}

        public slots:
            //! @{
            //! \publicsection
            virtual void logMessage(const BlackMisc::CStatusMessage &message, const BlackMisc::CIdentifier &origin) override;
            virtual void addLogSubscription(const BlackMisc::CIdentifier &subscriber, const BlackMisc::CLogPattern &pattern) override;
            virtual void removeLogSubscription(const BlackMisc::CIdentifier &subscriber, const BlackMisc::CLogPattern &pattern) override;
            virtual BlackCore::Context::CLogSubscriptionHash getAllLogSubscriptions() const override;
            virtual void synchronizeLogSubscriptions() override;
            virtual void changeSettings(const BlackMisc::CValueCachePacket &settings, const BlackMisc::CIdentifier &origin) override;
            virtual BlackMisc::CValueCachePacket getAllSettings() const override;
            virtual QStringList getUnsavedSettingsKeys() const override;
            virtual BlackCore::Context::CSettingsDictionary getUnsavedSettingsKeysDescribed() const override;
            virtual void synchronizeLocalSettings() override;
            virtual BlackMisc::CStatusMessage saveSettings(const QString &keyPrefix = {}) override;
            virtual BlackMisc::CStatusMessage saveSettingsByKey(const QStringList &keys) override;
            virtual BlackMisc::CStatusMessage loadSettings() override;
            virtual void registerHotkeyActions(const QStringList &actions, const BlackMisc::CIdentifier &origin) override;
            virtual void callHotkeyAction(const QString &action, bool argument, const BlackMisc::CIdentifier &origin) override;
            virtual BlackMisc::CIdentifier registerApplication(const BlackMisc::CIdentifier &application) override;
            virtual void unregisterApplication(const BlackMisc::CIdentifier &application) override;
            virtual BlackMisc::CIdentifierList getRegisteredApplications() const override;
            virtual bool writeToFile(const QString &fileName, const QString &content) override;
            virtual QString readFromFile(const QString &fileName) const override;
            virtual bool removeFile(const QString &fileName) override;
            virtual bool existsFile(const QString &fileName) const override;
            virtual QString dotCommandsHtmlHelp() const override;
            //! @}

            //! Used to test if there is a core running?
            //! \note creates and connects via proxy object, so not meant for very frequent tests
            //! \sa CDBusServer::isDBusAvailable as lightweight, but less accurate alternative
            static bool isContextResponsive(const QString &dbusAddress, QString &msg, int timeoutMs = 1500);

        protected:
            //! Constructor
            CContextApplicationProxy(CCoreFacadeConfig::ContextMode mode, CCoreFacade *runtime) : IContextApplication(mode, runtime), m_dBusInterface(nullptr) {}

            //! DBus version constructor
            CContextApplicationProxy(const QString &serviceName, QDBusConnection &connection, CCoreFacadeConfig::ContextMode mode, CCoreFacade *runtime);

        private:
            BlackMisc::CGenericDBusInterface *m_dBusInterface = nullptr;

            //! Relay connection signals to local signals
            void relaySignals(const QString &serviceName, QDBusConnection &connection);
        };
    } // ns
} // ns

#endif // guard
