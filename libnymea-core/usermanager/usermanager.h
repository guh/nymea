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

#ifndef USERMANAGER_H
#define USERMANAGER_H

#include "tokeninfo.h"
#include "userinfo.h"

#include <QObject>
#include <QSqlDatabase>

namespace nymeaserver {

class PushButtonDBusService;

class UserManager : public QObject
{
    Q_OBJECT
public:
    enum UserError {
        UserErrorNoError,
        UserErrorBackendError,
        UserErrorInvalidUserId,
        UserErrorDuplicateUserId,
        UserErrorBadPassword,
        UserErrorTokenNotFound,
        UserErrorPermissionDenied
    };
    Q_ENUM(UserError)

    explicit UserManager(const QString &dbName, QObject *parent = nullptr);

    bool initRequired() const;
    UserInfoList users() const;

    UserError createUser(const QString &username, const QString &password, Types::PermissionScopes scopes);
    UserError changePassword(const QString &username, const QString &newPassword);
    UserError removeUser(const QString &username);
    UserError setUserScopes(const QString &username, Types::PermissionScopes scopes);

    bool pushButtonAuthAvailable() const;

    QByteArray authenticate(const QString &username, const QString &password, const QString &deviceName);
    int requestPushButtonAuth(const QString &deviceName);
    void cancelPushButtonAuth(int transactionId);

    UserInfo userInfo(const QString &username = QString()) const;
    TokenInfo tokenInfo(const QByteArray &token) const;
    TokenInfo tokenInfo(const QUuid &tokenId) const;
    QList<TokenInfo> tokens(const QString &username) const;

    UserError removeToken(const QUuid &tokenId);


    bool verifyToken(const QByteArray &token);

signals:
    void userAdded(const QString &username);
    void userRemoved(const QString &username);
    void userChanged(const QString &username);
    void pushButtonAuthFinished(int transactionId, bool success, const QByteArray &token);

private:
    bool initDB();
    void rotate(const QString &dbName);
    bool validateUsername(const QString &username) const;
    bool validatePassword(const QString &password) const;
    bool validateToken(const QByteArray &token) const;

    void dumpDBError(const QString &message);

private slots:
    void onPushButtonPressed();

private:
    QSqlDatabase m_db;
    PushButtonDBusService *m_pushButtonDBusService = nullptr;
    int m_pushButtonTransactionIdCounter = 0;
    QPair<int, QString> m_pushButtonTransaction;

};
}
Q_DECLARE_METATYPE(nymeaserver::UserManager::UserError)

#endif // USERMANAGER_H
