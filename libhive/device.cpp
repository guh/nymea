#include "device.h"

#include <QDebug>

Device::Device(const QUuid &pluginId, const QUuid &id, const QUuid &deviceClassId, QObject *parent):
    QObject(parent),
    m_id(id),
    m_deviceClassId(deviceClassId),
    m_pluginId(pluginId)
{

}

Device::Device(const QUuid &pluginId, const QUuid &deviceClassId, QObject *parent):
    QObject(parent),
    m_id(QUuid::createUuid()),
    m_deviceClassId(deviceClassId),
    m_pluginId(pluginId)
{

}

QUuid Device::id() const
{
    return m_id;
}

QUuid Device::deviceClassId() const
{
    return m_deviceClassId;
}

QUuid Device::pluginId() const
{
    return m_pluginId;
}

QString Device::name() const
{
    return m_name;
}

void Device::setName(const QString &name)
{
    m_name = name;
}

QVariantMap Device::params() const
{
    return m_params;
}

void Device::setParams(const QVariantMap &params)
{
    m_params = params;
}

QList<State> Device::states() const
{
    return m_states;
}

void Device::setStates(const QList<State> &states)
{
    m_states = states;
}

QVariant Device::stateValue(const QUuid &stateTypeId) const
{
    foreach (const State &state, m_states) {
        if (state.stateTypeId() == stateTypeId) {
            return state.value();
        }
    }
    return QVariant();
}

void Device::setStateValue(const QUuid &stateTypeId, const QVariant &value)
{
    qDebug() << "setting state for id" << stateTypeId;
    for (int i = 0; i < m_states.count(); ++i) {
        qDebug() << "got state id" << m_states.at(i).stateTypeId();
        if (m_states.at(i).stateTypeId() == stateTypeId) {
            State newState(stateTypeId, m_id);
            newState.setValue(value);
            m_states[i] = newState;
            qDebug() << "set state for device" << value;
            return;
        }
    }
    qDebug() << "failed setting state" << value;
}
