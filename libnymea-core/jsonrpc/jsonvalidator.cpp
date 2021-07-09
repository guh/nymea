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

#include "jsonvalidator.h"
#include "jsonrpc/jsonhandler.h"

#include "loggingcategories.h"

#include <QJsonDocument>
#include <QColor>
#include <QDateTime>

namespace nymeaserver {

bool JsonValidator::checkRefs(const QVariantMap &map, const QVariantMap &api)
{
    QVariantMap enums = api.value("enums").toMap();
    QVariantMap flags = api.value("flags").toMap();
    QVariantMap types = api.value("types").toMap();
    foreach (const QString &key, map.keys()) {
        if (map.value(key).toString().startsWith("$ref:")) {
            QString refName = map.value(key).toString().remove("$ref:");
            if (!enums.contains(refName) && !flags.contains(refName) && !types.contains(refName)) {
                qCWarning(dcJsonRpc()) << "Invalid reference to" << refName;
                return false;
            }
        }
        if (map.value(key).type() == QVariant::Map) {
            bool ret = checkRefs(map.value(key).toMap(), api);
            if (!ret) {
                return false;
            }
        }
        if (map.value(key).type() == QVariant::List) {
            foreach (const QVariant &entry, map.value(key).toList()) {
                if (entry.toString().startsWith("$ref:")) {
                    QString refName = entry.toString().remove("$ref:");
                    if (!enums.contains(refName) && !flags.contains(refName) && !types.contains(refName)) {
                        qCWarning(dcJsonRpc()) << "Invalid reference to" << refName;
                        return false;
                    }
                }
                if (entry.type() == QVariant::Map) {
                    bool ret = checkRefs(map.value(key).toMap(), api);
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
    m_result = validateMap(params, paramDefinition, api, QIODevice::WriteOnly);
    m_result.setWhere(method + ", param " + m_result.where());
    return m_result;
}

JsonValidator::Result JsonValidator::validateReturns(const QVariantMap &returns, const QString &method, const QVariantMap &api)
{
    QVariantMap returnsDefinition = api.value("methods").toMap().value(method).toMap().value("returns").toMap();
    m_result = validateMap(returns, returnsDefinition, api, QIODevice::ReadOnly);
    m_result.setWhere(method + ", returns " + m_result.where());
    return m_result;
}

JsonValidator::Result JsonValidator::validateNotificationParams(const QVariantMap &params, const QString &notification, const QVariantMap &api)
{
    QVariantMap paramDefinition = api.value("notifications").toMap().value(notification).toMap().value("params").toMap();
    m_result = validateMap(params, paramDefinition, api, QIODevice::ReadOnly);
    m_result.setWhere(notification + ", param " + m_result.where());
    return m_result;
}

JsonValidator::Result JsonValidator::result() const
{
    return m_result;
}

JsonValidator::Result JsonValidator::validateMap(const QVariantMap &map, const QVariantMap &definition, const QVariantMap &api, QIODevice::OpenMode openMode)
{
    // Make sure all required values are available
    foreach (const QString &key, definition.keys()) {
        QRegExp isOptional = QRegExp("^([a-z]:)*o:.*");
        if (isOptional.exactMatch(key)) {
            continue;
        }
        QRegExp isReadOnly = QRegExp("^([a-z]:)*r:.*");
        if (isReadOnly.exactMatch(key) && openMode.testFlag(QIODevice::WriteOnly)) {
            continue;
        }
        QString trimmedKey = key;
        trimmedKey.remove(QRegExp("^(o:|r:|d:)*"));
        if (!map.contains(trimmedKey)) {
            return Result(false, "Missing required key: " + key, key);
        }
    }

    // Make sure given values are valid
    foreach (const QString &key, map.keys()) {
        // Is the key allowed in here?
        QVariant expectedValue = definition.value(key);
        foreach (const QString &definitionKey, definition.keys()) {
            QRegExp regExp = QRegExp("(o:|r:|d:)*" + key);
            if (regExp.exactMatch(definitionKey)) {
                expectedValue = definition.value(definitionKey);
            }
        }
        if (!expectedValue.isValid()) {
            expectedValue = definition.value("o:" + key);
        }
        if (!expectedValue.isValid()) {
            expectedValue = definition.value("d:" + key);
        }
        if (!expectedValue.isValid()) {
            return Result(false, "Invalid key: " + key);
        }

        // Validate content
        QVariant value = map.value(key);

        Result result = validateEntry(value, expectedValue, api, openMode);
        if (!result.success()) {
            result.setWhere(key + '.' + result.where());
            result.setErrorString(result.errorString());
            return result;
        }

   }

    return Result(true);
}

JsonValidator::Result JsonValidator::validateEntry(const QVariant &value, const QVariant &definition, const QVariantMap &api, QIODevice::OpenMode openMode)
{
    if (definition.type() == QVariant::String) {
        QString expectedTypeName = definition.toString();

        if (expectedTypeName.startsWith("$ref:")) {
            QString refName = expectedTypeName;
            refName.remove("$ref:");

            // Refs might be enums
            QVariantMap enums = api.value("enums").toMap();
            if (enums.contains(refName)) {
                QVariant refDefinition = enums.value(refName);

                QVariantList enumList = refDefinition.toList();
                if (!enumList.contains(value.toString())) {
                    return Result(false, "Expected enum value for" + refName + " but got " + value.toString());
                }
                return Result(true);
            }
            // Or flags
            QVariantMap flags = api.value("flags").toMap();
            if (flags.contains(refName)) {
                QVariant refDefinition = flags.value(refName);
                if (value.type() != QVariant::List && value.type() != QVariant::StringList) {
                    return Result(false, "Expected flags " + refName + " but got " + value.toString());
                }
                QString flagEnum = refDefinition.toList().first().toString();
                foreach (const QVariant &flagsEntry, value.toList()) {
                    Result result = validateEntry(flagsEntry, flagEnum, api, openMode);
                    if (!result.success()) {
                        return result;
                    }
                }
                return Result(true);
            }

            QVariantMap types = api.value("types").toMap();
            QVariant refDefinition = types.value(refName);
            return validateEntry(value, refDefinition, api, openMode);
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
            QTime time = QTime::fromString(value.toString(), "hh:mm");
            if (!time.isValid()) {
                return Result(false, "Invalid Time: " + value.toString());
            }
        }


        return Result(true);
    }

    if (definition.type() == QVariant::Map) {
        if (value.type() != QVariant::Map) {
            return Result(false, "Invalid value. Expected a map bug received: " + value.toString());
        }
        return validateMap(value.toMap(), definition.toMap(), api, openMode);
    }

    if (definition.type() == QVariant::List) {
        QVariantList list = definition.toList();
        QVariant entryDefinition = list.first();
        if (value.type() != QVariant::List && value.type() != QVariant::StringList) {
            return Result(false, "Expected list of " + entryDefinition.toString() + " but got value of type " + value.typeName() + "\n" + QJsonDocument::fromVariant(value).toJson());
        }
        foreach (const QVariant &entry, value.toList()) {
            Result result = validateEntry(entry, entryDefinition, api, openMode);
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
