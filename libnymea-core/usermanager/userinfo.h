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

#ifndef USERINFO_H
#define USERINFO_H

#include <QUuid>
#include <QObject>
#include <QVariant>
#include "typeutils.h"

namespace nymeaserver {

class UserInfo
{
    Q_GADGET
    Q_PROPERTY(QString username READ username)
    Q_PROPERTY(Types::PermissionScopes scopes READ scopes)

public:
    UserInfo();
    UserInfo(const QString &username);

    QString username() const;
    void setUsername(const QString &username);

    Types::PermissionScopes scopes() const;
    void setScopes(Types::PermissionScopes scopes);

private:
    QString m_username;
    Types::PermissionScopes m_scopes = Types::PermissionScopeNone;
};

class UserInfoList: public QList<UserInfo>
{
    Q_GADGET
    Q_PROPERTY(int count READ count)
public:
    Q_INVOKABLE QVariant get(int index) const;
    Q_INVOKABLE void put(const QVariant &variant);
};
}
#endif // USERINFO_H
