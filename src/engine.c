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
 * @defgroup engine engine
 * @brief State Machine Engine functions
 * @{
 */

/*============================================================================*/
/*!
@file engine.c

    Dynamic State Machine Processing Engine

    The state machine engine component provides functions
    for operating the state machine.

    The following handled:

    - wait for signals
    - execute entry actions
    - execute exit actions
    - evaluate state transition rules
    - change states
    - manage timers


*/
/*============================================================================*/

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
#include "SMTypes.h"

/*==============================================================================
       Function declarations
==============================================================================*/

static int waitSignal( int *signum, int *id );
static State *FindState( StateMachine *pStateMachine, char *stateName );
static int HandleSignal( StateMachine *pStateMachine, int signum, int id );
static int CheckTransition( StateMachine *pStateMachine,
                            Transition *trans,
                            int signum,
                            int id );
static int CheckInConditions( Variable *pVariable, int signum, int id );
static int EnterState( StateMachine *pStateMachine, char *targetState );
static int ExitState( StateMachine *pStateMachine );

/*==============================================================================
       Definitions
==============================================================================*/


/*==============================================================================
       Function definitions
==============================================================================*/

/*============================================================================*/
/*  RunStateMachine                                                           */
/*!
    Run the state machine

    The RunStateMachine function executes the state machine

@param[in]
    pStateMachine
        Pointer to the state machine to execute

@retval EOK the state machine completed normally
@retval EINVAL invalid arguments
@retval other state machine error

==============================================================================*/
int RunStateMachine( StateMachine *pStateMachine )
{
    int result = EINVAL;
    int signum;
    int id;

    if ( pStateMachine != NULL )
    {
        /* assign the current state to the 'init' state */
        result = EnterState( pStateMachine, "init" );
        if ( result == EOK )
        {
            /* run the state machine forever */
            while( true )
            {
                /* wait for a signal to occur */
                waitSignal( &signum, &id );

                if( pStateMachine->verbose )
                {
                    fprintf( stdout,
                             "Received signal %d id = %d\n",
                             signum,
                             id );
                }

                /* handle the received signal */
                result = HandleSignal( pStateMachine, signum, id );
                if( pStateMachine->verbose )
                {
                    fprintf( stdout,
                             "signal %d %d: %s\n",
                             signum,
                             id,
                             strerror(result) );
                }
            }
        }
        else
        {
            syslog( LOG_ERR, "Cannot find init state" );
        }
    }

    return result;
}

/*============================================================================*/
/*  waitSignal                                                                */
/*!
    Wait for a signal from the system

    The waitSignal function waits for either a variable modified
    or timer expired signal from the system

@param[in,out]
    signum
        Pointer to a location to store the received signal

@param[in,out]
    id
        Pointer to a location to store the signal identifier

@retval 0 signal received successfully
@retval -1 an error occurred

==============================================================================*/
static int waitSignal( int *signum, int *id )
{
    sigset_t mask;
    siginfo_t info;
    int result = EINVAL;
    int sig;

    if( ( signum != NULL ) &&
        ( id != NULL ) )
    {
        /* create an empty signal set */
        sigemptyset( &mask );

        /* timer notification */
        sigaddset( &mask, TIMER_NOTIFICATION );

        /* modified notification */
        sigaddset( &mask, VAR_NOTIFICATION );

        /* apply signal mask */
        sigprocmask( SIG_BLOCK, &mask, NULL );

        /* wait for the signal */
        sig = sigwaitinfo( &mask, &info );

        /* return the signal information */
        *signum = sig;
        *id = info.si_value.sival_int;

        /* indicate success */
        result = EOK;
    }

    return result;
}

/*============================================================================*/
/*  FindState                                                                 */
/*!
    Find a state given its name

    The FindState function searches the state machine's state list
    looking for a state with a matching name

@param[in]
    pStateMachine
        pointer to the state machine to search in

@param[in]
    stateName
        pointer to the state name to search for

@retval pointer to the state that was found
@retval NULL if no state was found

==============================================================================*/
static State *FindState( StateMachine *pStateMachine, char *stateName )
{
    State *pState = NULL;

    if ( ( pStateMachine != NULL ) &&
         ( stateName != NULL ) )
    {
        pState = pStateMachine->pStateList;

        while( pState != NULL )
        {
            if( strcmp( pState->id, stateName ) == 0 )
            {
                /* found it! */
                break;
            }

            pState = pState->pNext;
        }
    }

    return pState;
}

/*============================================================================*/
/*  HandleSignal                                                              */
/*!
    Handle a received signal

    The HandleSignal function drives state transitions.
    It searches through the state machine's current state's transitions
    looking for where the received signal is used, if at all, and then
    evaluates the conditions around the signal.  If the conditions
    related to the received signal evaluate to true, then the
    appropriate state transition is executed.

@param[in]
    pStateMachine
        pointer to the state machine

@param[in]
    signum
        the type of signal received

@param[in]
    id
        the identifier of the signal

@retval EINVAL invalid argument
@retval EOK the signal was processed and a transition was executed
@retval ENOTSUP the signal was processed with no transition
@retval ENOENT the signal was not found in this state
@retval ENOTRECOVERABLE the state does not exist

==============================================================================*/
static int HandleSignal( StateMachine *pStateMachine, int signum, int id )
{
    int result = EINVAL;
    Transition *trans;
    State *pState;

    if ( pStateMachine != NULL )
    {
        if ( signum == TIMER_NOTIFICATION )
        {
            /* set the active timer identifier in the var actions
             * so the timer comparison logic can be executed */
            SetTimer( id );
        }

        /* get the current state */
        pState = pStateMachine->pCurrentState;
        if( pState != NULL )
        {
            /* assume we cannot find the signal in the state */
            result = ENOENT;

            /* select the first transition in the state */
            trans = pState->trans;

            while( trans != NULL )
            {
                /* check the signal in the transition */
                result = CheckTransition( pStateMachine, trans, signum, id );
                if ( result == EOK )
                {
                    /* abort the search when a transition has occurred */
                    break;
                }

                /* select the next transition in the state */
                trans = trans->pNext;
            }
        }
        else
        {
            /* we should never get here */
            result = ENOTRECOVERABLE;
        }

        /* clear the active timer */
        SetTimer( 0 );
    }

    return result;
}

/*============================================================================*/
/*  CheckTransition                                                           */
/*!
    Check a transition for a signal

    The CheckTransition function checks a specific transition to see
    if the specified signal is part of its evaluation rules.  If the
    signal is found, the rules are evaluated to determine if the
    transition should be executed.
    If the rules evaluate to true, the transition will be executed
    by:

    1) executing the exit actions of the current state
    2) setting the current state to the target state
    3) executing the entry actions of the target state

@param[in]
    pStateMachine
        pointer to the state machine to search in

@param[in]
    trans
        pointer to the transition to check

@param[in]
    signum
        the type of signal received

@param[in]
    id
        the identifier of the signal.
        This can either be a timer id, or a variable handle

@retval EINVAL invalid argument
@retval EOK the signal was processed and a transition was executed
@retval ENOTSUP the signal was processed with no transition
@retval ENOENT the signal was not found in this transition

==============================================================================*/
static int CheckTransition( StateMachine *pStateMachine,
                            Transition *trans,
                            int signum,
                            int id )
{
    int result = EINVAL;
    char *targetState;
    Variable *pVariable;

    if ( ( trans != NULL ) &&
         ( trans->statename != NULL ) )
    {
        /* get the name of the target state */
        targetState = trans->statename;

        /* check to see if the signal exists in the transition rules */
        result = CheckInConditions( trans->pVariable, signum, id );
        if ( result == EOK )
        {
            /* evaluate the transition rules to see if a transition
             * should occur */
            pVariable = trans->pVariable;
            result = ProcessExpr( pStateMachine->hVarServer, pVariable );
            if( result == EOK )
            {
                /* check the result of the expression */
                result = ( pVariable->obj.val.ui != 0 ) ? EOK : ENOTSUP;
                if ( result == EOK )
                {
                    /* a transition is required.  Process the exit actions
                     * on the current state */
                    result = ExitState( pStateMachine );
                    if ( result == EOK )
                    {
                        /* process the entry actions on the target state */
                        result = EnterState( pStateMachine, targetState );
                    }
                }
            }
        }
    }

    return result;
}

/*============================================================================*/
/*  CheckInConditions                                                         */
/*!
    Check to see if the signal is referenced in the condition list

    The CheckInConditions function iterates through all the
    conditions in the condition list and checks each condition
    to see if it references the specified signal.

@param[in]
    pVariable
        pointer to the variable expression to check

@param[in]
    signum
        the type of signal to search for.  This can either be
        a timer or a variable.

@param[in]
    id
        the identifier of the signal.
        This can either be a timer id, or a variable handle

@retval EINVAL invalid argument
@retval EOK the signal was found in the condition list
@retval ENOENT the signal was not found in the conditon list

==============================================================================*/
static int CheckInConditions( Variable *pVariable, int signum, int id )
{
    int result = EINVAL;

    if ( pVariable != NULL )
    {
        result = ENOENT;

        if ( ( pVariable->operation == VA_TIMER ) &&
             ( signum == TIMER_NOTIFICATION ) &&
             ( pVariable->obj.val.ui == id ) )
        {
            result = EOK;
        }
        else if ( ( pVariable->operation == VA_SYSVAR ) &&
                  ( signum == VAR_NOTIFICATION ) &&
                  ( pVariable->hVar == id ) )
        {
            result = EOK;
        }
        else if ( CheckInConditions( pVariable->left, signum, id ) == EOK )
        {
            result = EOK;
        }
        else if ( CheckInConditions( pVariable->right, signum, id ) == EOK )
        {
            result = EOK;
        }
    }

    return result;
}

/*============================================================================*/
/*  EnterState                                                                */
/*!
    Enter a new state

    The EnterState function searches for a state with the given state name
    and if found, performs the exit actions of the current state, and
    then performs the entry actions of the new state and sets the current
    state to the new state.

@param[in]
    pStateMachine
        pointer to the state machine to search in

@param[in]
    targetState
        name of the target state

@retval EOK successfully transitioned to the new state
@retval ENOENT the target state was not found
@retval EINVAL invalid arguments

==============================================================================*/
static int EnterState( StateMachine *pStateMachine, char *targetState )
{
    int result = EINVAL;
    State *pState;
    StateEntry *entry;

    if ( ( pStateMachine != NULL ) &&
         ( targetState != NULL ) )
    {
        pState = FindState( pStateMachine, targetState );
        if( pState != NULL )
        {
            if ( pStateMachine->verbose )
            {
                fprintf( stdout, "Enter State: %s\n", pState->id );
            }

            pStateMachine->pCurrentState = pState;
            entry = pState->entry;
            if( entry != NULL )
            {
                if ( entry->pStatements != NULL )
                {
                    result = ProcessCompoundStatement(
                                    pStateMachine->hVarServer,
                                    entry->pStatements );
                }
                else
                {
                    result = EOK;
                }
            }
            else
            {
                syslog( LOG_ERR,
                        "State '%s' has no entry actions",
                        targetState );
            }
        }
        else
        {
            syslog( LOG_ERR, "Cannot find state: %s", targetState );
            result = ENOENT;
        }
    }

    return result;
}

/*============================================================================*/
/*  ExitState                                                                 */
/*!
    Exit the current state

    The ExitState function performs the exit actions of the current
    state.

@param[in]
    pStateMachine
        pointer to the state machine

@retval EOK successfully processed the state exit actions
@retval EINVAL invalid arguments

==============================================================================*/
static int ExitState( StateMachine *pStateMachine )
{
    int result = EINVAL;
    State *pState;
    StateExit *exit;
    char *statename;

    if ( pStateMachine != NULL )
    {
        pState = pStateMachine->pCurrentState;
        if ( pState != NULL )
        {
            if ( pStateMachine->verbose )
            {
                fprintf( stdout, "Exit State: %s\n", pState->id );
            }

            exit = pState->exit;
            if ( exit != NULL )
            {
                if ( exit->pStatements != NULL )
                {
                    result = ProcessCompoundStatement(
                                pStateMachine->hVarServer,
                                exit->pStatements );
                }
                else
                {
                    result = EOK;
                }
            }
            else
            {
                statename = ( pState->id != NULL ) ? pState->id : "unknown";
                syslog( LOG_ERR,
                        "State '%s' has no exit actions",
                        statename );
            }
        }
        else
        {
            syslog( LOG_ERR, "No state to exit from" );
        }
    }

    return result;
}

/*! @}
 * end of engine group */
