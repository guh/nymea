/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stuerz <simon.stuerz@guh.guru>                *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <QTimer>
#include <QObject>
#include <QDateTime>
#include <QTimeZone>

namespace guhserver {

class TimeManager : public QObject
{
    Q_OBJECT
public:
    explicit TimeManager(const QByteArray &timeZone, QObject *parent = 0);

    QByteArray timeZone() const;
    void setTimeZone(const QByteArray &timeZone = QTimeZone::systemTimeZoneId());

    QDateTime currentDateTime() const;
    QTime currentTime() const;
    QDate currentDate() const;

private:
    QTimeZone m_timeZone;
    QDateTime m_dateTime;
    QTimer *m_guhTimer;

signals:
    void tick();
    void dateChanged(const QDate &currentDate);
    void timeChanged(const QTime &currentTime);


private slots:
    void guhTimeout();

};

}

#endif // TIMEMANAGER_H
