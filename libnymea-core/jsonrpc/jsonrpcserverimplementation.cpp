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

/*!
    \class nymeaserver::JsonRPCServer
    \brief This class provides a JSON-RPC API interface to the \l{TransportInterface}{TransportInterfaces}.

    \ingroup server
    \inmodule core

    The \l{JsonRPCServer} class provides the server interface for a JSON-RPC API call. This class
    communicates with \l{TransportInterface}{TransportInterfaces} and processes the
    JSON-RPC request in the corresponding \l{JsonHandler}. The \l{JsonRPCServer} it self is also
    an \l{JsonHandler} and provides the introspection, version and notification control methods
    for the \l{JSON-RPC API}.

    \sa ServerManager, TransportInterface, TcpServer, WebSocketServer
*/


#include "jsonrpcserverimplementation.h"
#include "jsonrpc/jsonhandler.h"
#include "jsonvalidator.h"
#include "nymeacore.h"
#include "integrations/thingmanager.h"
#include "integrations/integrationplugin.h"
#include "integrations/thing.h"
#include "types/thingclass.h"
#include "ruleengine/rule.h"
#include "ruleengine/ruleengine.h"
#include "loggingcategories.h"
#include "platform/platform.h"
#include "version.h"
#include "cloud/cloudmanager.h"

#include "devicehandler.h"
#include "integrationshandler.h"
#include "actionhandler.h"
#include "ruleshandler.h"
#include "scriptshandler.h"
#include "eventhandler.h"
#include "logginghandler.h"
#include "statehandler.h"
#include "configurationhandler.h"
#include "networkmanagerhandler.h"
#include "tagshandler.h"
#include "appdatahandler.h"
#include "systemhandler.h"
#include "usershandler.h"
#include "zigbeehandler.h"
#include "modbusrtuhandler.h"

#include <QJsonDocument>
#include <QStringList>
#include <QSslConfiguration>

namespace nymeaserver {

/*! Constructs a \l{JsonRPCServer} with the given \a sslConfiguration and \a parent. */
JsonRPCServerImplementation::JsonRPCServerImplementation(const QSslConfiguration &sslConfiguration, QObject *parent):
    JsonHandler(parent),
    m_notificationId(0)
{
    Q_UNUSED(sslConfiguration)
    // First, define our own JSONRPC API

    // Enums
    registerEnum<BasicType>();
    registerEnum<UserManager::UserError>();
    registerEnum<CloudManager::CloudConnectionState>();
    registerEnum<Types::PermissionScope, Types::PermissionScopes>();

    // Objects
    registerObject<TokenInfo>();

    QVariantMap experiece;
    experiece.insert("name", enumValueName(String));
    experiece.insert("version", enumValueName(String));
    registerObject("Experience", experiece);

    QVariantMap cacheHash;
    cacheHash.insert("method", enumValueName(String));
    cacheHash.insert("hash", enumValueName(String));
    registerObject("CacheHash", cacheHash);

    // Methods
    QString description; QVariantMap returns; QVariantMap params;
    description = "Initiates a connection. Use this method to perform an initial handshake of the "
                            "connection. Optionally, a parameter \"locale\" is can be passed to set up the used "
                            "locale for this connection. Strings such as ThingClass displayNames etc will be "
                            "localized to this locale. If this parameter is omitted, the default system locale "
                            "(depending on the configuration) is used. The reply of this method contains information "
                            "about this core instance such as version information, uuid and its name. The locale value"
                            "indicates the locale used for this connection. Note: This method can be called multiple "
                            "times. The locale used in the last call for this connection will be used. Other values, "
                            "like initialSetupRequired might change if the setup has been performed in the meantime.\n "
                            "The field cacheHashes may contain a map of methods and MD5 hashes. As long as the hash for "
                            "a method does not change, a client may use a previously cached copy of the call instead of "
                            "fetching the content again. While the Hello call doesn't necessarily require a token, this "
                            "can be called with a token. If the token is valid, the reyponse will contain information about "
                            "the user and permissions for the given token.";
    params.insert("o:locale", enumValueName(String));
    returns.insert("server", enumValueName(String));
    returns.insert("name", enumValueName(String));
    returns.insert("version", enumValueName(String));
    returns.insert("uuid", enumValueName(Uuid));
    returns.insert("language", enumValueName(String));
    returns.insert("locale", enumValueName(String));
    returns.insert("protocol version", enumValueName(String));
    returns.insert("initialSetupRequired", enumValueName(Bool));
    returns.insert("authenticationRequired", enumValueName(Bool));
    returns.insert("pushButtonAuthAvailable", enumValueName(Bool));
    returns.insert("o:experiences", QVariantList() << objectRef("Experience"));
    returns.insert("o:cacheHashes", QVariantList() << objectRef("CacheHash"));
    returns.insert("o:permissionScopes", enumRef<Types::PermissionScopes>());
    returns.insert("o:username", enumValueName(String));
    registerMethod("Hello", description, params, returns, Types::PermissionScopeNone);

    params.clear(); returns.clear();
    description = "Introspect this API.";
    returns.insert("methods", enumValueName(Object));
    returns.insert("notifications", enumValueName(Object));
    returns.insert("types", enumValueName(Object));
    registerMethod("Introspect", description, params, returns, Types::PermissionScopeNone);

    params.clear(); returns.clear();
    description = "Version of this nymea/JSONRPC interface.";
    returns.insert("version", enumValueName(String));
    returns.insert("protocol version", enumValueName(String));
    returns.insert("qtVersion", enumValueName(String));
    returns.insert("qtBuildVersion", enumValueName(String));
    registerMethod("Version", description, params, returns, Types::PermissionScopeNone);

    params.clear(); returns.clear();
    description = "Enable/Disable notifications for this connections. Either \"enabled\" or """
                                            "\"namespaces\" needs to be given but not both of them. The boolean based "
                                            "\"enabled\" parameter will enable/disable all notifications at once. If "
                                            "instead the list-based \"namespaces\" parameter is provided, all given namespaces"
                                            "will be enabled, the others will be disabled. The return value of \"success\" will "
                                            "indicate success of the operation. The \"enabled\" property in the return value is "
                                            "deprecated and used for legacy compatibilty only. It will be set to true if at least "
                                            "one namespace has been enabled.";
    params.insert("o:namespaces", enumValueName(StringList));
    params.insert("d:o:enabled", enumValueName(Bool));
    returns.insert("namespaces", enumValueName(StringList));
    returns.insert("d:enabled", enumValueName(Bool));
    registerMethod("SetNotificationStatus", description, params, returns, Types::PermissionScopeNone);

    params.clear(); returns.clear();
    description = "Create a new user in the API. Currently this is only allowed to be called once when a new nymea instance is set up. Call Authenticate after this to obtain a device token for this user.";
    params.insert("username", enumValueName(String));
    params.insert("password", enumValueName(String));
    returns.insert("error", enumRef<UserManager::UserError>());
    registerMethod("CreateUser", description, params, returns, Types::PermissionScopeAdmin, "Use Users.CreateUser instead.");

    params.clear(); returns.clear();
    description = "Authenticate a client to the api via user & password challenge. Provide "
                   "a device name which allows the user to identify the client and revoke the token in case "
                   "the device is lost or stolen. This will return a new token to be used to authorize a "
                   "client at the API.";
    params.insert("username", enumValueName(String));
    params.insert("password", enumValueName(String));
    params.insert("deviceName", enumValueName(String));
    returns.insert("success", enumValueName(Bool));
    returns.insert("o:token", enumValueName(String));
    returns.insert("o:username", enumValueName(String));
    returns.insert("o:scopes", enumRef<Types::PermissionScopes>());
    registerMethod("Authenticate", description, params, returns, Types::PermissionScopeNone, "Use Users.Authenticate instead.");

    params.clear(); returns.clear();
    description = "Authenticate a client to the api via Push Button method. "
                   "Provide a device name which allows the user to identify the client and revoke the "
                   "token in case the device is lost or stolen. If push button hardware is available, "
                   "this will return with success and start listening for push button presses. When the "
                   "push button is pressed, the PushButtonAuthFinished notification will be sent to the "
                   "requesting client. The procedure will be cancelled when the connection is interrupted. "
                   "If another client requests push button authentication while a procedure is still going "
                   "on, the second call will take over and the first one will be notified by the "
                   "PushButtonAuthFinished signal about the error. The application should make it clear "
                   "to the user to not press the button when the procedure fails as this can happen for 2 "
                   "reasons: a) a second user is trying to auth at the same time and only the currently "
                   "active user should press the button or b) it might indicate an attacker trying to take "
                   "over and snooping in for tokens.";
    params.insert("deviceName", enumValueName(String));
    returns.insert("success", enumValueName(Bool));
    returns.insert("transactionId", enumValueName(Int));
    registerMethod("RequestPushButtonAuth", description, params, returns, Types::PermissionScopeNone, "Use Users.RequestPushButtonAuth instead.");

    params.clear(); returns.clear();
    description = "Return a list of TokenInfo objects of all the tokens for the current user.";
    returns.insert("tokenInfoList", QVariantList() << objectRef("TokenInfo"));
    registerMethod("Tokens", description, params, returns, Types::PermissionScopeNone, "Use Users.GetTokens instead.");

    params.clear(); returns.clear();
    description = "Revoke access for a given token.";
    params.insert("tokenId", enumValueName(Uuid));
    returns.insert("error", enumRef<UserManager::UserError>());
    registerMethod("RemoveToken", description, params, returns, Types::PermissionScopeNone, "Use Users.RemoveToken instead.");

    params.clear(); returns.clear();
    description = "Sets up the cloud connection by deploying a certificate and its configuration.";
    params.insert("rootCA", enumValueName(String));
    params.insert("certificatePEM", enumValueName(String));
    params.insert("publicKey", enumValueName(String));
    params.insert("privateKey", enumValueName(String));
    params.insert("endpoint", enumValueName(String));
    returns.insert("success", enumValueName(Bool));
    registerMethod("SetupCloudConnection", description, params, returns);

    params.clear(); returns.clear();
    description = "Setup the remote connection by providing AWS token information. This requires the cloud to be connected.";
    params.insert("idToken", enumValueName(String));
    params.insert("userId", enumValueName(String));
    returns.insert("status", enumValueName(Int));
    returns.insert("message", enumValueName(String));
    registerMethod("SetupRemoteAccess", description, params, returns);

    params.clear(); returns.clear();
    description = "Check whether the cloud is currently connected. \"connected\" will be true whenever connectionState equals CloudConnectionStateConnected and is deprecated. Please use the connectionState value instead.";
    returns.insert("d:connected", enumValueName(Bool));
    returns.insert("connectionState", enumRef<CloudManager::CloudConnectionState>());
    registerMethod("IsCloudConnected", description, params, returns);

    params.clear(); returns.clear();
    description = "This is basically a Ping/Pong mechanism a client app may use to check server connectivity. Currently, the server does not actually do anything with this information and will return the call providing the given sessionId back to the caller. It is up to the client whether to use this or not and not required by the server to keep the connection alive.";
    params.insert("sessionId", enumValueName(String));
    returns.insert("success", enumValueName(Bool));
    returns.insert("sessionId", enumValueName(String));
    registerMethod("KeepAlive", description, params, returns, Types::PermissionScopeNone);

    // Notifications
    params.clear(); returns.clear();
    description = "Emitted whenever the cloud connection status changes.";
    params.insert("connected", enumValueName(Bool));
    params.insert("connectionState", enumRef<CloudManager::CloudConnectionState>());
    registerNotification("CloudConnectedChanged", description, params);

    params.clear();
    description = "Emitted when a push button authentication reaches final state. NOTE: This notification is special. It will only be emitted to connections that did actively request a push button authentication, but also it will be emitted regardless of the notification settings. ";
    params.insert("success", enumValueName(Bool));
    params.insert("transactionId", enumValueName(Int));
    params.insert("o:token", enumValueName(String));
    registerNotification("PushButtonAuthFinished", description, params, "Use Users.PushButtonAuthFinished instead.");

    QMetaObject::invokeMethod(this, "setup", Qt::QueuedConnection);

    connect(NymeaCore::instance()->userManager(), &UserManager::pushButtonAuthFinished, this, &JsonRPCServerImplementation::onPushButtonAuthFinished);
}

/*! Returns the \e namespace of \l{JsonHandler}. */
QString JsonRPCServerImplementation::name() const
{
    return QStringLiteral("JSONRPC");
}

JsonReply *JsonRPCServerImplementation::Hello(const QVariantMap &params, const JsonContext &context)
{
    TransportInterface *interface = reinterpret_cast<TransportInterface*>(property("transportInterface").toLongLong());

    qCDebug(dcJsonRpc()) << params;
    QUuid clientId = context.clientId();
    if (params.contains("locale")) {
        m_clientLocales.insert(clientId, QLocale(params.value("locale").toString()));
    }

    qCDebug(dcJsonRpc()) << "Client" << clientId << "initiated handshake." << m_clientLocales.value(clientId);

    // If we waited for the handshake, here it is. Remove the timer...
    if (m_newConnectionWaitTimers.contains(clientId)) {
        delete m_newConnectionWaitTimers.take(clientId);
    }

    // Compose the reply
    QVariantMap handshake;
    handshake.insert("server", "nymea");
    handshake.insert("name", NymeaCore::instance()->configuration()->serverName());
    handshake.insert("version", NYMEA_VERSION_STRING);
    handshake.insert("uuid", NymeaCore::instance()->configuration()->serverUuid().toString());
    // "language" is deprecated
    handshake.insert("language", m_clientLocales.value(clientId).name());
    handshake.insert("locale", m_clientLocales.value(clientId).name());
    handshake.insert("protocol version", JSON_PROTOCOL_VERSION);
    handshake.insert("initialSetupRequired", (interface->configuration().authenticationEnabled ? NymeaCore::instance()->userManager()->initRequired() : false));
    handshake.insert("authenticationRequired", interface->configuration().authenticationEnabled);
    handshake.insert("pushButtonAuthAvailable", NymeaCore::instance()->userManager()->pushButtonAuthAvailable());
    if (!m_experiences.isEmpty()) {
        QVariantList experiences;
        foreach (JsonHandler* handler, m_experiences.keys()) {
            QVariantMap experience;
            experience.insert("name", handler->name());
            experience.insert("version", m_experiences.value(handler));
            experiences.append(experience);
        }
        handshake.insert("experiences", experiences);
    }
    QVariantList cacheHashes;
    foreach (const QString &handlerName, m_handlers.keys()) {
        QHash<QString, QString> hashes = m_handlers.value(handlerName)->cacheHashes();
        foreach (const QString &hashName, hashes.keys()) {
            QVariantMap cacheHash;
            cacheHash.insert("method", handlerName + "." + hashName);
            cacheHash.insert("hash", hashes.value(hashName));
            cacheHashes.append(cacheHash);
        }
    }
    if (!cacheHashes.isEmpty()) {
        handshake.insert("cacheHashes", cacheHashes);
    }

    if (!context.token().isEmpty()) {
        TokenInfo tokenInfo = NymeaCore::instance()->userManager()->tokenInfo(context.token());
        UserInfo userInfo = NymeaCore::instance()->userManager()->userInfo(tokenInfo.username());
        handshake.insert("permissionScopes", Types::scopesToStringList(userInfo.scopes()));
        handshake.insert("username", userInfo.username());
    }

    return createReply(handshake);
}

JsonReply* JsonRPCServerImplementation::Introspect(const QVariantMap &params) const
{
    Q_UNUSED(params)
    return createReply(m_api);
}

JsonReply* JsonRPCServerImplementation::Version(const QVariantMap &params) const
{
    Q_UNUSED(params)

    QVariantMap data;
    data.insert("version", NYMEA_VERSION_STRING);
    data.insert("protocol version", JSON_PROTOCOL_VERSION);
    data.insert("qtVersion", qVersion());
    data.insert("qtBuildVersion", QT_VERSION_STR);
    return createReply(data);
}

JsonReply* JsonRPCServerImplementation::SetNotificationStatus(const QVariantMap &params, const JsonContext &context)
{
    QUuid clientId = context.clientId();
    Q_ASSERT_X(m_clientTransports.contains(clientId), "JsonRPCServer", "Invalid client ID.");

    QStringList enabledNamespaces;
    foreach (const QString &namespaceName, m_handlers.keys()) {
        if (params.contains("enabled")) {
            if (params.value("enabled").toBool()) {
                enabledNamespaces.append(namespaceName);
            }
        } else {
            if (params.value("namespaces").toList().contains(namespaceName)) {
                enabledNamespaces.append(namespaceName);
            }
        }
    }
    qCDebug(dcJsonRpc()) << "Notification settings for client" << clientId << ":" << enabledNamespaces;
    m_clientNotifications[clientId] = enabledNamespaces;

    QVariantMap returns;
    returns.insert("namespaces", m_clientNotifications[clientId]);
    // legacy, deprecated
    returns.insert("enabled", m_clientNotifications[clientId].count() > 0);
    return createReply(returns);
}

JsonReply *JsonRPCServerImplementation::CreateUser(const QVariantMap &params)
{
    QString username = params.value("username").toString();
    QString password = params.value("password").toString();

    UserManager::UserError status = NymeaCore::instance()->userManager()->createUser(username, password, Types::PermissionScopeAdmin);

    QVariantMap returns;
    returns.insert("error", enumValueName<UserManager::UserError>(status));
    return createReply(returns);
}

JsonReply *JsonRPCServerImplementation::Authenticate(const QVariantMap &params)
{
    QString username = params.value("username").toString();
    QString password = params.value("password").toString();
    QString deviceName = params.value("deviceName").toString();

    QByteArray token = NymeaCore::instance()->userManager()->authenticate(username, password, deviceName);
    QVariantMap ret;
    ret.insert("success", !token.isEmpty());
    if (!token.isEmpty()) {
        ret.insert("token", token);
        TokenInfo tokenInfo = NymeaCore::instance()->userManager()->tokenInfo(token);
        UserInfo userInfo = NymeaCore::instance()->userManager()->userInfo(tokenInfo.username());
        ret.insert("username", userInfo.username());
        ret.insert("scopes", Types::scopesToStringList(userInfo.scopes()));
    }
    return createReply(ret);
}

JsonReply *JsonRPCServerImplementation::RequestPushButtonAuth(const QVariantMap &params, const JsonContext &context)
{
    QString deviceName = params.value("deviceName").toString();
    QUuid clientId = context.clientId();

    int transactionId = NymeaCore::instance()->userManager()->requestPushButtonAuth(deviceName);
    m_pushButtonTransactions.insert(transactionId, clientId);

    QVariantMap data;
    data.insert("transactionId", transactionId);
    // TODO: return false if pushbutton auth is disabled in settings
    data.insert("success", true);
    return createReply(data);
}

JsonReply *JsonRPCServerImplementation::Tokens(const QVariantMap &params, const JsonContext &context) const
{
    Q_UNUSED(params)

    TokenInfo tokenInfo = NymeaCore::instance()->userManager()->tokenInfo(context.token());
    QList<TokenInfo> tokens = NymeaCore::instance()->userManager()->tokens(tokenInfo.username());
    QVariantList retList;
    foreach (const TokenInfo &tokenInfo, tokens) {
        retList << pack(tokenInfo);
    }
    QVariantMap retMap;
    retMap.insert("tokenInfoList", retList);
    return createReply(retMap);
}

JsonReply *JsonRPCServerImplementation::RemoveToken(const QVariantMap &params)
{
    QUuid tokenId = params.value("tokenId").toUuid();
    UserManager::UserError error = NymeaCore::instance()->userManager()->removeToken(tokenId);
    QVariantMap ret;
    ret.insert("error", enumValueName<UserManager::UserError>(error));
    return createReply(ret);
}

JsonReply *JsonRPCServerImplementation::SetupCloudConnection(const QVariantMap &params)
{
    if (NymeaCore::instance()->cloudManager()->connectionState() != CloudManager::CloudConnectionStateUnconfigured) {
        qCDebug(dcCloud) << "Cloud already configured. Not changing configuration as it won't work anyways. If you want to reconfigure this instance to a different cloud, change the system UUID and wipe the cloud settings from the config.";
        QVariantMap data;
        data.insert("success", false);
        return createReply(data);
    }
    QByteArray rootCA = params.value("rootCA").toByteArray();
    QByteArray certificatePEM = params.value("certificatePEM").toByteArray();
    QByteArray publicKey = params.value("publicKey").toByteArray();
    QByteArray privateKey = params.value("privateKey").toByteArray();
    QString endpoint = params.value("endpoint").toString();
    bool status = NymeaCore::instance()->cloudManager()->installClientCertificates(rootCA, certificatePEM, publicKey, privateKey, endpoint);
    QVariantMap ret;
    ret.insert("success", status);
    return createReply(ret);
}

JsonReply *JsonRPCServerImplementation::SetupRemoteAccess(const QVariantMap &params)
{
    QString idToken = params.value("idToken").toString();
    QString userId = params.value("userId").toString();
    NymeaCore::instance()->cloudManager()->pairDevice(idToken, userId);
    JsonReply *reply = createAsyncReply("SetupRemoteAccess");
    m_pairingRequests.insert(userId, reply);
    connect(reply, &JsonReply::finished, [this, userId](){
        m_pairingRequests.remove(userId);
    });
    return reply;
}

JsonReply *JsonRPCServerImplementation::IsCloudConnected(const QVariantMap &params)
{
    Q_UNUSED(params)
    bool connected = NymeaCore::instance()->cloudManager()->connectionState() == CloudManager::CloudConnectionStateConnected;
    QVariantMap data;
    data.insert("connected", connected);
    data.insert("connectionState", enumValueName<CloudManager::CloudConnectionState>(NymeaCore::instance()->cloudManager()->connectionState()));
    return createReply(data);
}

/*! A client may use this as a ping/pong mechanism to check server connectivity. */
JsonReply *JsonRPCServerImplementation::KeepAlive(const QVariantMap &params)
{
    QString sessionId = params.value("sessionId").toString();
    qCDebug(dcJsonRpc()) << "KeepAlive received" << sessionId;
    QVariantMap resultMap;
    resultMap.insert("success", true);
    resultMap.insert("sessionId", sessionId);
    return createReply(resultMap);
}

/*! Returns the list of registered \l{JsonHandler}{JsonHandlers} and their name.*/
QHash<QString, JsonHandler *> JsonRPCServerImplementation::handlers() const
{
    return m_handlers;
}

/*! Register a new \l{TransportInterface} to the JSON server. If the given interface is already registered, just the authenticationRequired flag will be updated. */
void JsonRPCServerImplementation::registerTransportInterface(TransportInterface *interface, bool authenticationRequired)
{
    if (!m_interfaces.contains(interface)) {
        connect(interface, &TransportInterface::clientConnected, this, &JsonRPCServerImplementation::clientConnected);
        connect(interface, &TransportInterface::clientDisconnected, this, &JsonRPCServerImplementation::clientDisconnected);
        connect(interface, &TransportInterface::dataAvailable, this, &JsonRPCServerImplementation::processData);
        m_interfaces.insert(interface, authenticationRequired);
    } else {
        m_interfaces[interface] = authenticationRequired;
    }
}

void JsonRPCServerImplementation::unregisterTransportInterface(TransportInterface *interface)
{
    disconnect(interface, &TransportInterface::clientConnected, this, &JsonRPCServerImplementation::clientConnected);
    disconnect(interface, &TransportInterface::clientDisconnected, this, &JsonRPCServerImplementation::clientDisconnected);
    disconnect(interface, &TransportInterface::dataAvailable, this, &JsonRPCServerImplementation::processData);
    foreach (const QUuid &clientId, m_clientTransports.keys(interface)) {
        interface->terminateClientConnection(clientId);
        clientDisconnected(clientId);
    }
    m_interfaces.take(interface);
}

bool JsonRPCServerImplementation::registerExperienceHandler(JsonHandler *handler, int majorVersion, int minorVersion)
{
    bool ret = registerHandler(handler);
    if (ret) {
        m_experiences.insert(handler, QString("%1.%2").arg(majorVersion).arg(minorVersion));
    }
    return ret;
}

/*! Send a JSON success response to the client with the given \a clientId,
 * \a commandId and \a params to the inerted \l{TransportInterface}.
 */
void JsonRPCServerImplementation::sendResponse(TransportInterface *interface, const QUuid &clientId, int commandId, const QVariantMap &params, const QString &deprecationWarning)
{
    QVariantMap response;
    response.insert("id", commandId);
    response.insert("status", "success");
    response.insert("params", params);

    if (!deprecationWarning.isEmpty()) {
        response.insert("deprecationWarning", deprecationWarning);
    }

    QByteArray data = QJsonDocument::fromVariant(response).toJson(QJsonDocument::Compact);
    qCDebug(dcJsonRpcTraffic()) << "Sending data:" << data;
    interface->sendData(clientId, data);
}

/*! Send a JSON error response to the client with the given \a clientId,
 * \a commandId and \a error to the inerted \l{TransportInterface}.
 */
void JsonRPCServerImplementation::sendErrorResponse(TransportInterface *interface, const QUuid &clientId, int commandId, const QString &error)
{
    QVariantMap errorResponse;
    errorResponse.insert("id", commandId);
    errorResponse.insert("status", "error");
    errorResponse.insert("error", error);

    QByteArray data = QJsonDocument::fromVariant(errorResponse).toJson(QJsonDocument::Compact);
    qCDebug(dcJsonRpcTraffic()) << "Sending data:" << data;
    interface->sendData(clientId, data);
}

void JsonRPCServerImplementation::sendUnauthorizedResponse(TransportInterface *interface, const QUuid &clientId, int commandId, const QString &error)
{
    QVariantMap errorResponse;
    errorResponse.insert("id", commandId);
    errorResponse.insert("status", "unauthorized");
    errorResponse.insert("error", error);

    QByteArray data = QJsonDocument::fromVariant(errorResponse).toJson(QJsonDocument::Compact);
    qCDebug(dcJsonRpcTraffic()) << "Sending data:" << data;
    interface->sendData(clientId, data);
}

void JsonRPCServerImplementation::setup()
{
    registerHandler(this);
    registerHandler(new IntegrationsHandler(NymeaCore::instance()->thingManager(), this));
    registerHandler(new DeviceHandler(this));
    registerHandler(new ActionHandler(this));
    registerHandler(new RulesHandler(this));
    registerHandler(new EventHandler(this));
    registerHandler(new LoggingHandler(this));
    registerHandler(new StateHandler(this));
    registerHandler(new ConfigurationHandler(this));
    registerHandler(new NetworkManagerHandler(NymeaCore::instance()->networkManager(), this));
    registerHandler(new TagsHandler(this));
    registerHandler(new AppDataHandler(this));
    registerHandler(new SystemHandler(NymeaCore::instance()->platform(), this));
    registerHandler(new UsersHandler(NymeaCore::instance()->userManager(), this));
    registerHandler(new ZigbeeHandler(NymeaCore::instance()->zigbeeManager(), this));
    registerHandler(new ModbusRtuHandler(NymeaCore::instance()->modbusRtuManager(), this));

    connect(NymeaCore::instance()->cloudManager(), &CloudManager::pairingReply, this, &JsonRPCServerImplementation::pairingFinished);
    connect(NymeaCore::instance()->cloudManager(), &CloudManager::connectionStateChanged, this, &JsonRPCServerImplementation::onCloudConnectionStateChanged);
}

void JsonRPCServerImplementation::processData(const QUuid &clientId, const QByteArray &data)
{
    qCDebug(dcJsonRpcTraffic()) << "Incoming data:" << data;

    TransportInterface *interface = qobject_cast<TransportInterface *>(sender());

    // Handle packet fragmentation
    QByteArray buffer = m_clientBuffers[clientId];
    buffer.append(data);
    int splitIndex = buffer.indexOf("}\n{");
    while (splitIndex > -1) {
        processJsonPacket(interface, clientId, buffer.left(splitIndex + 1));
        buffer = buffer.right(buffer.length() - splitIndex - 2);
        splitIndex = buffer.indexOf("}\n{");
    }
    if (buffer.trimmed().endsWith("}")) {
        processJsonPacket(interface, clientId, buffer);
        buffer.clear();
    }
    m_clientBuffers[clientId] = buffer;

    if (buffer.size() > 1024 * 1024) {
        qCWarning(dcJsonRpc()) << "Client buffer larger than 1MB and no valid data. Dropping client connection.";
        interface->terminateClientConnection(clientId);
    }
}

void JsonRPCServerImplementation::processJsonPacket(TransportInterface *interface, const QUuid &clientId, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    if(error.error != QJsonParseError::NoError) {
        qCWarning(dcJsonRpc) << "Failed to parse JSON data" << data << ":" << error.errorString();
        sendErrorResponse(interface, clientId, -1, QString("Failed to parse JSON data: %1").arg(error.errorString()));
        return;
    }

    QVariantMap message = jsonDoc.toVariant().toMap();

    bool success;
    int commandId = message.value("id").toInt(&success);
    if (!success) {
        qCWarning(dcJsonRpc) << "Error parsing command. Missing \"id\":" << message;
        sendErrorResponse(interface, clientId, commandId, "Error parsing command. Missing 'id'");
        return;
    }

    QString methodString = message.value("method").toString();
    QStringList commandList = methodString.split('.');
    if (commandList.count() != 2) {
        qCWarning(dcJsonRpc) << "Error parsing method.\nGot:" << message.value("method").toString() << "\nExpected: \"Namespace.method\"";
        sendErrorResponse(interface, clientId, commandId, QString("Error parsing method. Got: '%1'', Expected: 'Namespace.method'").arg(message.value("method").toString()));
        return;
    }
    QString targetNamespace = commandList.first();
    QString method = commandList.last();

    // check if authentication is required for this transport
    if (m_interfaces.value(interface)) {
        QByteArray token = message.value("token").toByteArray();
        QStringList authExemptMethodsNoUser = {"JSONRPC.Introspect", "JSONRPC.Hello", "JSONRPC.RequestPushButtonAuth", "JSONRPC.CreateUser", "Users.RequestPushButtonAuth", "Users.CreateUser"};
        QStringList authExemptMethodsWithUser = {"JSONRPC.Introspect", "JSONRPC.Hello", "JSONRPC.Authenticate", "JSONRPC.RequestPushButtonAuth", "Users.Authenticate", "Users.RequestPushButtonAuth"};
        // if there is no user in the system yet, let's fail unless this is a special method for authentication itself
        if (NymeaCore::instance()->userManager()->initRequired()) {
            if (!authExemptMethodsNoUser.contains(methodString) && (token.isEmpty() || !NymeaCore::instance()->userManager()->verifyToken(token))) {
                sendUnauthorizedResponse(interface, clientId, commandId, "Initial setup required. Call Users.CreateUser first.");
                qCWarning(dcJsonRpc()) << "Initial setup required but client does not call the setup. Dropping connection.";
                interface->terminateClientConnection(clientId);
                return;
            }
        } else {
            // ok, we have a user. if there isn't a valid token, let's fail unless this is a Authenticate, Introspect  Hello call
            if (!authExemptMethodsWithUser.contains(methodString)) {
                if (token.isEmpty() || !NymeaCore::instance()->userManager()->verifyToken(token)) {
                    sendUnauthorizedResponse(interface, clientId, commandId, "Forbidden: Invalid token.");
                    qCWarning(dcJsonRpc()) << "Client did not not present a valid token. Dropping connection.";
                    interface->terminateClientConnection(clientId);
                    return;
                }
                // Check if the user has the required permissions
                TokenInfo tokenInfo = NymeaCore::instance()->userManager()->tokenInfo(token);
                UserInfo userInfo = NymeaCore::instance()->userManager()->userInfo(tokenInfo.username());
                Types::PermissionScope methodScope = static_cast<Types::PermissionScope>(m_api.value("methods").toMap().value(methodString).toMap().value("permissionScope").toInt());
                if (methodScope != Types::PermissionScopeNone && !userInfo.scopes().testFlag(Types::PermissionScopeAdmin) && !userInfo.scopes().testFlag(methodScope)) {
                    qCWarning(dcJsonRpc()) << "Method" << methodString << "requires" << Types::scopeToString(methodScope) << "but client token has:" << Types::scopesToStringList(userInfo.scopes());
                    sendErrorResponse(interface, clientId, commandId, "Permission denied.");
                    return;
                }
            }
        }
    }
    // At this point we can assume all the calls are authorized

    JsonHandler *handler = m_handlers.value(targetNamespace);
    if (!handler) {
        qCWarning(dcJsonRpc()) << "JSON RPC method called for invalid namespace:" << targetNamespace;
        sendErrorResponse(interface, clientId, commandId, "No such namespace");
        return;
    }
    if (!handler->jsonMethods().contains(method)) {
        qCWarning(dcJsonRpc()) << QString("JSON RPC method called for invalid method: %1.%2").arg(targetNamespace).arg(method);
        sendErrorResponse(interface, clientId, commandId, "No such method");
        return;
    }

    QVariantMap params = message.value("params").toMap();

    QVariantMap definition = handler->jsonMethods().value(method).toMap().value("params").toMap();
    JsonValidator validator;
    JsonValidator::Result validationResult = validator.validateParams(params, targetNamespace + '.' + method, m_api);
    if (!validationResult.success()) {
        qCWarning(dcJsonRpc()) << "JSON RPC parameter verification failed for method" << targetNamespace + '.' + method;
        qCWarning(dcJsonRpc()) << validationResult.errorString() << "in" << validationResult.where();
        qCWarning(dcJsonRpc()) << "Call params:" << qUtf8Printable(QJsonDocument::fromVariant(params).toJson());
        sendErrorResponse(interface, clientId, commandId, "Invalid params: " + validationResult.errorString() + " in " + validationResult.where());
        return;
    }

    if (!(targetNamespace == "JSONRPC" && method == "Hello")) {
        // This is not the handshake message. If we've waited for it, consider this a protocol violation and drop connection
        if (m_newConnectionWaitTimers.contains(clientId)) {
            sendErrorResponse(interface, clientId, commandId, "Handshake required. Call JSONRPC.Hello first.");
            qCWarning(dcJsonRpc()) << "Connection requires a handshake but client did not initiate handshake. Dropping connection";
            interface->terminateClientConnection(clientId);
            return;
        }
    }

    // Attach the transportInterface if this call is for ourselves
    if (handler == this) {
        handler->setProperty("transportInterface", reinterpret_cast<qint64>(interface));
    }

    JsonContext callContext(clientId, m_clientLocales.value(clientId));
    callContext.setToken(message.value("token").toByteArray());

    qCDebug(dcJsonRpc()) << "Invoking method" << targetNamespace + '.' +  method << "from client" << clientId;

    JsonReply *reply;
    if (handler->metaObject()->indexOfMethod(method.toUtf8() + "(QVariantMap,JsonContext)") >= 0) {
        QMetaObject::invokeMethod(handler, method.toUtf8().data(), Q_RETURN_ARG(JsonReply*, reply), Q_ARG(QVariantMap, params), Q_ARG(JsonContext, callContext));
    } else {
        QMetaObject::invokeMethod(handler, method.toUtf8().data(), Q_RETURN_ARG(JsonReply*, reply), Q_ARG(QVariantMap, params));
    }

    if (reply->type() == JsonReply::TypeAsync) {
        m_asyncReplies.insert(reply, interface);
        reply->setClientId(clientId);
        reply->setCommandId(commandId);
        connect(reply, &JsonReply::finished, this, &JsonRPCServerImplementation::asyncReplyFinished);
        reply->startWait();
    } else {
        JsonValidator validator;
        Q_ASSERT_X((targetNamespace == "JSONRPC" && method == "Introspect") || validator.validateReturns(reply->data(), targetNamespace + '.' + method, m_api).success(),
                   validator.result().where().toUtf8(),
                   validator.result().errorString().toUtf8() + "\nReturn value:\n" + QJsonDocument::fromVariant(reply->data()).toJson());

        QString deprecationWarning;
        if (m_api.value("methods").toMap().value(targetNamespace + '.' + method).toMap().contains("deprecated")) {
            deprecationWarning = m_api.value("methods").toMap().value(targetNamespace + '.' + method).toMap().value("deprecated").toString();
            qCWarning(dcJsonRpc()) << "Client uses deprecated API. Please update client implementation!";
            qCWarning(dcJsonRpc()) << targetNamespace + '.' + method + ':' << deprecationWarning;
        }

        sendResponse(interface, clientId, commandId, reply->data(), deprecationWarning);
        reply->deleteLater();
    }
}

void JsonRPCServerImplementation::sendNotification(const QVariantMap &params)
{
    JsonHandler *handler = qobject_cast<JsonHandler *>(sender());
    QMetaMethod method = handler->metaObject()->method(senderSignalIndex());

    QVariantMap notification;
    notification.insert("id", m_notificationId++);
    notification.insert("notification", handler->name() + "." + method.name());

    foreach (const QUuid &clientId, m_clientNotifications.keys()) {

        // Check if this client wants to be notified
        if (!m_clientNotifications.value(clientId).contains(handler->name())) {
            continue;
        }

        // Add deprecation warning if necessary
        if (m_api.value("notifications").toMap().value(handler->name() + '.' + method.name()).toMap().contains("deprecated")) {
            QString deprecationMessage = m_api.value("notifications").toMap().value(handler->name() + '.' + method.name()).toMap().value("deprecated").toString();
            qCWarning(dcJsonRpc()) << "Client" << clientId << "uses deprecated API. Please update client implementation!";
            qCWarning(dcJsonRpc()) << handler->name() + '.' + method.name() + ':' << deprecationMessage;
            notification.insert("deprecationWarning", deprecationMessage);
        }

        QLocale locale = m_clientLocales.value(clientId);
        QVariantMap translatedParams = handler->translateNotification(method.name(), params, locale);

        JsonValidator validator;
        Q_ASSERT_X(validator.validateNotificationParams(translatedParams, handler->name() + '.' + method.name(), m_api).success(),
                   validator.result().where().toUtf8(),
                   validator.result().errorString().toUtf8() + "\nGot:" + QJsonDocument::fromVariant(translatedParams).toJson(QJsonDocument::Indented));

        notification.insert("params", translatedParams);

        QByteArray data = QJsonDocument::fromVariant(notification).toJson(QJsonDocument::Compact);

        qCDebug(dcJsonRpc()) << "Sending notification" << handler->name() + "." + method.name() << "to client" << clientId;
        qCDebug(dcJsonRpcTraffic()) << "Notification content:" << data;

        m_clientTransports.value(clientId)->sendData(clientId, data);
    }
}

void JsonRPCServerImplementation::sendClientNotification(const QUuid &clientId, const QVariantMap &params)
{
    JsonHandler *handler = qobject_cast<JsonHandler *>(sender());
    QMetaMethod method = handler->metaObject()->method(senderSignalIndex());

    if (!m_clientTransports.contains(clientId)) {
        qCWarning(dcJsonRpc()) << "No client with id" << clientId << ". Not sending client notification.";
        return;
    }

    QVariantMap notification;
    notification.insert("id", m_notificationId++);
    notification.insert("notification", handler->name() + "." + method.name());
    notification.insert("params", params);

    JsonValidator validator;
    Q_ASSERT_X(validator.validateNotificationParams(params, handler->name() + '.' + method.name(), m_api).success(),
               validator.result().where().toUtf8(),
               validator.result().errorString().toUtf8() + "\nGot:" + QJsonDocument::fromVariant(params).toJson(QJsonDocument::Indented));

    if (m_api.value("notifications").toMap().value(handler->name() + '.' + method.name()).toMap().contains("deprecated")) {
        QString deprecationMessage = m_api.value("notifications").toMap().value(handler->name() + '.' + method.name()).toMap().value("deprecated").toString();
        qCWarning(dcJsonRpc()) << "Client uses deprecated API. Please update client implementation!";
        qCWarning(dcJsonRpc()) << handler->name() + '.' + method.name() + ':' << deprecationMessage;
        notification.insert("deprecationWarning", deprecationMessage);
    }

    QByteArray data = QJsonDocument::fromVariant(notification).toJson(QJsonDocument::Compact);
    qCDebug(dcJsonRpcTraffic()) << "Notification content:" << data;
    qCDebug(dcJsonRpc()) << "Sending notification:" << handler->name() + "." + method.name();
    m_clientTransports.value(clientId)->sendData(clientId, data);
}

void JsonRPCServerImplementation::asyncReplyFinished()
{
    JsonReply *reply = qobject_cast<JsonReply *>(sender());
    TransportInterface *interface = m_asyncReplies.take(reply);
    if (!interface) {
        qCWarning(dcJsonRpc()) << "Got an async reply but the requesting connection has vanished.";
        reply->deleteLater();
        return;
    }
    if (!reply->timedOut()) {
        JsonValidator validator;
        QString method = reply->handler()->name() + '.' + reply->method();
        Q_ASSERT_X(validator.validateReturns(reply->data(), method, m_api).success()
                   ,validator.result().where().toUtf8()
                   ,validator.result().errorString().toUtf8() + "\nReturn value:\n" + QJsonDocument::fromVariant(reply->data()).toJson());

        QString deprecationWarning;
        if (m_api.value("methods").toMap().value(method).toMap().contains("deprecated")) {
            deprecationWarning = m_api.value("methods").toMap().value(method).toMap().value("deprecated").toString();
            qCWarning(dcJsonRpc()) << "Client uses deprecated API. Please update client implementation!";
            qCWarning(dcJsonRpc()) << method + ':' << deprecationWarning;
        }

        sendResponse(interface, reply->clientId(), reply->commandId(), reply->data(), deprecationWarning);
    } else {
        qCWarning(dcJsonRpc()) << "RPC call timed out:" << reply->handler()->name() << ":" << reply->method();
        sendErrorResponse(interface, reply->clientId(), reply->commandId(), "Command timed out");
    }

    reply->deleteLater();
}

void JsonRPCServerImplementation::pairingFinished(QString cognitoUserId, int status, const QString &message)
{
    JsonReply *reply = m_pairingRequests.take(cognitoUserId);
    if (!reply) {
        return;
    }
    QVariantMap returns;
    returns.insert("status", status);
    returns.insert("message", message);
    reply->setData(returns);
    reply->finished();
}

void JsonRPCServerImplementation::onCloudConnectionStateChanged()
{
    QVariantMap params;
    params.insert("connected", NymeaCore::instance()->cloudManager()->connectionState() == CloudManager::CloudConnectionStateConnected);
    params.insert("connectionState", enumValueName<CloudManager::CloudConnectionState>(NymeaCore::instance()->cloudManager()->connectionState()));
    emit CloudConnectedChanged(params);
}

void JsonRPCServerImplementation::onPushButtonAuthFinished(int transactionId, bool success, const QByteArray &token)
{
    QUuid clientId = m_pushButtonTransactions.take(transactionId);
    if (clientId.isNull()) {
        qCDebug(dcJsonRpc()) << "Received a PushButton reply but wasn't expecting it.";
        return;
    }

    QVariantMap params;
    params.insert("transactionId", transactionId);
    params.insert("success", success);
    if (success) {
        params.insert("token", token);
    }

    emit PushButtonAuthFinished(clientId, params);
}

bool JsonRPCServerImplementation::registerHandler(JsonHandler *handler)
{
    // Sanity checks on API:
    // * Make sure all $ref: entries are valid.
    // * A handler must not register a type name that is already registered by a previously loaded handler with different content.
    QVariantMap methods = m_api.value("methods").toMap();
    QVariantMap notifications = m_api.value("notifications").toMap();
    QVariantMap apiIncludingThis = m_api;

    // Verify enums name clash
    QVariantMap enums = m_api.value("enums").toMap();
    foreach (const QString &enumName, handler->jsonEnums().keys()) {
        QVariantList list = handler->jsonEnums().value(enumName).toList();
        if (enums.contains(enumName) && enums.value(enumName) != list) {
            qCWarning(dcJsonRpc()) << "Enum type" << enumName << "is already registered. Not registering handler" << handler->name();
            return false;
        }
        enums.insert(enumName, list);
    }
    apiIncludingThis["enums"] = enums;

    QVariantMap flags = m_api.value("flags").toMap();
    foreach (const QString &flagName, handler->jsonFlags().keys()) {
        QVariant flagDescription = handler->jsonFlags().value(flagName);
        if (enums.contains(flagName)) {
            qCWarning(dcJsonRpc()) << "Enum with name" << flagName << "is already registered. Not registering handler" << handler->name();
            return false;
        }
        if (flags.contains(flagName) && flags.value(flagName) != handler->jsonFlags().value(flagName)) {
            qCWarning(dcJsonRpc()) << "Flags with name" << flagName << "is already registered. Not registering handler" << handler->name();
            return false;
        }
        flags.insert(flagName, flagDescription);
    }
    apiIncludingThis["flags"] = flags;

    // Verify objects
    QVariantMap existingTypes = m_api.value("types").toMap();
    QVariantMap typesIncludingThis = existingTypes;
    typesIncludingThis.unite(handler->jsonObjects());
    apiIncludingThis["types"] = typesIncludingThis;
    foreach (const QString &objectName, handler->jsonObjects().keys()) {
        QVariantMap object = handler->jsonObjects().value(objectName).toMap();
        // Check for name clashes
        if (existingTypes.contains(objectName) && existingTypes.value(objectName) != handler->jsonObjects().value(objectName)) {
            qCWarning(dcJsonRpc()) << "Object type" << objectName << "is already registered. Not registering handler" << handler->name();
            return false;
        }
        // Check for invalid $ref: entries
        if (!JsonValidator::checkRefs(object, apiIncludingThis)) {
            qCWarning(dcJsonRpc()).nospace() << "Invalid reference in object type " << objectName << ". Not registering handler " << handler->name();
            return false;
        }
    }

    // Verify methods
    QVariantMap newMethods;
    foreach (const QString &methodName, handler->jsonMethods().keys()) {
        QVariantMap method = handler->jsonMethods().value(methodName).toMap();

        if (handler->metaObject()->indexOfMethod(methodName.toUtf8() + "(QVariantMap)") < 0
                && handler->metaObject()->indexOfMethod(methodName.toUtf8() + "(QVariantMap,JsonContext)") < 0) {
            qCWarning(dcJsonRpc()).nospace().noquote() << "Invalid method \"" << methodName << "\". Method \"JsonReply* " + methodName + "(QVariantMap,JsonContext)\" does not exist. Not registering handler " << handler->name();
            return false;
        }
        if (!JsonValidator::checkRefs(method.value("params").toMap(), apiIncludingThis)) {
            qCWarning(dcJsonRpc()).nospace() << "Invalid reference in params of method " << methodName << ". Not registering handler " << handler->name();
            return false;
        }
        if (!JsonValidator::checkRefs(method.value("returns").toMap(), apiIncludingThis)) {
            qCWarning(dcJsonRpc()).nospace() << "Invalid reference in return value of method " << methodName << ". Not registering handler " << handler->name();
            return false;
        }
        newMethods.insert(handler->name() + '.' + methodName, method);
    }
    methods.unite(newMethods);
    apiIncludingThis["methods"] = methods;

    // Verify notifications
    QVariantMap newNotifications;
    foreach (const QString &notificationName, handler->jsonNotifications().keys()) {
        QVariantMap notification = handler->jsonNotifications().value(notificationName).toMap();
        if (!JsonValidator::checkRefs(notification.value("params").toMap(), apiIncludingThis)) {
            qCWarning(dcJsonRpc()).nospace() << "Invalid reference in params of notification " << notificationName << ". Not registering handler " << handler->name();
            return false;
        }
        newNotifications.insert(handler->name() + '.' + notificationName, notification);
    }
    notifications.unite(newNotifications);
    apiIncludingThis["notifications"] = notifications;

    // Checks completed. Store new API
    qCDebug(dcJsonRpc()) << "Registering JSON RPC handler:" << handler->name();
    m_api = apiIncludingThis;

    m_handlers.insert(handler->name(), handler);
    for (int i = 0; i < handler->metaObject()->methodCount(); ++i) {
        QMetaMethod method = handler->metaObject()->method(i);
        if (method.methodType() == QMetaMethod::Signal && QString(method.name()).contains(QRegExp("^[A-Z]"))) {
            if (method.parameterCount() == 1 && method.parameterType(0) == QVariant::Map) {
                QObject::connect(handler, method, this, metaObject()->method(metaObject()->indexOfSlot("sendNotification(QVariantMap)")));
            } else if (method.parameterCount() == 2 && method.parameterType(0) == QVariant::Uuid && method.parameterType(1) == QVariant::Map) {
                QObject::connect(handler, method, this, metaObject()->method(metaObject()->indexOfSlot("sendClientNotification(QUuid,QVariantMap)")));
            }
        }
    }
    return true;
}

void JsonRPCServerImplementation::clientConnected(const QUuid &clientId)
{
    qCDebug(dcJsonRpc()) << "Client connected with uuid" << clientId.toString();
    TransportInterface *interface = qobject_cast<TransportInterface *>(sender());

    m_clientTransports.insert(clientId, interface);

    // Initialize the connection locale to the settings default
    m_clientLocales.insert(clientId, NymeaCore::instance()->configuration()->locale());

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, timer, clientId, interface](){
        qCDebug(dcJsonRpc()) << "Client" << clientId << "did not initiate the handshake within the required timeout. Dropping connection.";
        timer->deleteLater();
        m_newConnectionWaitTimers.remove(clientId);
        interface->terminateClientConnection(clientId);
    });
    m_newConnectionWaitTimers.insert(clientId, timer);
    timer->start(10000);
}

void JsonRPCServerImplementation::clientDisconnected(const QUuid &clientId)
{
    qCDebug(dcJsonRpc()) << "Client disconnected:" << clientId;
    m_clientTransports.remove(clientId);
    m_clientNotifications.remove(clientId);
    m_clientBuffers.remove(clientId);
    m_clientLocales.remove(clientId);
    if (m_pushButtonTransactions.values().contains(clientId)) {
        NymeaCore::instance()->userManager()->cancelPushButtonAuth(m_pushButtonTransactions.key(clientId));
    }
    if (m_newConnectionWaitTimers.contains(clientId)) {
        delete m_newConnectionWaitTimers.take(clientId);
    }
}

}
