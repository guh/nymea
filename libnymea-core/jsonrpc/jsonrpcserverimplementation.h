/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU General Public License as published by the Free Software
* Foundation, GNU version 3. This project is distributed in the hope that it
* will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef JSONRPCSERVERIMPLEMENTATION_H
#define JSONRPCSERVERIMPLEMENTATION_H

#include "jsonrpc/jsonrpcserver.h"
#include "jsonrpc/jsonhandler.h"
#include "transportinterface.h"
#include "usermanager/usermanager.h"

#include "types/thingclass.h"
#include "types/action.h"
#include "types/event.h"

#include <QObject>
#include <QVariantMap>
#include <QString>
#include <QSslConfiguration>

class Thing;

namespace nymeaserver {

class JsonRPCServerImplementation: public JsonHandler, public JsonRPCServer
{
    Q_OBJECT
public:
    JsonRPCServerImplementation(const QSslConfiguration &sslConfiguration = QSslConfiguration(), QObject *parent = nullptr);

    // JsonHandler API implementation
    QString name() const override;
    Q_INVOKABLE JsonReply *Hello(const QVariantMap &params, const JsonContext &context);
    Q_INVOKABLE JsonReply *Introspect(const QVariantMap &params) const;
    Q_INVOKABLE JsonReply *Version(const QVariantMap &params) const;
    Q_INVOKABLE JsonReply *SetNotificationStatus(const QVariantMap &params, const JsonContext &context);

    Q_INVOKABLE JsonReply *CreateUser(const QVariantMap &params);
    Q_INVOKABLE JsonReply *Authenticate(const QVariantMap &params);
    Q_INVOKABLE JsonReply *RequestPushButtonAuth(const QVariantMap &params, const JsonContext &context);
    Q_INVOKABLE JsonReply *Tokens(const QVariantMap &params, const JsonContext &context) const;
    Q_INVOKABLE JsonReply *RemoveToken(const QVariantMap &params);
    Q_INVOKABLE JsonReply *SetupCloudConnection(const QVariantMap &params);
    Q_INVOKABLE JsonReply *SetupRemoteAccess(const QVariantMap &params);
    Q_INVOKABLE JsonReply *IsCloudConnected(const QVariantMap &params);
    Q_INVOKABLE JsonReply *KeepAlive(const QVariantMap &params);

signals:
    void CloudConnectedChanged(const QVariantMap &map);
    void PushButtonAuthFinished(const QUuid &clientId, const QVariantMap &params);

    // Server API
public:
    void registerTransportInterface(TransportInterface *interface, bool authenticationRequired);
    void unregisterTransportInterface(TransportInterface *interface);

    bool registerHandler(JsonHandler *handler) override;
    bool registerExperienceHandler(JsonHandler *handler, int majorVersion, int minorVersion) override;

private:
    QHash<QString, JsonHandler *> handlers() const;

    void sendResponse(TransportInterface *interface, const QUuid &clientId, int commandId, const QVariantMap &params = QVariantMap(), const QString &deprecationWarning = QString());
    void sendErrorResponse(TransportInterface *interface, const QUuid &clientId, int commandId, const QString &error);
    void sendUnauthorizedResponse(TransportInterface *interface, const QUuid &clientId, int commandId, const QString &error);

    void processJsonPacket(TransportInterface *interface, const QUuid &clientId, const QByteArray &data);

private slots:
    void setup();

    void clientConnected(const QUuid &clientId);
    void clientDisconnected(const QUuid &clientId);

    void processData(const QUuid &clientId, const QByteArray &data);

    void sendNotification(const QVariantMap &params);
    void sendClientNotification(const QUuid &clientId, const QVariantMap &params);

    void asyncReplyFinished();

    void pairingFinished(QString cognitoUserId, int status, const QString &message);
    void onCloudConnectionStateChanged();
    void onPushButtonAuthFinished(int transactionId, bool success, const QByteArray &token);

private:
    QVariantMap m_api;
    QHash<JsonHandler*, QString> m_experiences;
    QMap<TransportInterface*, bool> m_interfaces; // Interface, authenticationRequired
    QHash<QString, JsonHandler *> m_handlers;
    QHash<JsonReply *, TransportInterface *> m_asyncReplies;

    QHash<QUuid, TransportInterface*> m_clientTransports;
    QHash<QUuid, QByteArray> m_clientBuffers;
    QHash<QUuid, QStringList> m_clientNotifications;
    QHash<QUuid, QLocale> m_clientLocales;
    QHash<int, QUuid> m_pushButtonTransactions;
    QHash<QUuid, QTimer*> m_newConnectionWaitTimers;

    QHash<QString, JsonReply*> m_pairingRequests;

    int m_notificationId;

    QString formatAssertion(const QString &targetNamespace, const QString &method, QMetaMethod::MethodType methodType, JsonHandler *handler, const QVariantMap &data) const;
};

}

#endif // JSONRPCSERVERIMPLEMENTATION_H

