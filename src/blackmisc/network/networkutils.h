/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_NETWORKUTILS_H
#define BLACKMISC_NETWORKUTILS_H

#include "blackmisc/network/urllist.h"
#include "blackmisc/blackmiscexport.h"
#include "blackmisc/statusmessagelist.h"

#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QString>
#include <QStringList>
#include <QtGlobal>

class QHttpPart;
class QNetworkReply;
class QUrl;
class QUrlQuery;

namespace BlackMisc
{
    namespace Network
    {
        class CServer;

        //! Utilities, e.g. checking whether a network connection can be established
        class BLACKMISC_EXPORT CNetworkUtils
        {
        public:
            //! Request type
            enum RequestType
            {
                Get,
                PostUrlEncoded,
                Multipart
            };

            //! Default for timeout
            static int getTimeoutMs();

            //! Default for timeout (long)
            static int getLongTimeoutMs();

            //! Can ping the address?
            //! \note uses OS ping
            static bool canPing(const QString &hostAddress);

            //! Can connect?
            //! \param hostAddress   130.4.20.3, or myserver.com
            //! \param port          80, 1234
            //! \param timeoutMs
            //! \param message       human readable message
            static bool canConnect(const QString &hostAddress, int port, QString &message, int timeoutMs = getTimeoutMs());

            //! Can connect to server?
            //! \param server
            //! \param message       human readable message
            //! \param timeoutMs
            static bool canConnect(const BlackMisc::Network::CServer &server, QString &message, int timeoutMs = getTimeoutMs());

            //! Can connect to URL?
            static bool canConnect(const QString &url, QString &message, int timeoutMs = getTimeoutMs());

            //! Can connect to URL?
            static bool canConnect(const QUrl &url, QString &message, int timeoutMs = getTimeoutMs());

            //! Can connect to URL?
            static bool canConnect(const QUrl &url, int timeoutMs = getTimeoutMs());

            //! Can connect to URL?
            static bool canConnect(const BlackMisc::Network::CUrl &url, QString &message, int timeoutMs = getTimeoutMs());

            //! Can connect to URL?
            static bool canConnect(const BlackMisc::Network::CUrl &url, int timeoutMs = getTimeoutMs());

            //! Find out my IPv4 addresses including loopback, empty if not possible
            static QStringList getKnownLocalIpV4Addresses();

            //! Valid IPv4 address
            static bool isValidIPv4Address(const QString &candidate);

            //! Valid IPv6 address
            static bool isValidIPv6Address(const QString &candidate);

            //! Valid port
            static bool isValidPort(const QString &port);

            //! Build / concatenate an URL
            static QString buildUrl(const QString &protocol, const QString &server, const QString &baseUrl, const QString &serviceUrl);

            //! Ignore SSL verification such as self signed certificates
            static void ignoreSslVerification(QNetworkRequest &request);

            //! Set user agent for request
            static void setSwiftUserAgent(QNetworkRequest &request);

            //! Set swift client SSL certificate
            static void setSwiftClientSslCertificate(QNetworkRequest &request);

            //! Set swift client SSL certificate
            static void setSwiftClientSslCertificate(QNetworkRequest &request, const BlackMisc::Network::CUrlList &swiftSharedUrls);

            //! Add debug flag
            static void addDebugFlag(QUrlQuery &qurl);

            //! Our tweaked network request
            static QNetworkRequest getNetworkRequest(const CUrl &url, RequestType type = Get);

            //! Last modified from reply
            static qint64 lastModifiedMsSinceEpoch(QNetworkReply *nwReply);

            //! Last modified from reply
            static QDateTime lastModifiedDateTime(QNetworkReply *nwReply);

            //! Last modified from reply compared with now (in ms)
            static qint64 lastModifiedSinceNow(QNetworkReply *nwReply);

            //! Get the http status code
            static int getHttpStatusCode(QNetworkReply *nwReply);

            //! Is the reply an HTTP redirect?
            //! \details Status codes:
            //! - 301: Permanent redirect. Clients making subsequent requests for this resource should use the new URI. Clients should not follow the redirect automatically for POST/PUT/DELETE requests.
            //! - 302: Redirect for undefined reason. Clients making subsequent requests for this resource should not use the new URI. Clients should not follow the redirect automatically for POST/PUT/DELETE requests.
            //! - 303: Redirect for undefined reason. Typically, 'Operation has completed, continue elsewhere.' Clients making subsequent requests for this resource should not use the new URI. Clients should follow the redirect for POST/PUT/DELETE requests.
            //! - 307: Temporary redirect. Resource may return to this location at a later point. Clients making subsequent requests for this resource should use the old URI. Clients should not follow the redirect automatically for POST/PUT/DELETE requests.
            static bool isHttpStatusRedirect(QNetworkReply *nwReply);

            //! Get the redirect URL if any
            static QUrl getHttpRedirectUrl(QNetworkReply *nwReply);

            //! Remove the HTML formatting from a PHP error message
            static QString removeHtmlPartsFromPhpErrorMessage(const QString &errorMessage);

            //! Status about network
            static BlackMisc::CStatusMessageList createNetworkReport(const QNetworkAccessManager *am);

            //! Status about network, can be used when an URL fails to resolve issues
            //! \remark that can take a moment to complete, as it checks network
            static BlackMisc::CStatusMessageList createNetworkReport(const CUrl &url, const QNetworkAccessManager *am = nullptr);

        private:
            //! Hidden constructor
            CNetworkUtils() {}
        };
    } // namespace
} // namespace

#endif // guard
