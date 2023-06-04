/*==============================================================================
MIT License

Copyright (c) 2023 Trevor Monk

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
==============================================================================*/

/*!
 * @defgroup timer timer
 * @brief State Machine Timer functions
 * @{
 */

/*============================================================================*/
/*!
@file timer.c

    Dynamic State Machine Timer Management

    The state machine timer component provides functions
    for manipulating timers.

    - create one-shot timer
    - create repeating tick timer
    - delete timer
    - re-trigger tick timer

*/
/*============================================================================*/

#ifndef TIMER_H
#define TIMER_H

/*==============================================================================
        Includes
==============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include "SMTypes.h"
#include "timer.h"

/*==============================================================================
       Function declarations
==============================================================================*/


/*==============================================================================
       Definitions
==============================================================================*/

/*! Maximum number of timers allowed */
#define MAX_TIMERS ( 255 )

/*==============================================================================
       File Scoped Variables
==============================================================================*/

static timer_t timers[MAX_TIMERS] = {0};

/*==============================================================================
       Function definitions
==============================================================================*/

/*============================================================================*/
/*  DeleteTimer                                                               */
/*!
    Delete a one-shot or tick timer

    The DeleteTimer function cancels the specified one-shot or tick timer

@param[in]
    id
        id of the timer to delete

@retval EOK the timer was deleted
@retval EINVAL invalid arguments

==============================================================================*/
int DeleteTimer( int id )
{
    timer_t timerID;
    int result = EINVAL;

    if( ( id > 0 ) && ( id < MAX_TIMERS ) )
    {
        timerID = timers[id];

        if ( timer_delete( timerID ) == 0 )
        {
            result = EOK;
        }
        else
        {
            result = errno;
        }
    }
    else
    {
        result = ENOENT;
    }

    return result;
}

/*============================================================================*/
/*  CreateTimer                                                               */
/*!
    Create a one-shot timer

    The CreateOneShotTimer creates a timer which will fire once
    after a specified interval

@param[in]
    id
        id of the timer to create

@param[in]
    interval
        timer interval in milliseconds

@retval EOK the timer was created
@retval EINVAL invalid arguments

==============================================================================*/
int CreateTimer( int id, int timeoutms )
{
    struct sigevent te;
    struct itimerspec its;
    int sigNo = TIMER_NOTIFICATION;
    long secs;
    long msecs;
    timer_t *timerID;
    int result = EINVAL;

    secs = timeoutms / 1000;
    msecs = timeoutms % 1000;

    if( ( id > 0 ) && ( id < MAX_TIMERS ) )
    {
        if( timers[id] != 0 )
        {
            DeleteTimer( id );
        }

        timerID = &timers[id];

        /* Set and enable alarm */
        te.sigev_notify = SIGEV_SIGNAL;
        te.sigev_signo = sigNo;
        te.sigev_value.sival_int = id;
        timer_create(CLOCK_REALTIME, &te, timerID);

        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        its.it_value.tv_sec = secs;
        its.it_value.tv_nsec = msecs * 1000000L;
        timer_settime(*timerID, 0, &its, NULL);

        result = EOK;
    }
    else
    {
        result = ENOENT;
    }

    return result;
}

/*============================================================================*/
/*  CreateTick                                                                */
/*!
    Create a tick timer

    The CreateTick creates a timer which will fire repeatedly
    at a specified interval

@param[in]
    id
        id of the timer to create

@param[in]
    interval
        timer interval in milliseconds

@retval EOK the timer was created
@retval EINVAL invalid arguments

==============================================================================*/
int CreateTick( int id, int timeoutms )
{
    struct sigevent te;
    struct itimerspec its;
    int sigNo = TIMER_NOTIFICATION;
    long secs;
    long msecs;
    timer_t *timerID;
    int result = EINVAL;

    secs = timeoutms / 1000;
    msecs = timeoutms % 1000;

    if( ( id > 0 ) && ( id < MAX_TIMERS ) )
    {
        timerID = &timers[id];
        if( timers[id] != 0 )
        {
            DeleteTimer( id );
        }


        /* Set and enable alarm */
        te.sigev_notify = SIGEV_SIGNAL;
        te.sigev_signo = sigNo;
        te.sigev_value.sival_int = id;
        timer_create(CLOCK_REALTIME, &te, timerID);

        its.it_interval.tv_sec = secs;
        its.it_interval.tv_nsec = msecs * 1000000L;
        its.it_value.tv_sec = secs;
        its.it_value.tv_nsec = msecs * 1000000L;
        timer_settime(*timerID, 0, &its, NULL);

        result = EOK;
    }
    else
    {
        result = ENOENT;
    }

    return result;
}

/*! @}
 * end of timer group */
