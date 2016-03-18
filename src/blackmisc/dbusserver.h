/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_DBUSSERVER_H
#define BLACKMISC_DBUSSERVER_H

#include "blackmiscexport.h"
#include "blackmisc/valueobject.h"
#include <QObject>
#include <QtDBus/QDBusServer>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusConnection>
#include <QStringList>
#include <QMap>

//! Service name of DBus service
#define SWIFT_SERVICENAME "org.swift-project"

namespace BlackMisc
{

    /*!
     * Custom DBusServer
     * \details This class implements a custom DBusServer for DBus peer connections, but can also be used
     *          with session or system bus. For session/system bus this class represents no real server,
     *          but more a wrapper for QDBusConnection and object registration.
     */
    class BLACKMISC_EXPORT CDBusServer : public QObject
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", SWIFT_SERVICENAME)

    public:
        //! Default service name
        static const QString &coreServiceName();

        //! Server mode
        enum ServerMode
        {
            SERVERMODE_P2P,
            SERVERMODE_SESSIONBUS,
            SERVERMODE_SYSTEMBUS
        };

        //! Construct a server for the core service
        CDBusServer(const QString &address, QObject *parent = nullptr) : CDBusServer(coreServiceName(), address, parent) {}

        //! Construct a server for some arbitrary service
        CDBusServer(const QString &service, const QString &address, QObject *parent = nullptr);

        //! Destructor
        virtual ~CDBusServer();

        //! Add a QObject to be exposed via DBus
        void addObject(const QString &name, QObject *object);

        //! Last error
        QDBusError lastQDBusServerError() const;

        //! DBus server (if using P2P)
        const QDBusServer *qDBusServer() const;

        //! True if using P2P
        bool hasQDBusServer() const;

        //! Remove all objects added with addObject
        void removeAllObjects();

        //! Default connection
        static const QDBusConnection &defaultConnection();

        //! Address denoting a session bus server
        static const QString &sessionBusAddress();

        //! Address denoting a system bus server
        static const QString &systemBusAddress();

        //! Address denoting a P2P server at the given host and port.
        //! \remarks Port number may be embedding in the host string after a colon.
        //! \return p2p address like "tcp:host=foo.bar.com,port=1234"
        static QString p2pAddress(const QString &host, const QString &port = "");

        //! Turn something like 127.0.0.1:45000 into "tcp:host=127.0.0.1,port=45000"
        //! \note Handles also "session" and "system" as valid address while CDBusServer::p2pAddress is for
        //! P2P addresses only.
        static QString normalizeAddress(const QString &address);

        //! Return the server mode of the given address
        static ServerMode modeOfAddress(QString address);

        //! True if a valid Qt DBus address, e.g. "unix:tmpdir=/tmp", "tcp:host=127.0.0.1,port=45000"
        static bool isQtDBusAddress(const QString &address);

        //! True if address is session or system bus address
        static bool isSessionOrSystemAddress(const QString &address);

        //! False if address is session or system bus address
        static bool isP2PAddress(const QString &address);

        //! Extract host and port from a DBus address
        static bool dBusAddressToHostAndPort(QString dbusAddress, QString &o_host, int &o_port);

        //! Is there a DBus server running at the given address?
        //! @{
        static bool isDBusAvailable(const QString &host, int port, int timeoutMs = 1500);
        static bool isDBusAvailable(const QString &host, int port, QString &o_message, int timeoutMs = 1500);
        static bool isDBusAvailable(const QString &dbusAddress, QString &o_message, int timeoutMs = 1500);
        static bool isDBusAvailable(const QString &dbusAddress, int timeoutMs = 1500);
        //! @}

    private:
        ServerMode m_serverMode = SERVERMODE_P2P;
        QScopedPointer<QDBusServer> m_busServer;
        QMap<QString, QObject *> m_objects;
        QMap<QString, QDBusConnection> m_connections;

        void launchDBusDaemon();
        static QString getDBusInterfaceFromClassInfo(QObject *object);

        //! Register options with connection
        static QDBusConnection::RegisterOptions registerOptions()
        {
            return QDBusConnection::ExportAdaptors | QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllSlots;
        }

    private slots:
        //! Called when a new DBus client has connected in P2P mode
        bool ps_registerObjectsWithP2PConnection(QDBusConnection connection);
    };
}

#endif // guard
