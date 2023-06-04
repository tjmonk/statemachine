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
 * @defgroup statemachine statemachine
 * @brief Dynamic State Machine Processor
 * @{
 */

/*============================================================================*/
/*!
@file statemachine.c

    Dynamic State Machine Processor

    The State Machine application creates and operates a state
    machine as defined by the state machine configuration file
    provided as an argument to the application.

    The state machine engine supports:

    - variable (signal) based transitions
    - state entry and exit execution blocks
    - combinatorial logic on transitions
    - execution of shell commands on entry/exit of states

    The statee machine engine accepts a user defined state machine
    as an argument and builds the state machine logic dynamically.

    The state machine is event driven, and idle until external
    changes to variables cause state transitions.

    The state machine definition is parsed using flex/bison

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
#include <limits.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <varserver/varserver.h>
#include "SMTypes.h"
#include "engine.h"

/*==============================================================================
       Function declarations
==============================================================================*/
static void usage( char *cmdname );
int yylex(void);
int yyparse(void);
static int ParseStateMachine( char *filename );
static void SetupTerminationHandler( void );
static void TerminationHandler( int signum, siginfo_t *info, void *ptr );
static int ProcessOptions( int argC,
                           char *argV[],
                           StateMachine *pStateMachine );

/*==============================================================================
       Definitions
======================================================================-=======*/
#ifndef EOK
/*! success response */
#define EOK 0
#endif

/*==============================================================================
       File Scoped Variables
==============================================================================*/

/*! pointer to the state machine context */
StateMachine *pStateMachine;

/*==============================================================================
       Function definitions
==============================================================================*/

/*============================================================================*/
/*  main                                                                      */
/*!
    Main entry point for the statemachine application

    @param[in]
        argc
            number of arguments on the command line
            (including the command itself)

    @param[in]
        argv
            array of pointers to the command line arguments

    @return 0

==============================================================================*/
int main(int argC, char *argV[])
{
    pStateMachine = NULL;

    /* initialize the varactions library */
    InitVarAction();

    /* create the state machine instance */
    pStateMachine = (StateMachine *)calloc(1, sizeof( StateMachine ) );
    if ( pStateMachine != NULL )
    {
        /* get a handle to the variable server for transition events */
        pStateMachine->hVarServer =  VARSERVER_Open();
        if ( pStateMachine->hVarServer != NULL )
        {
            /* Process Options */
            ProcessOptions( argC, argV, pStateMachine );

            /* parse the state machine definition */
            ParseStateMachine( pStateMachine->filename );

            /* run the statemachine */
            RunStateMachine( pStateMachine );

            /* we should reach here only if the
             * state machine self-terminates */

            /* close the variable server */
            VARSERVER_Close( pStateMachine->hVarServer );
            pStateMachine->hVarServer = NULL;


            if ( pStateMachine->filename != NULL )
            {
                free( pStateMachine->filename );
                pStateMachine->filename = NULL;
            }
        }
    }

    return 0;
}

/*============================================================================*/
/*  ParseStateMachine                                                         */
/*!
    Parse the state machine from the state machine definition file

    @param[in]
        filename
            pointer to the NUL terminated name of the state machine
            definition file

    @retval EINVAL invalid arguments
    @retval EOK state machine parsed successfully

==============================================================================*/
static int ParseStateMachine( char *filename )
{
    int result = EINVAL;
    extern FILE *yyin;
    int tok;

    if ( filename != NULL )
    {
        /* open the state machine definition file */
        yyin = fopen( filename, "r" );
        if ( yyin != NULL )
        {
            /* parse the state machine file */
            if ( yyparse() == 0 )
            {
                result = EOK;
            }
        }
    }

    return result;
}

/*============================================================================*/
/*  usage                                                                     */
/*!
    Display the state machine usage

    The usage function dumps the application usage message
    to stderr.

    @param[in]
       cmdname
            pointer to the invoked command name

    @return none

==============================================================================*/
static void usage( char *cmdname )
{
    if( cmdname != NULL )
    {
        fprintf(stderr,
                "usage: %s [-v] [-h] [<filename>]\n"
                " [-h] : display this help\n"
                " [-v] : verbose output\n",
                cmdname );
    }
}

/*============================================================================*/
/*  ProcessOptions                                                            */
/*!
    Process the command line options

    The ProcessOptions function processes the command line options and
    populates the iotsend state object

    @param[in]
        argC
            number of arguments
            (including the command itself)

    @param[in]
        argv
            array of pointers to the command line arguments

    @param[in]
        pState
            pointer to the state machine object

    @return 0

==============================================================================*/
static int ProcessOptions( int argC,
                           char *argV[],
                           StateMachine *pStateMachine )
{
    int c;
    int result = EINVAL;
    const char *options = "hv";

    if( ( pStateMachine != NULL ) &&
        ( argV != NULL ) )
    {
        while( ( c = getopt( argC, argV, options ) ) != -1 )
        {
            switch( c )
            {
                case 'v':
                    pStateMachine->verbose = true;
                    break;

                case 'h':
                    usage( argV[0] );
                    break;

                default:
                    break;

            }
        }

        if ( optind < argC )
        {
            pStateMachine->filename = strdup(argV[optind]);
        }
    }

    return 0;
}

/*============================================================================*/
/*  SetupTerminationHandler                                                   */
/*!
    Set up an abnormal termination handler

    The SetupTerminationHandler function registers a termination handler
    function with the kernel in case of an abnormal termination of this
    process.

==============================================================================*/
static void SetupTerminationHandler( void )
{
    static struct sigaction sigact;

    memset( &sigact, 0, sizeof(sigact) );

    sigact.sa_sigaction = TerminationHandler;
    sigact.sa_flags = SA_SIGINFO;

    sigaction( SIGTERM, &sigact, NULL );
    sigaction( SIGINT, &sigact, NULL );

}

/*============================================================================*/
/*  TerminationHandler                                                        */
/*!
    Abnormal termination handler

    The TerminationHandler function will be invoked in case of an abnormal
    termination of this process.  The termination handler closes
    the connection with the variable server and cleans up its VARFP shared
    memory.

@param[in]
    signum
        The signal which caused the abnormal termination (unused)

@param[in]
    info
        pointer to a siginfo_t object (unused)

@param[in]
    ptr
        signal context information (ucontext_t) (unused)

==============================================================================*/
static void TerminationHandler( int signum, siginfo_t *info, void *ptr )
{
    syslog( LOG_ERR, "Abnormal termination of statemachine\n" );

    if ( pStateMachine != NULL )
    {
        if ( pStateMachine->hVarServer != NULL )
        {
            VARSERVER_Close( pStateMachine->hVarServer );
            pStateMachine->hVarServer = NULL;
        }

        if( pStateMachine->filename != NULL )
        {
            free( pStateMachine->filename );
            pStateMachine->filename = NULL;
        }

        free( pStateMachine );
        pStateMachine = NULL;
    }

    exit( 1 );
}

/*! @}
 * end of statemachine group */
