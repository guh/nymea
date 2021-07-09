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

#ifndef TIMEEVENTITEM_H
#define TIMEEVENTITEM_H

#include <QDateTime>
#include <QVariant>

#include "repeatingoption.h"

class TimeEventItem
{
    Q_GADGET
    Q_PROPERTY(QDateTime datetime READ dateTime WRITE setDateTime USER true)
    Q_PROPERTY(QTime time READ time WRITE setTime USER true)
    Q_PROPERTY(RepeatingOption repeating READ repeatingOption WRITE setRepeatingOption USER true)

public:
    TimeEventItem();

    QDateTime dateTime() const;
    void setDateTime(const QDateTime &dateTime);

    QTime time() const;
    void setTime(const QTime &time);

    RepeatingOption repeatingOption() const;
    void setRepeatingOption(const RepeatingOption &repeatingOption);

    // TODO spectioalDayTime

    bool isValid() const;

    bool evaluate(const QDateTime &lastEvaluationTime, const QDateTime &dateTime) const;

private:
    QDateTime m_dateTime;
    QTime m_time;

    RepeatingOption m_repeatingOption;
};
Q_DECLARE_METATYPE(TimeEventItem)

class TimeEventItems: public QList<TimeEventItem>
{
    Q_GADGET
    Q_PROPERTY(int count READ count)
public:
    TimeEventItems();
    TimeEventItems(const QList<TimeEventItem> &other);
    Q_INVOKABLE QVariant get(int index) const;
    Q_INVOKABLE void put(const QVariant &variant);
};
Q_DECLARE_METATYPE(TimeEventItems)

QDebug operator<<(QDebug dbg, const TimeEventItem &timeEventItem);


#endif // TIMEEVENTITEM_H
