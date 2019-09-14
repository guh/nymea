/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

/*!
    \class nymeaserver::DevicesResource
    \brief This subclass of \l{RestResource} processes the REST requests for the \tt Devices namespace.

    \ingroup json
    \inmodule core

    This \l{RestResource} will be created in the \l{RestServer} and used to handle REST requests
    for the \tt {Devices} namespace of the API.

    \code
        http://localhost:3333/api/v1/devices
    \endcode

    \sa Device, RestResource, RestServer
*/


#include "devicesresource.h"
#include "servers/httpreply.h"
#include "servers/httprequest.h"
#include "jsonrpc/jsontypes.h"
#include "nymeacore.h"

#include <QJsonDocument>

namespace nymeaserver {

/*! Constructs a \l DevicesResource with the given \a parent. */
DevicesResource::DevicesResource(QObject *parent) :
    RestResource(parent)
{
    connect(NymeaCore::instance(), &NymeaCore::actionExecuted, this, &DevicesResource::actionExecuted);
    connect(NymeaCore::instance(), &NymeaCore::deviceSetupFinished, this, &DevicesResource::deviceSetupFinished);
    connect(NymeaCore::instance(), &NymeaCore::deviceReconfigurationFinished, this, &DevicesResource::deviceReconfigurationFinished);
    connect(NymeaCore::instance(), &NymeaCore::pairingFinished, this, &DevicesResource::pairingFinished);
}

/*! Returns the name of the \l{RestResource}. In this case \b devices.

    \sa RestResource::name()
*/
QString DevicesResource::name() const
{
    return "devices";
}

/*! This method will be used to process the given \a request and the given \a urlTokens. The request
    has to be in this namespace. Returns the resulting \l HttpReply.

    \sa HttpRequest, HttpReply, RestResource::proccessRequest()
*/
HttpReply *DevicesResource::proccessRequest(const HttpRequest &request, const QStringList &urlTokens)
{
    m_device = nullptr;

    // get the main resource
    if (urlTokens.count() >= 4 && urlTokens.at(3) != "pair" && urlTokens.at(3) != "confirmpairing") {
        DeviceId deviceId = DeviceId(urlTokens.at(3));
        if (deviceId.isNull()) {
            qCWarning(dcRest) << "Could not parse DeviceId:" << urlTokens.at(3);
            return createDeviceErrorReply(HttpReply::BadRequest, Device::DeviceErrorDeviceNotFound);
        }
        m_device = NymeaCore::instance()->deviceManager()->findConfiguredDevice(deviceId);
        if (!m_device) {
            qCWarning(dcRest) << "Could find any device with DeviceId:" << urlTokens.at(3);
            return createDeviceErrorReply(HttpReply::NotFound, Device::DeviceErrorDeviceNotFound);
        }
    }

    // check method
    HttpReply *reply;
    switch (request.method()) {
    case HttpRequest::Get:
        reply = proccessGetRequest(request, urlTokens);
        break;
    case HttpRequest::Post:
        reply = proccessPostRequest(request, urlTokens);
        break;
    case HttpRequest::Put:
        reply = proccessPutRequest(request, urlTokens);
        break;
    case HttpRequest::Delete:
        reply = proccessDeleteRequest(request, urlTokens);
        break;
    case HttpRequest::Options:
        reply = proccessOptionsRequest(request, urlTokens);
        break;
    default:
        reply = createErrorReply(HttpReply::BadRequest);
        break;
    }
    return reply;
}

HttpReply *DevicesResource::proccessGetRequest(const HttpRequest &request, const QStringList &urlTokens)
{
    Q_UNUSED(request)

    // GET /api/v1/devices
    if (urlTokens.count() == 3)
        return getConfiguredDevices();

    // GET /api/v1/devices/{deviceId}
    if (urlTokens.count() == 4)
        return getConfiguredDevice(m_device);

    // GET /api/v1/devices/{deviceId}/states
    if (urlTokens.count() == 5 && urlTokens.at(4) == "states")
        return getDeviceStateValues(m_device);

    // /api/v1/devices/{deviceId}/states/{stateTypeId}
    if (urlTokens.count() >= 6 && urlTokens.at(4) == "states") {
        StateTypeId stateTypeId = StateTypeId(urlTokens.at(5));
        if (stateTypeId.isNull()) {
            qCWarning(dcRest) << "Could not parse StateTypeId:" << urlTokens.at(5);
            return createDeviceErrorReply(HttpReply::BadRequest, Device::DeviceErrorStateTypeNotFound);
        }

        if (!m_device->hasState(stateTypeId)){
            qCWarning(dcRest) << "This device has no StateTypeId:" << urlTokens.at(5);
             return createDeviceErrorReply(HttpReply::NotFound, Device::DeviceErrorStateTypeNotFound);
        }
        return getDeviceStateValue(m_device, stateTypeId);
    }
    return createErrorReply(HttpReply::NotImplemented);
}

HttpReply *DevicesResource::proccessDeleteRequest(const HttpRequest &request, const QStringList &urlTokens)
{
    Q_UNUSED(request)

    // DELETE /api/v1/devices
    if (urlTokens.count() == 3)
        return createErrorReply(HttpReply::BadRequest);

    // DELETE /api/v1/devices/{deviceId}
    if (urlTokens.count() == 4) {
        QVariantMap params;
        if (request.urlQuery().hasQueryItem("params")) {
            QString paramMapString = request.urlQuery().queryItemValue("params");

            QPair<bool, QVariant> verification = verifyPayload(paramMapString.toUtf8());
            if (!verification.first)
                return createErrorReply(HttpReply::BadRequest);

            params = verification.second.toMap();
        }
        return removeDevice(m_device, params);
    }

    return createErrorReply(HttpReply::NotImplemented);
}

HttpReply *DevicesResource::proccessPutRequest(const HttpRequest &request, const QStringList &urlTokens)
{
    Q_UNUSED(request)
    // PUT /api/v1/devices
    if (urlTokens.count() == 3)
        return createErrorReply(HttpReply::BadRequest);

    // PUT /api/v1/devices/{deviceId}
    if (urlTokens.count() == 4)
        return reconfigureDevice(m_device, request.payload());

    return createErrorReply(HttpReply::NotImplemented);
}

HttpReply *DevicesResource::proccessPostRequest(const HttpRequest &request, const QStringList &urlTokens)
{
    // POST /api/v1/devices
    if (urlTokens.count() == 3)
        return addConfiguredDevice(request.payload());

    // POST /api/v1/devices/pair
    if (urlTokens.count() == 4 && urlTokens.at(3) == "pair")
        return pairDevice(request.payload());

    // POST /api/v1/devices/confirmpairing
    if (urlTokens.count() == 4 && urlTokens.at(3) == "confirmpairing")
        return confirmPairDevice(request.payload());

    // POST /api/v1/devices/{deviceId}
    if (urlTokens.count() == 4)
        return editDevice(request.payload());

    // POST /api/v1/devices/{deviceId}/execute/{actionTypeId}
    if (urlTokens.count() >= 6 && urlTokens.at(4) == "execute") {
        ActionTypeId actionTypeId = ActionTypeId(urlTokens.at(5));
        if (actionTypeId.isNull()) {
            qCWarning(dcRest) << "Could not parse ActionTypeId:" << urlTokens.at(5);
            return createDeviceErrorReply(HttpReply::BadRequest, Device::DeviceErrorActionTypeNotFound);
        }
        bool found = false;
        DeviceClass deviceClass = NymeaCore::instance()->deviceManager()->findDeviceClass(m_device->deviceClassId());
        foreach (const ActionType actionType, deviceClass.actionTypes()) {
            if (actionType.id() == actionTypeId) {
                found = true;
                break;
            }
        }
        if (!found) {
            qCWarning(dcRest) << "Could not find ActionTypeId:" << actionTypeId.toString();
            return createDeviceErrorReply(HttpReply::NotFound, Device::DeviceErrorActionTypeNotFound);
        }

        return executeAction(m_device, actionTypeId, request.payload());
    }

    return createErrorReply(HttpReply::NotImplemented);
}

HttpReply *DevicesResource::proccessOptionsRequest(const HttpRequest &request, const QStringList &urlTokens)
{
    Q_UNUSED(request)
    Q_UNUSED(urlTokens)
    return RestResource::createCorsSuccessReply();
}

HttpReply *DevicesResource::getConfiguredDevices() const
{
    qCDebug(dcRest) << "Get all configured devices";
    HttpReply *reply = createSuccessReply();
    reply->setHeader(HttpReply::ContentTypeHeader, "application/json; charset=\"utf-8\";");
    reply->setPayload(QJsonDocument::fromVariant(JsonTypes::packConfiguredDevices()).toJson());
    return reply;
}

HttpReply *DevicesResource::getConfiguredDevice(Device *device) const
{
    qCDebug(dcRest) << "Get configured device with id:" << device->id().toString();
    HttpReply *reply = createSuccessReply();
    reply->setHeader(HttpReply::ContentTypeHeader, "application/json; charset=\"utf-8\";");
    reply->setPayload(QJsonDocument::fromVariant(JsonTypes::packDevice(device)).toJson());
    return reply;
}

HttpReply *DevicesResource::getDeviceStateValues(Device *device) const
{
    qCDebug(dcRest) << "Get states of device with id:" << device->id().toString();
    HttpReply *reply = createSuccessReply();
    reply->setHeader(HttpReply::ContentTypeHeader, "application/json; charset=\"utf-8\";");
    reply->setPayload(QJsonDocument::fromVariant(JsonTypes::packDeviceStates(device)).toJson());
    return reply;
}

HttpReply *DevicesResource::getDeviceStateValue(Device *device, const StateTypeId &stateTypeId) const
{
    qCDebug(dcRest) << "Get device state value of state with id:" << stateTypeId.toString();
    HttpReply *reply = createSuccessReply();
    QVariantMap stateValueMap;
    stateValueMap.insert("value", device->state(stateTypeId).value());
    reply->setHeader(HttpReply::ContentTypeHeader, "application/json; charset=\"utf-8\";");
    reply->setPayload(QJsonDocument::fromVariant(stateValueMap).toJson());
    return reply;
}

HttpReply *DevicesResource::removeDevice(Device *device, const QVariantMap &params) const
{
    qCDebug(dcRest) << "Remove device with id:" << device->id().toString();
    qCDebug(dcRest) << QJsonDocument::fromVariant(params).toJson();

    // global removePolicy has priority
    if (params.contains("removePolicy")) {
        RuleEngine::RemovePolicy removePolicy = params.value("removePolicy").toString() == "RemovePolicyCascade" ? RuleEngine::RemovePolicyCascade : RuleEngine::RemovePolicyUpdate;
        Device::DeviceError result = NymeaCore::instance()->removeConfiguredDevice(device->id(), removePolicy);
        return createDeviceErrorReply(HttpReply::Ok, result);
    }

    QHash<RuleId, RuleEngine::RemovePolicy> removePolicyList;
    foreach (const QVariant &variant, params.value("removePolicyList").toList()) {
        RuleId ruleId = RuleId(variant.toMap().value("ruleId").toString());
        RuleEngine::RemovePolicy policy = variant.toMap().value("policy").toString() == "RemovePolicyCascade" ? RuleEngine::RemovePolicyCascade : RuleEngine::RemovePolicyUpdate;
        removePolicyList.insert(ruleId, policy);
    }

    QPair<Device::DeviceError, QList<RuleId> > status = NymeaCore::instance()->removeConfiguredDevice(device->id(), removePolicyList);

    // if there are offending rules
    if (!status.second.isEmpty()) {
        QVariantList ruleIdList;
        QVariantMap returns;
        returns.insert("error", JsonTypes::deviceErrorToString(status.first));

        foreach (const RuleId &ruleId, status.second) {
            ruleIdList.append(ruleId.toString());
        }
        returns.insert("ruleIds", ruleIdList);

        HttpReply *reply = createErrorReply(HttpReply::BadRequest);
        reply->setHeader(HttpReply::ContentTypeHeader, "application/json; charset=\"utf-8\";");
        reply->setPayload(QJsonDocument::fromVariant(returns).toJson());
        return reply;
    }

    if (status.first == Device::DeviceErrorNoError)
        return createDeviceErrorReply(HttpReply::Ok, status.first);

    return createDeviceErrorReply(HttpReply::BadRequest, status.first);
}

HttpReply *DevicesResource::executeAction(Device *device, const ActionTypeId &actionTypeId, const QByteArray &payload) const
{
    QPair<bool, QVariant> verification = RestResource::verifyPayload(payload);
    if (!verification.first)
        return createErrorReply(HttpReply::BadRequest);

    QVariantMap message = verification.second.toMap();

    if (!message.contains("params"))
        return createErrorReply(HttpReply::BadRequest);

    ParamList actionParams = JsonTypes::unpackParams(message.value("params").toList());

    qCDebug(dcRest) << "Execute action with" << actionParams;

    Action action(actionTypeId, device->id());
    action.setParams(actionParams);

    Device::DeviceError status = NymeaCore::instance()->executeAction(action);
    if (status == Device::DeviceErrorAsync) {
        HttpReply *reply = createAsyncReply();
        m_asyncActionExecutions.insert(action.id(), reply);
        return reply;
    }

    if (status != Device::DeviceErrorNoError)
        return createDeviceErrorReply(HttpReply::InternalServerError, status);

    return createDeviceErrorReply(HttpReply::Ok, status);
}

HttpReply *DevicesResource::addConfiguredDevice(const QByteArray &payload) const
{
    QPair<bool, QVariant> verification = RestResource::verifyPayload(payload);
    if (!verification.first)
        return createErrorReply(HttpReply::BadRequest);

    QVariantMap params = verification.second.toMap();

    DeviceClassId deviceClassId(params.value("deviceClassId").toString());
    if (deviceClassId.isNull())
        return createDeviceErrorReply(HttpReply::BadRequest, Device::DeviceErrorDeviceClassNotFound);

    QString deviceName = params.value("name").toString();
    DeviceId newDeviceId = DeviceId::createDeviceId();
    ParamList deviceParams = JsonTypes::unpackParams(params.value("deviceParams").toList());
    DeviceDescriptorId deviceDescriptorId(params.value("deviceDescriptorId").toString());

    DeviceSetupInfo info;
    if (deviceDescriptorId.isNull()) {
        qCDebug(dcRest) << "Adding device" << deviceName << "with" << deviceParams;
        info = NymeaCore::instance()->deviceManager()->addConfiguredDevice(deviceClassId, deviceName, deviceParams, newDeviceId);
    } else {
        qCDebug(dcRest) << "Adding discovered device" << deviceName << "with DeviceDescriptorId" << deviceDescriptorId.toString();
        info = NymeaCore::instance()->deviceManager()->addConfiguredDevice(deviceClassId, deviceName, deviceDescriptorId, deviceParams, newDeviceId);
    }
    if (info.status() == Device::DeviceErrorAsync) {
        HttpReply *reply = createAsyncReply();
        qCDebug(dcRest) << "Device setup async reply";
        m_asyncDeviceAdditions.insert(newDeviceId, reply);
        return reply;
    }

    if (info.status() != Device::DeviceErrorNoError)
        return createDeviceErrorReply(HttpReply::InternalServerError, info.status());

    QVariant result = JsonTypes::packDevice(NymeaCore::instance()->deviceManager()->findConfiguredDevice(newDeviceId));
    HttpReply *reply = createSuccessReply();
    reply->setHeader(HttpReply::ContentTypeHeader, "application/json; charset=\"utf-8\";");
    reply->setPayload(QJsonDocument::fromVariant(result).toJson());
    return reply;
}

HttpReply *DevicesResource::editDevice(const QByteArray &payload) const
{
    QPair<bool, QVariant> verification = RestResource::verifyPayload(payload);
    if (!verification.first)
        return createErrorReply(HttpReply::BadRequest);

    QVariantMap params = verification.second.toMap();
    QString name = params.value("name").toString();
    Device::DeviceError status = NymeaCore::instance()->deviceManager()->editDevice(m_device->id(), name);

    if (status != Device::DeviceErrorNoError)
        return createDeviceErrorReply(HttpReply::BadRequest, status);

    return createDeviceErrorReply(HttpReply::Ok, status);
}

HttpReply *DevicesResource::pairDevice(const QByteArray &payload) const
{
    QPair<bool, QVariant> verification = RestResource::verifyPayload(payload);
    if (!verification.first)
        return createErrorReply(HttpReply::BadRequest);

    QVariantMap params = verification.second.toMap();

    DeviceClassId deviceClassId(params.value("deviceClassId").toString());
    DeviceClass deviceClass = NymeaCore::instance()->deviceManager()->findDeviceClass(deviceClassId);

    if (deviceClassId.isNull()) {
        qCWarning(dcRest) << "Could not find deviceClassId" << params.value("deviceClassId").toString();
        return createDeviceErrorReply(HttpReply::BadRequest, Device::DeviceErrorDeviceClassNotFound);
    }

    QString deviceName = params.value("name").toString();

    qCDebug(dcRest) << "Pair device" << deviceName << "with deviceClassId" << deviceClassId.toString();

    DevicePairingInfo pairingInfo;
    if (params.contains("deviceDescriptorId")) {
        DeviceDescriptorId deviceDescriptorId(params.value("deviceDescriptorId").toString());
        pairingInfo = NymeaCore::instance()->deviceManager()->pairDevice(deviceClassId, deviceName, deviceDescriptorId);
    } else {
        ParamList deviceParams = JsonTypes::unpackParams(params.value("deviceParams").toList());
        pairingInfo = NymeaCore::instance()->deviceManager()->pairDevice(deviceClassId, deviceName, deviceParams);
    }

    if (pairingInfo.status() != Device::DeviceErrorNoError)
        return createDeviceErrorReply(HttpReply::BadRequest, pairingInfo.status());

    QVariantMap returns;
    returns.insert("displayMessage", NymeaCore::instance()->deviceManager()->translate(deviceClass.pluginId(), deviceClass.pairingInfo(), NymeaCore::instance()->configuration()->locale()));
    returns.insert("pairingTransactionId", pairingInfo.transactionId().toString());
    returns.insert("setupMethod", JsonTypes::setupMethod().at(deviceClass.setupMethod()));
    HttpReply *reply = createSuccessReply();
    reply->setHeader(HttpReply::ContentTypeHeader, "application/json; charset=\"utf-8\";");
    reply->setPayload(QJsonDocument::fromVariant(returns).toJson());
    return reply;
}

HttpReply *DevicesResource::confirmPairDevice(const QByteArray &payload) const
{
    QPair<bool, QVariant> verification = RestResource::verifyPayload(payload);
    if (!verification.first)
        return createErrorReply(HttpReply::BadRequest);

    QVariantMap params = verification.second.toMap();

    PairingTransactionId pairingTransactionId = PairingTransactionId(params.value("pairingTransactionId").toString());
    QString secret = params.value("secret").toString();
    DevicePairingInfo info = NymeaCore::instance()->deviceManager()->confirmPairing(pairingTransactionId, secret);

    if (info.status() == Device::DeviceErrorAsync) {
        HttpReply *reply = createAsyncReply();
        qCDebug(dcRest) << "Confirm pairing async reply";
        m_asyncPairingRequests.insert(pairingTransactionId, reply);
        return reply;
    }

    if (info.status() != Device::DeviceErrorNoError)
        return createDeviceErrorReply(HttpReply::InternalServerError, info.status());

    return createDeviceErrorReply(HttpReply::Ok, Device::DeviceErrorNoError);
}

HttpReply *DevicesResource::reconfigureDevice(Device *device, const QByteArray &payload) const
{
    qCDebug(dcRest) << "Reconfigure device" << device->id();
    QPair<bool, QVariant> verification = RestResource::verifyPayload(payload);
    if (!verification.first)
        return createErrorReply(HttpReply::BadRequest);

    QVariantMap params = verification.second.toMap();
    ParamList deviceParams = JsonTypes::unpackParams(params.value("deviceParams").toList());

    DeviceSetupInfo info;
    DeviceDescriptorId deviceDescriptorId(params.value("deviceDescriptorId").toString());
    if (deviceDescriptorId.isNull()) {
        qCDebug(dcRest) << "Reconfigure device with params:" << deviceParams;
        info = NymeaCore::instance()->deviceManager()->reconfigureDevice(device->id(), deviceParams);
    } else {
        qCDebug(dcRest) << "Reconfigure device using the new discovered device with descriptorId:" << deviceDescriptorId.toString();
        info = NymeaCore::instance()->deviceManager()->reconfigureDevice(device->id(), deviceDescriptorId);
    }

    if (info.status() == Device::DeviceErrorAsync) {
        HttpReply *reply = createAsyncReply();
        qCDebug(dcRest) << "Device reconfiguration async reply";
        m_asyncReconfigureDevice.insert(device, reply);
        return reply;
    }

    if (info.status() != Device::DeviceErrorNoError)
        return createDeviceErrorReply(HttpReply::InternalServerError, info.status());

    return createDeviceErrorReply(HttpReply::Ok, Device::DeviceErrorNoError);
}

void DevicesResource::actionExecuted(const ActionId &actionId, Device::DeviceError status)
{
    if (!m_asyncActionExecutions.contains(actionId))
        return; // Not the action we are waiting for.

    QVariantMap response;
    response.insert("error", JsonTypes::deviceErrorToString(status));

    if (m_asyncActionExecutions.value(actionId).isNull()) {
        qCWarning(dcRest) << "Async reply for execute action does not exist any more (timeout).";
        return;
    }

    HttpReply *reply = m_asyncActionExecutions.take(actionId);
    reply->setHeader(HttpReply::ContentTypeHeader, "application/json; charset=\"utf-8\";");
    if (status == Device::DeviceErrorNoError) {
        qCDebug(dcRest) << "Action execution finished successfully";
        reply->setHttpStatusCode(HttpReply::Ok);
        reply->setPayload(QJsonDocument::fromVariant(response).toJson());
    } else {
        qCDebug(dcRest) << "Action execution finished with error" << status;
        QVariantMap response;
        response.insert("error", JsonTypes::deviceErrorToString(status));
        reply->setHttpStatusCode(HttpReply::InternalServerError);
        reply->setPayload(QJsonDocument::fromVariant(response).toJson());
    }

    reply->finished();
}

void DevicesResource::deviceSetupFinished(const DeviceSetupInfo &info)
{
    if (!m_asyncDeviceAdditions.contains(info.deviceId()))
        return; // Not the device we are waiting for.

    QVariantMap response;
    response.insert("error", JsonTypes::deviceErrorToString(info.status()));

    if (m_asyncDeviceAdditions.value(info.deviceId()).isNull()) {
        qCWarning(dcRest) << "Async reply for device setup does not exist any more (timeout).";
        return;
    }

    HttpReply *reply = m_asyncDeviceAdditions.take(info.deviceId());
    reply->setHeader(HttpReply::ContentTypeHeader, "application/json; charset=\"utf-8\";");
    if (info.status() == Device::DeviceErrorNoError) {
        qCDebug(dcRest) << "Device setup finished successfully";
        reply->setHttpStatusCode(HttpReply::Ok);
        reply->setPayload(QJsonDocument::fromVariant(response).toJson());
    } else {
        qCDebug(dcRest) << "Device setup finished with error" << info.status();
        reply->setHttpStatusCode(HttpReply::InternalServerError);
        reply->setPayload(QJsonDocument::fromVariant(response).toJson());
    }

    Device *device = NymeaCore::instance()->deviceManager()->findConfiguredDevice(info.deviceId());
    QVariant result = JsonTypes::packDevice(device);
    reply->setHeader(HttpReply::ContentTypeHeader, "application/json; charset=\"utf-8\";");
    reply->setPayload(QJsonDocument::fromVariant(result).toJson());
    reply->finished();
}

void DevicesResource::deviceReconfigurationFinished(Device *device, Device::DeviceError status)
{
    if (!m_asyncReconfigureDevice.contains(device))
        return; // Not the device we are waiting for.

    QVariantMap response;
    response.insert("error", JsonTypes::deviceErrorToString(status));

    if (m_asyncReconfigureDevice.value(device).isNull()) {
        qCWarning(dcRest) << "Async reply for device reconfiguration does not exist any more (timeout).";
        return;
    }

    HttpReply *reply = m_asyncReconfigureDevice.take(device);
    reply->setHeader(HttpReply::ContentTypeHeader, "application/json; charset=\"utf-8\";");
    if (status == Device::DeviceErrorNoError) {
        qCDebug(dcRest) << "Device reconfiguration finished successfully";
        reply->setHttpStatusCode(HttpReply::Ok);
        reply->setPayload(QJsonDocument::fromVariant(response).toJson());
    } else {
        qCDebug(dcRest) << "Device reconfiguration finished with error" << status;
        reply->setHttpStatusCode(HttpReply::InternalServerError);
        reply->setPayload(QJsonDocument::fromVariant(response).toJson());
    }

    reply->finished();
}

void DevicesResource::pairingFinished(const DevicePairingInfo &devicePairingInfo)
{
    if (!m_asyncPairingRequests.contains(devicePairingInfo.transactionId()))
        return; // Not the device pairing we are waiting for.

    QVariantMap response;
    response.insert("error", JsonTypes::deviceErrorToString(devicePairingInfo.status()));

    if (m_asyncPairingRequests.value(devicePairingInfo.transactionId()).isNull()) {
        qCWarning(dcRest) << "Async reply for device pairing does not exist any more.";
        return;
    }

    HttpReply *reply = m_asyncPairingRequests.take(devicePairingInfo.transactionId());
    if (devicePairingInfo.status() != Device::DeviceErrorNoError) {
        qCDebug(dcRest) << "Pairing device finished with error.";
        reply->setHeader(HttpReply::ContentTypeHeader, "application/json; charset=\"utf-8\";");
        reply->setHttpStatusCode(HttpReply::InternalServerError);
        reply->setPayload(QJsonDocument::fromVariant(response).toJson());
        reply->finished();
        return;
    }

    qCDebug(dcRest) << "Pairing device finished successfully";

    // Add device to async device addtions
    m_asyncDeviceAdditions.insert(devicePairingInfo.deviceId(), reply);
}

}
