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

#ifndef JSONHANDLER_H
#define JSONHANDLER_H

#include <QObject>
#include <QVariantMap>
#include <QMetaMethod>
#include <QDebug>
#include <QVariant>
#include <QDateTime>

#include "jsonreply.h"
#include "jsoncontext.h"
#include "typeutils.h"

class JsonHandler : public QObject
{
    Q_OBJECT
public:
    enum BasicType {
        Uuid,
        String,
        StringList,
        Int,
        Uint,
        Double,
        Bool,
        Variant,
        Color,
        Time,
        Object
    };
    Q_ENUM(BasicType)

    explicit JsonHandler(QObject *parent = nullptr);
    virtual ~JsonHandler() = default;

    virtual QString name() const = 0;
    virtual QHash<QString, QString> cacheHashes() const;

    virtual QVariantMap translateNotification(const QString &notification, const QVariantMap &params, const QLocale &locale);

    QVariantMap jsonEnums() const;
    QVariantMap jsonFlags() const;
    QVariantMap jsonObjects() const;
    QVariantMap jsonMethods() const;
    QVariantMap jsonNotifications() const;


    template<typename T> static QString enumRef();
    template<typename T> static QString objectRef();
    static QString objectRef(const QString &objectName);

    template<typename T> static QString enumValueName(T value);
    template<typename T> static T enumNameToValue(const QString &name);

    static BasicType variantTypeToBasicType(QVariant::Type variantType);
    static QVariant::Type basicTypeToVariantType(BasicType basicType);

    template<typename T> QVariant pack(const T &value) const;
    template<typename T> QVariant pack(T *value) const;
    template <typename T> T unpack(const QVariant &value) const;

protected:
    template <typename Enum> void registerEnum();
    template <typename Enum, typename Flags> void registerEnum();

    template <typename ObjectType> void registerObject();
    template <typename ObjectType, typename ListType> void registerObject();

    template <typename ObjectType> void registerUncreatableObject();
    template <typename ObjectType, typename ListType> void registerUncreatableObject();

    template<typename ListType, typename BasicTypeName> void registerList(BasicTypeName typeName);

    // Deprecated QString based registerObject
    void registerObject(const QString &name, const QVariantMap &object);


    void registerMethod(const QString &name, const QString &description, const QVariantMap &params, const QVariantMap &returns, Types::PermissionScope permissionScope = Types::PermissionScopeAdmin, const QString &deprecationInfo = QString());
    void registerNotification(const QString &name, const QString &description, const QVariantMap &params, const QString &deprecationInfo = QString());

    JsonReply *createReply(const QVariantMap &data) const;
    JsonReply *createAsyncReply(const QString &method) const;

private:
    void registerObject(const QMetaObject &metaObject);
    void registerObject(const QMetaObject &metaObject, const QMetaObject &listMetaObject);

    QVariant pack(const QMetaObject &metaObject, const void *gadget) const;
    QVariant unpack(const QMetaObject &metaObject, const QVariant &value) const;

private:
    QVariantMap m_enums;
    QHash<QString, QMetaEnum> m_metaEnums;
    QVariantMap m_flags;
    QHash<QString, QMetaEnum> m_metaFlags;
    QHash<QString, QString> m_flagsEnums;
    QVariantMap m_objects;
    QHash<QString, QMetaObject> m_metaObjects;
    QHash<QString, QMetaObject> m_listMetaObjects;
    QHash<QString, QString> m_listEntryTypes;
    QVariantMap m_methods;
    QVariantMap m_notifications;
};
Q_DECLARE_METATYPE(QVariant::Type)

template<typename T>
void JsonHandler::registerEnum()
{
    QMetaEnum metaEnum = QMetaEnum::fromType<T>();
    QStringList values;
    for (int i = 0; i < metaEnum.keyCount(); i++) {
        values << metaEnum.key(i);
    }
    m_enums.insert(metaEnum.name(), values);
    m_metaEnums.insert(metaEnum.name(), metaEnum);
}

template<typename Enum, typename Flags>
void JsonHandler::registerEnum()
{
    registerEnum<Enum>();
    QMetaEnum metaEnum = QMetaEnum::fromType<Enum>();
    QMetaEnum metaFlags = QMetaEnum::fromType<Flags>();
    m_metaFlags.insert(metaFlags.name(), metaFlags);
    m_flagsEnums.insert(metaFlags.name(), metaEnum.name());
    m_flags.insert(metaFlags.name(), QVariantList() << QString("$ref:%1").arg(metaEnum.name()));
}

template<typename ObjectType>
void JsonHandler::registerObject()
{
    qRegisterMetaType<ObjectType>();
    QMetaObject metaObject = ObjectType::staticMetaObject;
    registerObject(metaObject);
}

template<typename ObjectType, typename ListType>
void JsonHandler::registerObject()
{
    qRegisterMetaType<ObjectType>();
    qRegisterMetaType<ListType>();
    QMetaObject metaObject = ObjectType::staticMetaObject;
    QMetaObject listMetaObject = ListType::staticMetaObject;
    registerObject(metaObject, listMetaObject);
}

template<typename ObjectType>
void JsonHandler::registerUncreatableObject()
{
    QMetaObject metaObject = ObjectType::staticMetaObject;
    registerObject(metaObject);
}

template<typename ObjectType, typename ListType >
void JsonHandler::registerUncreatableObject()
{
    QMetaObject metaObject = ObjectType::staticMetaObject;
    QMetaObject listMetaObject = ListType::staticMetaObject;
    registerObject(metaObject, listMetaObject);
}

template<typename ListType, typename BasicTypeName>
void JsonHandler::registerList(BasicTypeName typeName)
{
    QMetaObject listMetaObject = ListType::staticMetaObject;
    QString listTypeName = QString(listMetaObject.className()).split("::").last();
    m_metaObjects.insert(listTypeName, listMetaObject);
    m_objects.insert(listTypeName, QVariantList() << QVariant(QString("$ref:%1").arg(enumValueName(typeName))));
    Q_ASSERT_X(listMetaObject.indexOfProperty("count") >= 0, "JsonHandler", QString("List type %1 does not implement \"count\" property!").arg(listTypeName).toUtf8());
    Q_ASSERT_X(listMetaObject.indexOfMethod("get(int)") >= 0, "JsonHandler", QString("List type %1 does not implement \"Q_INVOKABLE QVariant get(int index)\" method!").arg(listTypeName).toUtf8());
    Q_ASSERT_X(listMetaObject.indexOfMethod("put(QVariant)") >= 0, "JsonHandler", QString("List type %1 does not implement \"Q_INVOKABLE void put(QVariant variant)\" method!").arg(listTypeName).toUtf8());
}

template<typename T>
QString JsonHandler::enumRef()
{
    QMetaEnum metaEnum = QMetaEnum::fromType<T>();
    return QString("$ref:%1").arg(metaEnum.name());
}

template<typename T>
QString JsonHandler::objectRef()
{
    QMetaObject metaObject = T::staticMetaObject;
    return QString("$ref:%1").arg(QString(metaObject.className()).split("::").last());
}

template<typename T>
QString JsonHandler::enumValueName(T value)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<T>();
    return metaEnum.valueToKey(value);
}

template<typename T>
T JsonHandler::enumNameToValue(const QString &name)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<T>();
    return static_cast<T>(metaEnum.keyToValue(name.toUtf8()));
}

template<typename T>
QVariant JsonHandler::pack(const T &value) const
{
    QMetaObject metaObject = T::staticMetaObject;
    return pack(metaObject, static_cast<const void*>(&value));
}

template<typename T>
QVariant JsonHandler::pack(T *value) const
{
    QMetaObject metaObject = T::staticMetaObject;
    return pack(metaObject, static_cast<const void*>(value));
}

template<typename T>
T JsonHandler::unpack(const QVariant &value) const
{
    QMetaObject metaObject = T::staticMetaObject;
    QVariant ret = unpack(metaObject, value);
    return ret.value<T>();
}


#endif // JSONHANDLER_H
