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

#ifndef DEVICE_H
#define DEVICE_H

#include "typeutils.h"
#include "libnymea.h"

#include "types/thingclass.h"
#include "types/state.h"
#include "types/param.h"
#include "types/event.h"
#include "types/browseritem.h"

#include <QObject>
#include <QUuid>
#include <QVariant>

class IntegrationPlugin;
class StateValueFilter;

class LIBNYMEA_EXPORT Thing: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUuid id READ id)
    Q_PROPERTY(QUuid thingClassId READ thingClassId)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged USER true)
    Q_PROPERTY(ParamList params READ params)
    Q_PROPERTY(ParamList settings READ settings WRITE setSettings USER true)
    Q_PROPERTY(States states READ states)
    Q_PROPERTY(bool setupComplete READ setupComplete NOTIFY setupStatusChanged REVISION 1)
    Q_PROPERTY(ThingSetupStatus setupStatus READ setupStatus NOTIFY setupStatusChanged)
    Q_PROPERTY(QString setupDisplayMessage READ setupDisplayMessage NOTIFY setupStatusChanged USER true)
    Q_PROPERTY(ThingError setupError READ setupError NOTIFY setupStatusChanged)
    Q_PROPERTY(QUuid parentId READ parentId USER true)
    Q_PROPERTY(QList<EventTypeId> loggedEventTypeIds READ loggedEventTypeIds USER true)

public:
    enum ThingError {
        ThingErrorNoError,
        ThingErrorPluginNotFound,
        ThingErrorVendorNotFound,
        ThingErrorThingNotFound,
        ThingErrorThingClassNotFound,
        ThingErrorActionTypeNotFound,
        ThingErrorStateTypeNotFound,
        ThingErrorEventTypeNotFound,
        ThingErrorThingDescriptorNotFound,
        ThingErrorMissingParameter,
        ThingErrorInvalidParameter,
        ThingErrorSetupFailed,
        ThingErrorDuplicateUuid,
        ThingErrorCreationMethodNotSupported,
        ThingErrorSetupMethodNotSupported,
        ThingErrorHardwareNotAvailable,
        ThingErrorHardwareFailure,
        ThingErrorAuthenticationFailure,
        ThingErrorThingInUse,
        ThingErrorThingInRule,
        ThingErrorThingIsChild,
        ThingErrorPairingTransactionIdNotFound,
        ThingErrorParameterNotWritable,
        ThingErrorItemNotFound,
        ThingErrorItemNotExecutable,
        ThingErrorUnsupportedFeature,
        ThingErrorTimeout,
    };
    Q_ENUM(ThingError)

    enum ThingSetupStatus {
        ThingSetupStatusNone,
        ThingSetupStatusInProgress,
        ThingSetupStatusComplete,
        ThingSetupStatusFailed,
    };
    Q_ENUM(ThingSetupStatus)

    ~Thing() override;

    ThingId id() const;
    ThingClassId thingClassId() const;
    PluginId pluginId() const;

    ThingClass thingClass() const;

    Q_INVOKABLE QString name() const;
    Q_INVOKABLE void setName(const QString &name);

    ParamList params() const;
    bool hasParam(const ParamTypeId &paramTypeId) const;
    void setParams(const ParamList &params);

    QVariant paramValue(const ParamTypeId &paramTypeId) const;
    void setParamValue(const ParamTypeId &paramName, const QVariant &value);

    Q_INVOKABLE ParamList settings() const;
    Q_INVOKABLE bool hasSetting(const ParamTypeId &paramTypeId) const;
    Q_INVOKABLE void setSettings(const ParamList &settings);

    Q_INVOKABLE QVariant setting(const ParamTypeId &paramTypeId) const;
    Q_INVOKABLE QVariant setting(const QString &paramName) const;
    Q_INVOKABLE void setSettingValue(const ParamTypeId &paramTypeId, const QVariant &value);
    Q_INVOKABLE void setSettingValue(const QString &paramName, const QVariant &value);

    States states() const;
    bool hasState(const StateTypeId &stateTypeId) const;
    void setStates(const States &states);

    Q_INVOKABLE QVariant stateValue(const StateTypeId &stateTypeId) const;
    Q_INVOKABLE QVariant stateValue(const QString &stateName) const;
    Q_INVOKABLE void setStateValue(const StateTypeId &stateTypeId, const QVariant &value);
    Q_INVOKABLE void setStateValue(const QString &stateName, const QVariant &value);

    Q_INVOKABLE State state(const StateTypeId &stateTypeId) const;
    Q_INVOKABLE State state(const QString &stateName) const;

    QList<EventTypeId> loggedEventTypeIds() const;

    ThingId parentId() const;
    void setParentId(const ThingId &parentId);

    // Deprecated
    bool setupComplete() const;
    bool autoCreated() const;

    ThingSetupStatus setupStatus() const;
    ThingError setupError() const;
    QString setupDisplayMessage() const;

public slots:
    void emitEvent(const EventTypeId &eventTypeId, const ParamList &params = ParamList());

signals:
    void stateValueChanged(const StateTypeId &stateTypeId, const QVariant &value);
    void settingChanged(const ParamTypeId &paramTypeId, const QVariant &value);
    void nameChanged();
    void setupStatusChanged();
    void eventTriggered(const Event &event);

private:
    friend class ThingManager;
    friend class ThingManagerImplementation;
    Thing(const PluginId &pluginId, const ThingClass &thingClass, const ThingId &id, QObject *parent = nullptr);
    Thing(const PluginId &pluginId, const ThingClass &thingClass, QObject *parent = nullptr);

    void setSetupStatus(ThingSetupStatus status, ThingError setupError, const QString &displayMessage = QString());
    void setLoggedEventTypeIds(const QList<EventTypeId> loggedEventTypeIds);
    void setStateValueFilter(const StateTypeId &stateTypeId, Types::StateValueFilter filter);

private:
    ThingClass m_thingClass;
    PluginId m_pluginId;
    ThingId m_id;
    ThingId m_parentId;
    QString m_name;
    ParamList m_params;
    ParamList m_settings;
    States m_states;
    bool m_autoCreated = false;

    ThingSetupStatus m_setupStatus = ThingSetupStatusNone;
    ThingError m_setupError = ThingErrorNoError;
    QString m_setupDisplayMessage;

    QList<EventTypeId> m_loggedEventTypeIds;
    QHash<StateTypeId, StateValueFilter*> m_stateValueFilters;
};

QDebug operator<<(QDebug dbg, Thing *device);

class LIBNYMEA_EXPORT Things: public QList<Thing*>
{
    Q_GADGET
    Q_PROPERTY(int count READ count)
public:
    Things() = default;
    Things(const QList<Thing *> &other);
    Thing* findById(const ThingId &id);
    Thing* findByParams(const ParamList &params) const;
    Things filterByParam(const ParamTypeId &paramTypeId, const QVariant &value = QVariant());
    Things filterByThingClassId(const ThingClassId &thingClassId);
    Things filterByParentId(const ThingId &thingId);
    Things filterByInterface(const QString &interface);
    Q_INVOKABLE QVariant get(int index) const;
    Q_INVOKABLE void put(const QVariant &variant);
};

Q_DECLARE_METATYPE(Thing::ThingError)

#endif
