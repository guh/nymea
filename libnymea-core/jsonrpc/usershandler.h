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

#ifndef USERSHANDLER_H
#define USERSHANDLER_H

#include <QObject>

#include "jsonrpc/jsonhandler.h"

namespace nymeaserver {

class UserManager;

class UsersHandler : public JsonHandler
{
    Q_OBJECT
public:
    explicit UsersHandler(UserManager *userManager, QObject *parent = nullptr);

    QString name() const override;

    Q_INVOKABLE JsonReply *CreateUser(const QVariantMap &params);
    Q_INVOKABLE JsonReply *ChangePassword(const QVariantMap &params, const JsonContext &context);
    Q_INVOKABLE JsonReply *Authenticate(const QVariantMap &params);
    Q_INVOKABLE JsonReply *RequestPushButtonAuth(const QVariantMap &params, const JsonContext &context);
    Q_INVOKABLE JsonReply *GetUserInfo(const QVariantMap &params, const JsonContext &context);
    Q_INVOKABLE JsonReply *GetTokens(const QVariantMap &params, const JsonContext &context);
    Q_INVOKABLE JsonReply *RemoveToken(const QVariantMap &params, const JsonContext &context);
    Q_INVOKABLE JsonReply *GetUsers(const QVariantMap &params);
    Q_INVOKABLE JsonReply *RemoveUser(const QVariantMap &params, const JsonContext &context);
    Q_INVOKABLE JsonReply *SetUserScopes(const QVariantMap &params, const JsonContext &context);

signals:
    void PushButtonAuthFinished(const QUuid &clientId, const QVariantMap &params);
    void UserAdded(const QVariantMap &params);
    void UserRemoved(const QVariantMap &params);
    void UserChanged(const QVariantMap &params);

private slots:
    void onPushButtonAuthFinished(int transactionId, bool success, const QByteArray &token);

private:
    UserManager *m_userManager = nullptr;
    QHash<int, QUuid> m_pushButtonTransactions;

};

}

#endif // USERSHANDLER_H
