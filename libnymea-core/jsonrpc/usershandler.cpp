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
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "usershandler.h"
#include "usermanager/usermanager.h"
#include "usermanager/userinfo.h"

#include "loggingcategories.h"

namespace nymeaserver {

UsersHandler::UsersHandler(UserManager *userManager, QObject *parent):
    JsonHandler(parent),
    m_userManager(userManager)
{
    registerEnum<Types::PermissionScope, Types::PermissionScopes>();
    registerObject<UserInfo, UserInfoList>();
    registerObject<TokenInfo, TokenInfoList>();

    QVariantMap params, returns;
    QString description;

    params.clear(); returns.clear();
    description = "Create a new user in the API with the given username and password. Use scopes to define the permissions for the new user. If no scopes are given, this user will be an admin user. Call Authenticate after this to obtain a device token for this user.";
    params.insert("username", enumValueName(String));
    params.insert("password", enumValueName(String));
    params.insert("o:scopes", enumRef<Types::PermissionScopes>());
    returns.insert("error", enumRef<UserManager::UserError>());
    registerMethod("CreateUser", description, params, returns);

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
    registerMethod("Authenticate", description, params, returns);

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
    registerMethod("RequestPushButtonAuth", description, params, returns);

    params.clear(); returns.clear();
    description = "Change the password for the currently logged in user.";
    params.insert("newPassword", enumValueName(String));
    returns.insert("error", enumRef<UserManager::UserError>());
    registerMethod("ChangePassword", description, params, returns);

    params.clear(); returns.clear();
    description = "Get info about the current token (the currently logged in user).";
    returns.insert("o:userInfo", objectRef<UserInfo>());
    returns.insert("error", enumRef<UserManager::UserError>());
    registerMethod("GetUserInfo", description, params, returns, Types::PermissionScopeNone);

    params.clear(); returns.clear();
    description = "Get all the tokens for the current user.";
    returns.insert("o:tokenInfoList", objectRef<TokenInfoList>());
    returns.insert("error", enumRef<UserManager::UserError>());
    registerMethod("GetTokens", description, params, returns);

    params.clear(); returns.clear();
    description = "Revoke access for a given token.";
    params.insert("tokenId", enumValueName(Uuid));
    returns.insert("error", enumRef<UserManager::UserError>());
    registerMethod("RemoveToken", description, params, returns);

    params.clear(); returns.clear();
    description = "Return a list of all users in the system.";
    returns.insert("users", objectRef<UserInfoList>());
    registerMethod("GetUsers", description, params, returns);

    params.clear(); returns.clear();
    description = "Remove a user from the system.";
    params.insert("username", enumValueName(String));
    returns.insert("error", enumRef<UserManager::UserError>());
    registerMethod("RemoveUser", description, params, returns);

    params.clear(); returns.clear();
    description = "Set the permissions (scopes) for a given user.";
    params.insert("username", enumValueName(String));
    params.insert("scopes", enumRef<Types::PermissionScopes>());
    returns.insert("error", enumRef<UserManager::UserError>());
    registerMethod("SetUserScopes", description, params, returns);

    // Notifications
    params.clear();
    description = "Emitted when a user is added to the system.";
    params.insert("userInfo", objectRef<UserInfo>());
    registerNotification("UserAdded", description, params);

    params.clear();
    description = "Emitted when a user is removed from the system.";
    params.insert("username", enumValueName(String));
    registerNotification("UserRemoved", description, params);

    params.clear();
    description = "Emitted whenever a user is changed.";
    params.insert("userInfo", objectRef<UserInfo>());
    registerNotification("UserChanged", description, params);

    params.clear();
    description = "Emitted when a push button authentication reaches final state. NOTE: This notification is "
                  "special. It will only be emitted to connections that did actively request a push button "
                  "authentication, but also it will be emitted regardless of the notification settings.";
    params.insert("success", enumValueName(Bool));
    params.insert("transactionId", enumValueName(Int));
    params.insert("o:token", enumValueName(String));
    registerNotification("PushButtonAuthFinished", description, params);

    connect(m_userManager, &UserManager::userAdded, this, [this](const QString &username){
        QVariantMap params;
        params.insert("userInfo", pack(m_userManager->userInfo(username)));
        emit UserAdded(params);
    });
    connect(m_userManager, &UserManager::userChanged, this, [this](const QString &username){
        QVariantMap params;
        params.insert("userInfo", pack(m_userManager->userInfo(username)));
        emit UserChanged(params);
    });
    connect(m_userManager, &UserManager::userRemoved, this, [this](const QString &username){
        QVariantMap params;
        params.insert("username", username);
        emit UserRemoved(params);
    });
    connect(m_userManager, &UserManager::pushButtonAuthFinished, this, &UsersHandler::onPushButtonAuthFinished);

}

QString UsersHandler::name() const
{
    return "Users";
}

JsonReply *UsersHandler::CreateUser(const QVariantMap &params)
{
    QString username = params.value("username").toString();
    QString password = params.value("password").toString();
    QStringList scopesList = params.value("scopes", Types::scopesToStringList(Types::PermissionScopeAdmin)).toStringList();
    Types::PermissionScopes scopes = Types::scopesFromStringList(scopesList);

    UserManager::UserError status = m_userManager->createUser(username, password, scopes);

    QVariantMap returns;
    returns.insert("error", enumValueName<UserManager::UserError>(status));
    return createReply(returns);
}

JsonReply *UsersHandler::ChangePassword(const QVariantMap &params, const JsonContext &context)
{
    QVariantMap ret;

    QByteArray currentToken = context.token();
    if (currentToken.isEmpty()) {
        qCWarning(dcJsonRpc()) << "Cannot change password from an unauthenticated connection";
        ret.insert("error", enumValueName<UserManager::UserError>(UserManager::UserErrorPermissionDenied));
        return createReply(ret);
    }
    if (!m_userManager->verifyToken(currentToken)) {
        // Might happen if the client is connecting via an unauthenticated connection but tries to sneak in an invalid token
        qCWarning(dcJsonRpc()) << "Invalid token. Is this an unauthenticated connection?";
        ret.insert("error", enumValueName<UserManager::UserError>(UserManager::UserErrorPermissionDenied));
        return createReply(ret);
    }

    QString newPassword = params.value("newPassword").toString();
    QString username = m_userManager->userInfo(currentToken).username();

    UserManager::UserError status = m_userManager->changePassword(username, newPassword);
    ret.insert("error", enumValueName<UserManager::UserError>(status));
    return createReply(ret);
}

JsonReply *UsersHandler::Authenticate(const QVariantMap &params)
{
    QString username = params.value("username").toString();
    QString password = params.value("password").toString();
    QString deviceName = params.value("deviceName").toString();

    QByteArray token = m_userManager->authenticate(username, password, deviceName);
    QVariantMap ret;
    ret.insert("success", !token.isEmpty());
    if (!token.isEmpty()) {
        ret.insert("token", token);
    }
    return createReply(ret);
}

JsonReply *UsersHandler::RequestPushButtonAuth(const QVariantMap &params, const JsonContext &context)
{
    QString deviceName = params.value("deviceName").toString();

    int transactionId = m_userManager->requestPushButtonAuth(deviceName);
    m_pushButtonTransactions.insert(transactionId, context.clientId());

    QVariantMap data;
    data.insert("transactionId", transactionId);
    // TODO: return false if pushbutton auth is disabled in settings
    data.insert("success", true);
    return createReply(data);
}

JsonReply *UsersHandler::GetUserInfo(const QVariantMap &params, const JsonContext &context)
{
    Q_UNUSED(params)
    QVariantMap ret;

    QByteArray currentToken = context.token();
    if (currentToken.isEmpty()) {
        qCWarning(dcJsonRpc()) << "Cannot get user info from an unauthenticated connection";
        ret.insert("error", enumValueName<UserManager::UserError>(UserManager::UserErrorPermissionDenied));
        return createReply(ret);
    }
    if (!m_userManager->verifyToken(currentToken)) {
        // Might happen if the client is connecting via an unauthenticated connection but tries to sneak in an invalid token
        qCWarning(dcJsonRpc()) << "Invalid token. Is this an unauthenticated connection?";
        ret.insert("error", enumValueName<UserManager::UserError>(UserManager::UserErrorPermissionDenied));
        return createReply(ret);
    }

    TokenInfo tokenInfo = m_userManager->tokenInfo(currentToken);

    UserInfo userInfo = m_userManager->userInfo(tokenInfo.username());
    ret.insert("userInfo", pack(userInfo));
    ret.insert("error", enumValueName<UserManager::UserError>(UserManager::UserErrorNoError));
    return createReply(ret);
}

JsonReply *UsersHandler::GetTokens(const QVariantMap &params, const JsonContext &context)
{
    Q_UNUSED(params)
    QVariantMap ret;

    QByteArray currentToken = context.token();
    if (currentToken.isEmpty()) {
        qCWarning(dcJsonRpc()) << "Cannot fetch tokens form an unauthenticated connection";
        ret.insert("error", enumValueName<UserManager::UserError>(UserManager::UserErrorPermissionDenied));
        return createReply(ret);
    }
    if (!m_userManager->verifyToken(currentToken)) {
        // Might happen if the client is connecting via an unauthenticated connection but tries to sneak in an invalid token
        qCWarning(dcJsonRpc()) << "Invalid token. Is this an unauthenticated connection?";
        ret.insert("error", enumValueName<UserManager::UserError>(UserManager::UserErrorPermissionDenied));
        return createReply(ret);
    }

    TokenInfo tokenInfo = m_userManager->tokenInfo(currentToken);
    qCDebug(dcJsonRpc()) << "Fetching tokens for user" << tokenInfo.username();
    QList<TokenInfo> tokens = m_userManager->tokens(tokenInfo.username());
    QVariantList retList;
    foreach (const TokenInfo &tokenInfo, tokens) {
        retList << pack(tokenInfo);
    }
    ret.insert("tokenInfoList", retList);
    ret.insert("error", enumValueName<UserManager::UserError>(UserManager::UserErrorNoError));
    return createReply(ret);
}

JsonReply *UsersHandler::RemoveToken(const QVariantMap &params, const JsonContext &context)
{
    QVariantMap ret;

    QByteArray currentToken = context.token();
    if (currentToken.isEmpty()) {
        qCWarning(dcJsonRpc()) << "Cannot remove a token from an unauthenticated connection.";
        ret.insert("error", enumValueName<UserManager::UserError>(UserManager::UserErrorPermissionDenied));
        return createReply(ret);
    }
    if (!m_userManager->verifyToken(currentToken)) {
        // Might happen if the client is connecting via an unauthenticated connection but tries to sneak in an invalid token
        qCWarning(dcJsonRpc()) << "Invalid token. Is this an unauthenticated connection?";
        ret.insert("error", enumValueName<UserManager::UserError>(UserManager::UserErrorPermissionDenied));
        return createReply(ret);
    }
    QUuid tokenId = params.value("tokenId").toUuid();
    TokenInfo tokenToRemove = m_userManager->tokenInfo(tokenId);
    if (tokenToRemove.id().isNull()) {
        qCWarning(dcJsonRpc()) << "Token with ID" << tokenId << "not found";
        ret.insert("error", enumValueName<UserManager::UserError>(UserManager::UserErrorTokenNotFound));
        return createReply(ret);
    }

    TokenInfo currentTokenInfo = m_userManager->tokenInfo(currentToken);
    if (currentTokenInfo.username() != tokenToRemove.username()) {
        qCWarning(dcJsonRpc()) << "Cannot remove a token from another user!";
        ret.insert("error", enumValueName<UserManager::UserError>(UserManager::UserErrorPermissionDenied));
        return createReply(ret);
    }

    qCDebug(dcJsonRpc()) << "Removing token" << tokenId << "for user" << currentTokenInfo.username();

    UserManager::UserError error = m_userManager->removeToken(tokenId);
    ret.insert("error", enumValueName<UserManager::UserError>(error));
    return createReply(ret);
}

void UsersHandler::onPushButtonAuthFinished(int transactionId, bool success, const QByteArray &token)
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

JsonReply *UsersHandler::GetUsers(const QVariantMap &params)
{
    Q_UNUSED(params)
    QVariantMap reply;
    reply.insert("users", pack(m_userManager->users()));
    return createReply(reply);
}

JsonReply *UsersHandler::RemoveUser(const QVariantMap &params, const JsonContext &context)
{
    Q_UNUSED(context)
    QString username = params.value("username").toString();
    QVariantMap returns;
    UserManager::UserError error = m_userManager->removeUser(username);
    returns.insert("error", enumValueName<UserManager::UserError>(error));
    return createReply(returns);
}

JsonReply *UsersHandler::SetUserScopes(const QVariantMap &params, const JsonContext &context)
{
    Q_UNUSED(context)
    QString username = params.value("username").toString();
    Types::PermissionScopes scopes = Types::scopesFromStringList(params.value("scopes").toStringList());
    UserManager::UserError error = m_userManager->setUserScopes(username, scopes);
    QVariantMap returns;
    returns.insert("error", enumValueName<UserManager::UserError>(error));
    return createReply(returns);
}

}
