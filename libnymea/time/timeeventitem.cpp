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
    \class TimeEventItem
    \brief Describes a time event in a \l{TimeDescriptor}.

    \ingroup time

    \sa TimeDescriptor, CalendarItem
*/

#include "timeeventitem.h"

#include <QDebug>

/*! Constructs an invalid \l{TimeEventItem}. */
TimeEventItem::TimeEventItem()
{

}

/*! Returns the dateTime of this \l{TimeEventItem}. */
QDateTime TimeEventItem::dateTime() const
{
    return m_dateTime;
}

/*! Sets the dateTime of this \l{TimeEventItem} to the given \a timeStamp. */
void TimeEventItem::setDateTime(const QDateTime &dateTime)
{
    m_dateTime = dateTime;
}

/*! Returns the time of this \l{TimeEventItem}. */
QTime TimeEventItem::time() const
{
    return m_time;
}

/*! Sets the \a time of this \l{TimeEventItem}. */
void TimeEventItem::setTime(const QTime &time)
{
    m_time = time;
}

/*! Returns the \l{RepeatingOption} of this \l{TimeEventItem}. */
RepeatingOption TimeEventItem::repeatingOption() const
{
    return m_repeatingOption;
}

/*! Sets the \a repeatingOption of this \l{TimeEventItem}. */
void TimeEventItem::setRepeatingOption(const RepeatingOption &repeatingOption)
{
    m_repeatingOption = repeatingOption;
}

/*! Returns true if this \l{TimeEventItem} is valid. A \l{TimeEventItem} is valid
    if either the \l{time()} or the \l{dateTime()} is set.
*/
bool TimeEventItem::isValid() const
{
    // If dateTime AND a repeating option definend...
    if (m_dateTime.isValid() && !repeatingOption().isEmtpy())
        // ...only repeating mode yearly is allowed for dateTime
        if (repeatingOption().mode() != RepeatingOption::RepeatingModeYearly)
            return false;

    return (!m_dateTime.isNull() != !m_time.isNull());
}

/*! Returns true, if the given \a dateTime matches this \l{TimeEventItem} since the last \a lastEvaluationTime. */
bool TimeEventItem::evaluate(const QDateTime &lastEvaluationTime, const QDateTime &dateTime) const
{
    // Check time matches
    if (m_time.isValid()) {
        switch (m_repeatingOption.mode()) {
            // If there is no repeating option, we assume it is meant daily.
        case RepeatingOption::RepeatingModeNone:
            return lastEvaluationTime.time() < m_time && m_time <= dateTime.time();
        case RepeatingOption::RepeatingModeDaily:
            return lastEvaluationTime.time() < m_time && m_time <= dateTime.time();
        case RepeatingOption::RepeatingModeHourly: {
            QTime begin, vut, end;
            begin.setHMS(0, lastEvaluationTime.time().minute(), lastEvaluationTime.time().second());
            end.setHMS(0, dateTime.time().minute(), dateTime.time().second());
            vut.setHMS(0, m_time.minute(), m_time.second());
            return begin < vut && vut <= end;
        }
        case RepeatingOption::RepeatingModeWeekly:
            return m_repeatingOption.evaluateWeekDay(dateTime) &&
                    lastEvaluationTime.time() < m_time &&
                    m_time <= dateTime.time();
        case RepeatingOption::RepeatingModeMonthly:
            return m_repeatingOption.evaluateMonthDay(dateTime) &&
                    lastEvaluationTime.time() < m_time &&
                    m_time <= dateTime.time();
        case RepeatingOption::RepeatingModeYearly:
            return false;
        }
    }

    // Check dateTime and yearly repeating
    if (m_repeatingOption.mode() == RepeatingOption::RepeatingModeYearly) {
        // adjust the stored year to the current one...
        QDateTime adjustedTime = m_dateTime;
        adjustedTime.setDate(QDate(dateTime.date().year(), m_dateTime.date().month(), m_dateTime.date().day()));
        return lastEvaluationTime < adjustedTime && adjustedTime <= dateTime;
    }

    // Check dateTime matches
    return lastEvaluationTime < m_dateTime && m_dateTime <= dateTime;
}

/*! Print a TimeEvent to QDebug. */
QDebug operator<<(QDebug dbg, const TimeEventItem &timeEventItem)
{
    dbg.nospace() << "TimeEventItem (Time:" << timeEventItem.time() << ", DateTime:" << timeEventItem.dateTime().toString() << ", " << timeEventItem.repeatingOption() << ")" << endl;
    return dbg;
}


TimeEventItems::TimeEventItems()
{

}

TimeEventItems::TimeEventItems(const QList<TimeEventItem> &other): QList<TimeEventItem>(other)
{

}

QVariant TimeEventItems::get(int index) const
{
    return QVariant::fromValue(at(index));
}

void TimeEventItems::put(const QVariant &variant)
{
    append(variant.value<TimeEventItem>());
}
