/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Michael Zanetti <michael.zanetti@nymea.io>          *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "scriptaction.h"

#include "devices/devicemanager.h"
#include "types/action.h"

#include <QQmlEngine>
#include <qqml.h>

#include "loggingcategories.h"

namespace nymeaserver {

ScriptAction::ScriptAction(QObject *parent) : QObject(parent)
{

}

void ScriptAction::classBegin()
{
    m_deviceManager = reinterpret_cast<DeviceManager*>(qmlEngine(this)->property("deviceManager").toULongLong());
}

void ScriptAction::componentComplete()
{

}

QString ScriptAction::deviceId() const
{
    return m_deviceId;
}

void ScriptAction::setDeviceId(const QString &deviceId)
{
    if (m_deviceId != deviceId) {
        m_deviceId = deviceId;
        emit deviceIdChanged();
    }
}

QString ScriptAction::actionTypeId() const
{
    return m_actionTypeId;
}

void ScriptAction::setActionTypeId(const QString &actionTypeId)
{
    if (m_actionTypeId != actionTypeId) {
        m_actionTypeId = actionTypeId;
        emit actionTypeIdChanged();
    }
}

QString ScriptAction::actionName() const
{
    return m_actionName;
}

void ScriptAction::setActionName(const QString &actionName)
{
    if (m_actionName != actionName) {
        m_actionName = actionName;
        emit actionNameChanged();
    }
}

void ScriptAction::execute(const QVariantMap &params)
{
    Device *device = m_deviceManager->configuredDevices().findById(DeviceId(m_deviceId));
    if (!device) {
        qCWarning(dcScriptEngine) << "No device with id" << m_deviceId;
        return;
    }
    ActionType actionType;
    if (!ActionTypeId(m_actionTypeId).isNull()) {
        actionType = device->deviceClass().actionTypes().findById(ActionTypeId(m_actionTypeId));
    } else {
        actionType = device->deviceClass().actionTypes().findByName(m_actionName);
    }
    if (actionType.id().isNull()) {
        qCWarning(dcScriptEngine()) << "Either a valid actionTypeId or actionName is required";
        return;
    }
    Action action;
    action.setActionTypeId(actionType.id());
    action.setDeviceId(DeviceId(m_deviceId));
    ParamList paramList;
    foreach (const QString &paramNameOrId, params.keys()) {
        ParamType paramType;
        if (!ParamTypeId(paramNameOrId).isNull()) {
            paramType = actionType.paramTypes().findById(ParamTypeId(paramNameOrId));
        } else {
            paramType = actionType.paramTypes().findByName(paramNameOrId);
        }
        if (paramType.id().isNull()) {
            qCWarning(dcScriptEngine()) << "Invalid param id or name";
            continue;
        }
        paramList << Param(paramType.id(), params.value(paramNameOrId));
    }
    action.setParams(paramList);
    m_deviceManager->executeAction(action);
}

}
