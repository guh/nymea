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
    \class CalendarItem
    \brief Describes a clendar item.

    \ingroup time

    \sa TimeDescriptor, TimeEventItem
*/

#include "calendaritem.h"
#include "loggingcategories.h"

#include <QDebug>

/*! Construct a invalid \l{CalendarItem}. */
CalendarItem::CalendarItem():
    m_duration(0)
{

}

/*! Returns the date time of this \l{CalendarItem}. */
QDateTime CalendarItem::dateTime() const
{
    return m_dateTime;
}

/*! Sets the \a dateTime of this \l{CalendarItem}. */
void CalendarItem::setDateTime(const QDateTime &dateTime)
{
    m_dateTime = dateTime;
}

/*! Returns the start time of this \l{CalendarItem}. */
QTime CalendarItem::startTime() const
{
    return m_startTime;
}

/*! Sets the \a startTime of this \l{CalendarItem}. */
void CalendarItem::setStartTime(const QTime &startTime)
{
    m_startTime = startTime;
}

/*! Returns the duratiorn of this \l{CalendarItem}. */
uint CalendarItem::duration() const
{
    return m_duration;
}

/*! Sets the \a duration of this \l{CalendarItem}. */
void CalendarItem::setDuration(const uint &duration)
{
    m_duration = duration;
}

/*! Returns the \l{RepeatingOption} of this \l{CalendarItem}. */
RepeatingOption CalendarItem::repeatingOption() const
{
    return m_repeatingOption;
}

/*! Sets the \a repeatingOption of this \l{CalendarItem}. */
void CalendarItem::setRepeatingOption(const RepeatingOption &repeatingOption)
{
    m_repeatingOption = repeatingOption;
}

/*! Returns true if this \l{CalendarItem} is valid. A \l{CalendarItem} is invalid
    if start time and datetime are set or if the duration is 0.
*/
bool CalendarItem::isValid() const
{
    // If dateTime AND a repeating option definend...
    if (m_dateTime.isValid() && !repeatingOption().isEmtpy())
        // ...only repeating mode yearly is allowed for dateTime
        if (repeatingOption().mode() != RepeatingOption::RepeatingModeYearly)
            return false;

    return (!m_startTime.isValid() != !m_dateTime.isValid()) && m_duration > 0;
}

/*! Returns true, if the given \a dateTime matches this \l{CalendarItem}. */
bool CalendarItem::evaluate(const QDateTime &dateTime) const
{
    if (m_startTime.isValid()) {

        switch (m_repeatingOption.mode()) {
        case RepeatingOption::RepeatingModeNone:
            // If there is no repeating option, we assume it is meant daily.
            return evaluateDaily(dateTime);
        case RepeatingOption::RepeatingModeHourly:
            return evaluateHourly(dateTime);
        case RepeatingOption::RepeatingModeDaily:
            return evaluateDaily(dateTime);
        case RepeatingOption::RepeatingModeWeekly:
            return evaluateWeekly(dateTime);
        case RepeatingOption::RepeatingModeMonthly:
            return evaluateMonthly(dateTime);
        case RepeatingOption::RepeatingModeYearly:
            return evaluateYearly(dateTime);
        }

    }
    if (m_repeatingOption.mode() == RepeatingOption::RepeatingModeYearly)
        return evaluateYearly(dateTime);

    return dateTime >= m_dateTime && dateTime < m_dateTime.addSecs(duration() * 60);
}

bool CalendarItem::evaluateHourly(const QDateTime &dateTime) const
{
    // If the duration is longer than a hour, this calendar item is always true
    // 1 hour has 60 minutes
    if (duration() >= 60)
        return true;

    QDateTime startDateTime = QDateTime(dateTime.date(), QTime(dateTime.time().hour(), startTime().minute()));
    QDateTime endDateTime = startDateTime.addSecs(duration() * 60);

    bool timeValid = dateTime >= startDateTime && dateTime < endDateTime;
    bool weekdayValid = repeatingOption().evaluateWeekDay(dateTime);
    bool monthdayValid = repeatingOption().evaluateMonthDay(dateTime);

    return timeValid && weekdayValid && monthdayValid;
}

bool CalendarItem::evaluateDaily(const QDateTime &dateTime) const
{
    // If the duration is longer than a day, this calendar item is always true
    // 1 day has 1440 minutes
    if (duration() >= 1440)
        return true;

    // Get todays startTime
    QDateTime startDateTime = dateTime;
    startDateTime.setTime(startTime());
    QDateTime endDateTime = startDateTime.addSecs(duration() * 60);

    // Get yesterdays startTime for day overlapping clendaritems
    QDateTime startDateTimeYesterday = dateTime.addDays(-1);
    startDateTimeYesterday.setTime(startTime());
    QDateTime endDateTimeYesterday = startDateTimeYesterday.addSecs(duration() * 60);

    bool todayValid = dateTime >= startDateTime && dateTime < endDateTime;
    bool yesterdayValid = dateTime >= startDateTimeYesterday && dateTime < endDateTimeYesterday;

    return todayValid || yesterdayValid;
}

bool CalendarItem::evaluateWeekly(const QDateTime &dateTime) const
{
    // If the duration is longer than a week, this calendar item is always true
    // 1 week has 10080 minutes
    if (duration() >= 10080)
        return true;

    // Get the first day of this week with the correct start time
    QDateTime weekStartDateTime = dateTime.addDays(-dateTime.date().dayOfWeek());
    weekStartDateTime.setTime(startTime());

    // Check each week day in the list
    foreach (const int &weekDay, repeatingOption().weekDays()) {
        QDateTime startDateTime = weekStartDateTime.addDays(weekDay);
        QDateTime endDateTime = startDateTime.addSecs(duration() * 60);

        // Check if dateTime matches for this week
        if (dateTime >= startDateTime && dateTime < endDateTime)
            // Return true if the current time is between start
            // and end of this calendar item
            return true;

        // Check if this calendar item overlaps a week...
        if (startDateTime.date().weekNumber() != endDateTime.date().weekNumber()) {
            // ...jump one week back in to the past
            QDateTime startDateTimePreviousWeek = startDateTime.addDays(-7);
            QDateTime endDateTimePreviousWeek = startDateTimePreviousWeek.addSecs(duration() * 60);

            if (dateTime >= startDateTimePreviousWeek && dateTime < endDateTimePreviousWeek)
                // Return true if the current time is between start
                // and end of this calendar item from the previouse week
                return true;

        }
    }

    return false;
}

bool CalendarItem::evaluateMonthly(const QDateTime &dateTime) const
{
    // Get the first day of this month with the correct start time
    QDateTime monthStartDateTime = dateTime;
    monthStartDateTime.setDate(QDate(dateTime.date().year(), dateTime.date().month(), 1));
    monthStartDateTime.setTime(m_startTime);

    // Check each month day in the list
    foreach (const int &monthDay, repeatingOption().monthDays()) {
        QDateTime startDateTime = monthStartDateTime.addDays(monthDay - 1);
        QDateTime endDateTime = startDateTime.addSecs(duration() * 60);

        // Check if this calendar item starts in the future...
        if (startDateTime > dateTime) {
            //...go one month back
            startDateTime = startDateTime.addMonths(-1);
            endDateTime =  startDateTime.addSecs(duration() * 60);
        }

        // Check if dateTime already matches for this month
        if (dateTime >= startDateTime && dateTime < endDateTime)
            // Return true if the current time is between start
            // and end of this calendar item
            return true;

    }

    return false;
}

bool CalendarItem::evaluateYearly(const QDateTime &dateTime) const
{
    // check for this year
    QDateTime startDateTimeThisYear = dateTime;
    startDateTimeThisYear.setDate(QDate(dateTime.date().year(), m_dateTime.date().month(), m_dateTime.date().day()));
    startDateTimeThisYear.setTime(m_dateTime.time());
    QDateTime endDateTimeThisYear = startDateTimeThisYear.addSecs(duration() * 60);

    // check if we are in the interval of this year
    if (dateTime >= startDateTimeThisYear && dateTime < endDateTimeThisYear)
        return true;

    // check if this calendaritem overlaps a year
    if (startDateTimeThisYear.date().year() != endDateTimeThisYear.date().year()) {
        // go one year in to the past
       QDateTime startDateTimePreviousYear = startDateTimeThisYear.addYears(-1);
       QDateTime endDateTimePreviousYear = startDateTimePreviousYear.addSecs(duration() * 60);

       // check if we are in the interval of the previous year
       if (dateTime >= startDateTimePreviousYear && dateTime < endDateTimePreviousYear)
           return true;

    }

    return false;
}

/*! Print a CalendarItem to QDebug. */
QDebug operator<<(QDebug dbg, const CalendarItem &calendarItem)
{
    dbg.nospace() << "CalendarItem (StartTime:" << calendarItem.startTime() << ", DateTime:" << calendarItem.dateTime().toString() << ", " << calendarItem.repeatingOption() << ", Duration:" << calendarItem.duration() << ")";
    return dbg;
}



CalendarItems::CalendarItems()
{

}

CalendarItems::CalendarItems(const QList<CalendarItem> &other): QList<CalendarItem>(other)
{

}

QVariant CalendarItems::get(int index) const
{
    return QVariant::fromValue(at(index));
}

void CalendarItems::put(const QVariant &variant)
{
    append(variant.value<CalendarItem>());
}
