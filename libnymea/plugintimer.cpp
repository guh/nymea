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
    \class PluginTimer
    \brief The plugin timer class for plugins.

    \ingroup hardware
    \inmodule libnymea

    The plugin timer allows to trigger repeating actions in an integration plugin. This
    timer does not represent a precise timer and should be used for not time critical things.
    The PluginTimerManager will schedule the requested timer as needed and trigger the timeout() method.


    \chapter Example

    In order to do something repeatedly in an \l{IntegrationPlugin} you can register a new PluginTimer like this:

    \tt integrationpluginexample.h

    \code
    #include "plugintimer.h"

    class IntegrationPluginExample : public IntegrationPlugin
    {
    ...
    public:
        void init() override;

    private:
        PluginTimer *m_pluginTimer = nullptr;
    ...

    };
    \endcode

    \tt integrationpluginexample.cpp

    \code

    void IntegrationPluginExample::init() {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this](){
            ...
        });
    }

    IntegrationPluginExample::~IntegrationPluginExample()
    {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
    }

    \endcode

    \sa PluginTimerManager
*/

/*! \fn PluginTimer::~PluginTimer();
    The virtual destructor of the PluginTimer. You should always use the PluginTimerManager for destroying a PluginTimer object.
*/

/*! \fn int PluginTimer::interval() const;
    This property holds the timeout interval in seconds.
*/

/*! \fn int PluginTimer::currentTick() const;
    Returns the current timer tick of this PluginTimer in seconds.
*/

/*! \fn bool PluginTimer::running() const;
    Returns true, if this PluginTimer is currently running.

    \sa start(), stop(), pause(), resume(), reset()
*/

/*! \fn void PluginTimer::timeout();
    This signal will be emitted if the timer timeouted.
*/

/*! \fn void PluginTimer::currentTickChanged(const int &currentTick);
    This signal will be emitted whenever the \a currentTick of this PluginTimer changed.

    \sa currentTick()
*/

/*! \fn void PluginTimer::runningChanged(const bool &running);
    This signal will be emitted whenever the \a running status of this PluginTimer changed.

    \sa running()
*/

/*! \fn void PluginTimer::pausedChanged(const bool &paused);
    This signal will be emitted whenever the \a paused status of this PluginTimer changed.

    \sa running()
*/

/*! \fn void PluginTimer::reset();
    This method resets the timer to 0. This method does not change the current running state or the configured interval.
*/

/*! \fn void PluginTimer::pause();
    Pauses the timer.

    \sa running()
*/

/*! \fn void PluginTimer::start();
    Starts the timer.
*/

/*!
    \fn void PluginTimer::stop();
    Stops the timer.

    \sa running(), runningChanged()
*/


/*! \fn void PluginTimer::resume();
    Resumes the timer. If the timer was not on paused, this method has no effect.

    \sa pause()
*/



/*!
    \class PluginTimerManager
    \brief The plugin timer manager for guh.

    \ingroup hardware
    \inmodule libnymea

    The plugin timer manager allows to register and unregister generic timers for device plugins. In order to save
    resources the PluginTimerManager is responsible to schedule the timers appropriate and stop them if the HardwareResource
    gets disabled.

    You can find an example \l{PluginTimer}{here}.

    \sa PluginTimer, HardwareResource
*/

/*! \fn PluginTimerManager::~PluginTimerManager();
    The virtual destructor of the PluginTimerManager.
*/

/*! \fn PluginTimer *PluginTimerManager::registerTimer(int seconds = 60);
    Registers a new PluginTimer with an interval of the given \a seconds parameter. Returns a new PluginTimer object.

    \sa unregisterTimer()
*/

/*! \fn void unregisterTimer(PluginTimer *timer = nullptr);
    Unregisters the given \a timer. The PluginTimerManager will delete the object once the unregister process is complete.

    \sa registerTimer()
*/

#include "plugintimer.h"
#include "loggingcategories.h"

/*! Constructs a \l{PluginTimerManager} with the given \a parent. */
PluginTimerManager::PluginTimerManager(QObject *parent) :
    HardwareResource("PluginTimerManager", parent)
{

}

/*! Constructs a PluginTimer with the given \a parent. */
PluginTimer::PluginTimer(QObject *parent) :
    QObject(parent)
{

}
