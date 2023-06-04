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

#ifndef SMTYPES_H
#define SMTYPES_H

/*==============================================================================
        Includes
==============================================================================*/

#include <varserver/varserver.h>
#include <varaction/varaction.h>

/*==============================================================================
        Public Definitions
==============================================================================*/

#ifndef EOK
/*! success response */
#define EOK 0
#endif

/*! timer notification */
#define TIMER_NOTIFICATION SIGRTMIN+5

/*! variable change notification */
#define VAR_NOTIFICATION SIGRTMIN+6

/*! transition */
typedef struct trans
{
    /*! name of state to transition to if conditions allow */
    char *statename;

    /*! pointer to a variable expression */
    Variable *pVariable;

    /*! pointer to the next transition in the list */
    struct trans *pNext;
} Transition;

/*! state exit */
typedef struct state_exit
{
    /*! list of local variable declarations */
    Variable *pDeclarations;

    /*! list of statement execute */
    Statement *pStatements;
} StateExit;

/*! state entry */
typedef struct state_entry
{
    /*! list of local variable declarations */
    Variable *pDeclarations;

    /*! list of statements to execute */
    Statement *pStatements;

} StateEntry;

/*! state definition */
typedef struct _state
{
    /* state identifier */
    char *id;

    /*! pointer to the state entry actions */
    StateEntry *entry;

    /*! pointer to the state transitions */
    Transition *trans;

    /*! pointer to the state exit actions */
    StateExit *exit;

    /*! pointer to the next state in the state machine */
    struct _state *pNext;
} State;

/*! State Machine object */
typedef struct state_machine
{
    /*! handle to the variable server */
    VARSERVER_HANDLE hVarServer;

    /*! current active state in the state machine */
    State *pCurrentState;

    /*! state machine definition file */
    char *filename;

    /*! name of this state machine */
    char *name;

    /*! description of this state machine */
    char *description;

    /*! verbose mode */
    bool verbose;

    /*! pointer to the first state in a list of states */
    State *pStateList;
} StateMachine;

#endif

