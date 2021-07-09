#include "userinfo.h"

#include <QMetaEnum>

namespace nymeaserver {

UserInfo::UserInfo()
{

}

UserInfo::UserInfo(const QString &username):
    m_username(username)
{

}

QString UserInfo::username() const
{
    return m_username;
}

void UserInfo::setUsername(const QString &username)
{
    m_username = username;
}

Types::PermissionScopes UserInfo::scopes() const
{
    return m_scopes;
}

void UserInfo::setScopes(Types::PermissionScopes scopes)
{
    m_scopes = scopes;
}

QVariant UserInfoList::get(int index) const
{
    return QVariant::fromValue(at(index));
}

void UserInfoList::put(const QVariant &variant)
{
    append(variant.value<UserInfo>());
}

}
