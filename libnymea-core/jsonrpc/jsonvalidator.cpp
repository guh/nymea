/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Michael Zanetti <michael.zanetti@nymea.io>          *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "jsonvalidator.h"
#include "jsonrpc/jsonhandler.h"

#include "loggingcategories.h"

#include <QJsonDocument>
#include <QColor>
#include <QDateTime>

namespace nymeaserver {

bool JsonValidator::checkRefs(const QVariantMap &map, const QVariantMap &types)
{
    foreach (const QString &key, map.keys()) {
        if (map.value(key).toString().startsWith("$ref:")) {
            QString refName = map.value(key).toString().remove("$ref:");
            if (!types.contains(refName)) {
                qCWarning(dcJsonRpc()) << "Invalid reference to" << refName;
                return false;
            }
        }
        if (map.value(key).type() == QVariant::Map) {
            bool ret = checkRefs(map.value(key).toMap(), types);
            if (!ret) {
                return false;
            }
        }
        if (map.value(key).type() == QVariant::List) {
            foreach (const QVariant &entry, map.value(key).toList()) {
                if (entry.toString().startsWith("$ref:")) {
                    QString refName = entry.toString().remove("$ref:");
                    if (!types.contains(refName)) {
                        qCWarning(dcJsonRpc()) << "Invalid reference to" << refName;
                        return false;
                    }
                }
                if (entry.type() == QVariant::Map) {
                    bool ret = checkRefs(map.value(key).toMap(), types);
                    if (!ret) {
                        return false;
                    }
                }
            }
        }
    }
    return true;

}

JsonValidator::Result JsonValidator::validateParams(const QVariantMap &params, const QString &method, const QVariantMap &api)
{
    QVariantMap paramDefinition = api.value("methods").toMap().value(method).toMap().value("params").toMap();
    m_result = validateMap(params, paramDefinition, api.value("types").toMap());
    m_result.setWhere(method + ", param " + m_result.where());
    return m_result;
}

JsonValidator::Result JsonValidator::validateReturns(const QVariantMap &returns, const QString &method, const QVariantMap &api)
{
    QVariantMap returnsDefinition = api.value("methods").toMap().value(method).toMap().value("returns").toMap();
    m_result = validateMap(returns, returnsDefinition, api.value("types").toMap());
    m_result.setWhere(method + ", returns " + m_result.where());
    return m_result;
}

JsonValidator::Result JsonValidator::validateNotificationParams(const QVariantMap &params, const QString &notification, const QVariantMap &api)
{
    QVariantMap paramDefinition = api.value("notifications").toMap().value(notification).toMap().value("params").toMap();
    m_result = validateMap(params, paramDefinition, api.value("types").toMap());
    m_result.setWhere(notification + ", param " + m_result.where());
    return m_result;
}

JsonValidator::Result JsonValidator::result() const
{
    return m_result;
}

JsonValidator::Result JsonValidator::validateMap(const QVariantMap &map, const QVariantMap &definition, const QVariantMap &types)
{
    // Make sure all required values are available
    foreach (const QString &key, definition.keys()) {
        if (key.startsWith("o:")) {
            continue;
        }
        if (!map.contains(key)) {
            return Result(false, "Missing required key: " + key, key);
        }
    }

    // Make sure given values are valid
    foreach (const QString &key, map.keys()) {
        // Is the key allowed in here?
        QVariant expectedValue = definition.value(key);
        if (!expectedValue.isValid()) {
            expectedValue = definition.value("o:" + key);
        }
        if (!expectedValue.isValid()) {
            return Result(false, "Invalid key: " + key);
        }

        // Validate content
        QVariant value = map.value(key);

        Result result = validateEntry(value, expectedValue, types);
        if (!result.success()) {
            result.setWhere(key + '.' + result.where());
            result.setErrorString(result.errorString());
            return result;
        }

   }

    return Result(true);
}

JsonValidator::Result JsonValidator::validateEntry(const QVariant &value, const QVariant &definition, const QVariantMap &types)
{
    if (definition.type() == QVariant::String) {
        QString expectedTypeName = definition.toString();

        if (expectedTypeName.startsWith("$ref:")) {
            QString refName = expectedTypeName;
            refName.remove("$ref:");

            QVariant refDefinition = types.value(refName);
            // Refs might be enums
            if (refDefinition.type() == QVariant::List) {
                if (value.type() != QVariant::String) {
                    return Result(false, "Expected enum " + refName + " but got " + value.toString());
                }
                QVariantList enumList = refDefinition.toList();
                if (!enumList.contains(value.toString())) {
                    return Result(false, "Expected enum " + refName + " but got " + value.toString());
                }
                return Result(true);
            }

            return validateEntry(value, refDefinition, types);
        }

        JsonHandler::BasicType expectedBasicType = JsonHandler::enumNameToValue<JsonHandler::BasicType>(expectedTypeName);
        QVariant::Type expectedVariantType = JsonHandler::basicTypeToVariantType(expectedBasicType);

        // Verify basic compatiblity
        if (expectedBasicType != JsonHandler::Variant && !value.canConvert(expectedVariantType)) {
            return Result(false, "Invalid value. Expected: " + definition.toString() + ", Got: " + value.toString());
        }

        // Any string converts fine to Uuid, but the resulting uuid might be null
        if (expectedBasicType == JsonHandler::Uuid && value.toUuid().isNull()) {
            return Result(false, "Invalid Uuid: " + value.toString());
        }
        // Make sure ints are valid
        if (expectedBasicType == JsonHandler::Int) {
            bool ok;
            value.toLongLong(&ok);
            if (!ok) {
                return Result(false, "Invalid Int: " + value.toString());
            }
        }
        // UInts
        if (expectedBasicType == JsonHandler::Uint) {
            bool ok;
            value.toULongLong(&ok);
            if (!ok) {
                return Result(false, "Invalid UInt: " + value.toString());
            }
        }
        // Double
        if (expectedBasicType == JsonHandler::Double) {
            bool ok;
            value.toDouble(&ok);
            if (!ok) {
                return Result(false, "Invalid Double: " + value.toString());
            }
        }
        // Color
        if (expectedBasicType == JsonHandler::Color) {
            QColor color = value.value<QColor>();
            if (!color.isValid()) {
                return Result(false, "Invalid Color: " + value.toString());
            }
        }
        // Time
        if (expectedBasicType == JsonHandler::Time) {
            bool ok;
            QDateTime time = QDateTime::fromTime_t(value.toUInt(&ok));
            if (!ok || !time.isValid()) {
                return Result(false, "Invalid Time: " + value.toString());
            }
        }


        return Result(true);
    }

    if (definition.type() == QVariant::Map) {
        if (value.type() != QVariant::Map) {
            return Result(false, "Invalud value. Expected a map bug received: " + value.toString());
        }
        return validateMap(value.toMap(), definition.toMap(), types);
    }

    if (definition.type() == QVariant::List) {
        QVariantList list = definition.toList();
        QVariant entryDefinition = list.first();
        if (value.type() != QVariant::List && value.type() != QVariant::StringList) {
            return Result(false, "Expected list of " + entryDefinition.toString() + " but got value of type " + value.typeName() + "\n" + QJsonDocument::fromVariant(value).toJson());
        }
        foreach (const QVariant &entry, value.toList()) {
            Result result = validateEntry(entry, entryDefinition, types);
            if (!result.success()) {
                return result;
            }
        }
        return Result(true);
    }
    Q_ASSERT_X(false, "JsonValildator", "Incomplete validation. Unexpected type in template");
    return Result(false);
}

}
