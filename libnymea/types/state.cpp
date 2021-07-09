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

/*!
  \class State
  \brief Holds the value of a State of a \l{Thing}.

  \ingroup nymea-types

  States hold the state values for devices. A State is associated to a \l{Thing} by
  the \l{State::thingId()} and represents the value of a state described in a \l{StateType}

  \sa StateType, StateDescriptor
*/

#include "state.h"

State::State()
{

}

/*!
    Constructs a State reflecting the \l{StateType} given by \a stateTypeId
    and associated with the \l{Device} given by \a deviceId
*/
State::State(const StateTypeId &stateTypeId, const ThingId &deviceId):
    m_stateTypeId(stateTypeId),
    m_thingId(deviceId)
{
}

/*! Returns the id of the StateType describing this State. */
StateTypeId State::stateTypeId() const
{
    return m_stateTypeId;
}

/*! Returns the \l{ThingId} of the thing of this State. */
ThingId State::thingId() const
{
    return m_thingId;
}

/*! Returns the state's value. */
QVariant State::value() const
{
    return m_value;
}

/*! Set the state's value to \a value. */
void State::setValue(const QVariant &value)
{
    m_value = value;
}

Types::StateValueFilter State::filter() const
{
    return m_filter;
}

void State::setFilter(Types::StateValueFilter filter)
{
    m_filter = filter;
}

/*! Writes the stateTypeId, the deviceId and the value of the given \a state to \a dbg. */
QDebug operator<<(QDebug dbg, const State &state)
{
    dbg.nospace() << "State(StateTypeId: " << state.stateTypeId().toString() << ", DeviceId:" << state.thingId() << ", value:" << state.value() << ")";
    return dbg.space();
}

/*! Writes each stateTypeId, deviceId and value of the given \a states to \a dbg. */
QDebug operator<<(QDebug dbg, const QList<State> &states)
{
    dbg.nospace() << "StateList (count:" << states.count() << ")";
    for (int i = 0; i < states.count(); i++ ) {
        dbg.nospace() << "     " << i << ": " << states.at(i);
    }

    return dbg.space();
}

States::States()
{

}

States::States(const QList<State> &other): QList<State>(other)
{

}

QVariant States::get(int index) const
{
    return QVariant::fromValue(at(index));
}

void States::put(const QVariant &variant)
{
    append(variant.value<State>());
}

QVariant States::stateValue(const StateTypeId &stateTypeId)
{
    foreach (const State & state, *this) {
        if (state.stateTypeId() == stateTypeId) {
            return state.value();
        }
    }
    return QVariant();
}
