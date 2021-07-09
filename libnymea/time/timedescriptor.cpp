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
    \class TimeDescriptor
    \brief A \l{TimeDescriptor} is used to describe times.

    \ingroup time

    Times can be described by events, either single events or recurring events or time frames. Time
    events are described using \l{TimeEventItem}{TimeEventItems} while time frames are desceibed
    using \l{CalendarItem}{CalendarItems}.
    A single \l{TimeDescriptor} van have either a list of \l{TimeEventItem}{TimeEventItems} or a list
    of \l{CalendarItem}{CalendarItems} but never both at the same time.

    \sa TimeEventItem, CalendarItem
*/

#include "timedescriptor.h"

#include <QDebug>

/*! Constructs an invalid \l{TimeDescriptor}.*/
TimeDescriptor::TimeDescriptor()
{

}

/*! Returns the list of \l{TimeEventItem}{TimeEventItems} of this \l{TimeDescriptor}.*/
TimeEventItems TimeDescriptor::timeEventItems() const
{
    return m_timeEventItems;
}

/*! Set the list of \l{TimeEventItem}{TimeEventItems} of this \l{TimeDescriptor} to the given \a timeEventItems.*/
void TimeDescriptor::setTimeEventItems(const TimeEventItems &timeEventItems)
{
    m_timeEventItems = timeEventItems;
}

/*! Returns the list of \l{CalendarItem}{CalendarItems} of this \l{TimeDescriptor}.*/
CalendarItems TimeDescriptor::calendarItems() const
{
    return m_calendarItems;
}

/*! Set the list of \l{CalendarItem}{CalendarItems} of this \l{TimeDescriptor} to the given \a calendarItems.*/
void TimeDescriptor::setCalendarItems(const CalendarItems &calendarItems)
{
    m_calendarItems = calendarItems;
}

/*! Returns true if either the calendarItems list is not empty or the timeEventItems list.*/
bool TimeDescriptor::isValid() const
{
    return !m_timeEventItems.isEmpty() != !m_calendarItems.isEmpty();
}

/*! Returns true if the calendarItems list and the timeEventItems list is empty.*/
bool TimeDescriptor::isEmpty() const
{
    return m_calendarItems.isEmpty() && m_timeEventItems.isEmpty();
}

/*! Returns true if this \l{TimeDescriptor} is valid for the given \a dateTime. A \l{TimeDescriptor} is
    valid if the \l{TimeEventItem}{TimeEventItems} or \l{CalendarItem}{CalendarItems} match
    the given \a dateTime since the \a lastEvaluationTime.
*/
bool TimeDescriptor::evaluate(const QDateTime &lastEvaluationTime, const QDateTime &dateTime) const
{
    // If there are calendarItems (always OR connected)
    if (!m_calendarItems.isEmpty()) {
        foreach (const CalendarItem &calendarItem, m_calendarItems) {
            if (calendarItem.evaluate(dateTime)) {
                return true;
            }
        }
    }

    // If there are timeEventItems (always OR connected)
    if (!m_timeEventItems.isEmpty()) {
        foreach (const TimeEventItem &timeEventItem, m_timeEventItems) {
            if (timeEventItem.evaluate(lastEvaluationTime, dateTime)) {
                return true;
            }
        }
    }

    return false;
}

/*! Print a TimeDescriptor including the full lists of CalendarItems and TimeEventItems to QDebug. */
QDebug operator<<(QDebug dbg, const TimeDescriptor &timeDescriptor)
{
    dbg.nospace() << "TimeDescriptor (TimeEventItems:" << timeDescriptor.timeEventItems().count() << ", CalendarItems:" << timeDescriptor.calendarItems().count() << ")" << endl;
    for (int i = 0; i < timeDescriptor.timeEventItems().count(); i++) {
        dbg.nospace() << "  " << i << ": " << timeDescriptor.timeEventItems().at(i);
    }
    for (int i = 0; i < timeDescriptor.calendarItems().count(); i++) {
        dbg.nospace() << "  " << i << ": " << timeDescriptor.calendarItems().at(i);
    }
    return dbg;
}
