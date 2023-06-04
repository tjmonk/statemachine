%{
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
#include <varserver/varserver.h>
#include "SMTypes.h"
#include "lineno.h"

/*==============================================================================
       Definitions
==============================================================================*/

#define YYSTYPE void *

extern char *yytext;

/* input file for lex */
extern FILE *yyin;

/* output file */
static FILE *fp;

/* function declarations */

void yyerror();

extern StateMachine *pStateMachine;

int yylex(void);

/* error flag */
static bool errorFlag = false;

/*==============================================================================
       Function declarations
==============================================================================*/

static int RequestNotifications( VARSERVER_HANDLE hVarServer,
                                 Variable *pVariable );
VAR_HANDLE RequestNotification(VARSERVER_HANDLE hVarServer, char *varName );

%}

%token STATEMACHINE
%token NAME
%token DESCRIPTION
%token STATE
%token CREATE
%token TIMER
%token TICK
%token DELETE
%token SH
%token AND
%token OR
%token NOT
%token ENTRY
%token EXIT
%token TRANSITION
%token COLON
%token COMMA
%token SEMICOLON
%token ASSIGN
%token EQUALS
%token NOTEQUALS
%token GT
%token LT
%token GTE
%token LTE
%token LPAREN
%token RPAREN
%token LBRACE
%token RBRACE
%token LBRACKET
%token RBRACKET
%token ADD
%token SUB
%token MUL
%token DIV
%token ID
%token DQUOTE
%token CHARSTR
%token NUM
%token MLS
%token SCRIPT
%token AND_EQUALS
%token DIV_EQUALS
%token TIMES_EQUALS
%token PLUS_EQUALS
%token MINUS_EQUALS
%token OR_EQUALS
%token XOR_EQUALS
%token BAND
%token BOR
%token XOR
%token LSHIFT
%token RSHIFT
%token INC
%token DEC
%token IF
%token ELSE
%token FLOAT
%token INT
%token SHORT
%token STRING
%token FLOATNUM

%%
statemachine : STATEMACHINE  LBRACE name description state_definition_list RBRACE
			 {
                if ( pStateMachine != NULL )
                {
                    pStateMachine->name = $3;
                    pStateMachine->description = $4;
                    pStateMachine->pStateList = $5;
                }
                $$ = pStateMachine;
             }
             ;

name: NAME COLON string
    {
        $$ = $3;
    }
    ;

description :  DESCRIPTION COLON string
            {
                $$ = $3;
            }
            ;

state_definition_list
		: 	state_definition state_definition_list
			{
                State *pState = $1;
                if ( pState != NULL )
                {
                    pState->pNext = $2;
                }
                $$ = pState;
			}

        | 	{ $$ = NULL; }
        ;

state_definition
		: 	STATE id LBRACE state_body RBRACE
			{
                State *pState = (State *)$4;
                if ( pState != NULL )
                {
                    pState->id = $2;
                }
                $$ = $4;
            }
		;

state_body : entry transblock exit
           {
                State *pState = calloc( 1, sizeof( State ) );
                if ( pState != NULL )
                {
                    pState->entry = $1;
                    pState->trans = $2;
                    pState->exit = $3;
                }
                $$ = pState;
           }
           ;

entry : ENTRY LBRACE declaration_list statement_list RBRACE
      {
          StateEntry *entry = calloc( 1, sizeof( StateEntry ) );
          if ( entry != NULL )
          {
              entry->pDeclarations = $3;
              entry->pStatements = $4;
          }
          $$ = entry;
      }
      ;

transblock : TRANSITION LBRACE transitionlist RBRACE
           {
                $$ = $3;
           }
           ;

transitionlist : transition transitionlist
               {
                    Transition *trans = (Transition *)$1;
                    if ( trans != NULL )
                    {
                        trans->pNext = $2;
                    }
                    $$ = $1;
               }
               | { $$ = NULL; }
               ;

transition : id COLON expression
           {
                Transition *trans = calloc( 1, sizeof( Transition ) );
                if ( trans != NULL )
                {
                    trans->statename = $1;
                    trans->pVariable = $3;
                    RequestNotifications( pStateMachine->hVarServer, trans->pVariable );
                }
                $$ = trans;
           }

id : ID
   {
       $$ = strdup( yytext );
   }
   ;

exit : EXIT LBRACE declaration_list statement_list RBRACE
     {
        StateExit *exit = calloc( 1, sizeof( StateExit ) );
        if ( exit != NULL )
        {
              exit->pDeclarations = $3;
              exit->pStatements = $4;
        }
        $$ = exit;
     }
     ;

statement_list : statement statement_list
        {
            Statement *pStatement = (Statement *)$1;
            if ( pStatement != NULL )
            {
                pStatement->pNext = $2;
            }
            $$ = $1;
        }
        | { $$ = NULL; }
        ;

statement : expression SEMICOLON
            {
               Statement *pStatement = (Statement *)calloc( 1, sizeof( Statement ) );
               if ( pStatement != NULL )
               {
                   pStatement->pVariable = $1;
               }

               $$ = (void *)pStatement;
            }
          | selection_statement
            {
               Statement *pStatement = (Statement *)calloc( 1, sizeof( Statement ) );
               if ( pStatement != NULL )
               {
                   pStatement->pVariable = $1;
               }

               $$ = (void *)pStatement;
            }
          | script
            {
               Statement *pStatement = (Statement *)calloc( 1, sizeof( Statement ) );
               if ( pStatement != NULL )
               {
                   pStatement->script = (char *)$1;
               }

               $$ = (void *)pStatement;
            }
          | timer_statement SEMICOLON
            {
               Statement *pStatement = (Statement *)calloc( 1, sizeof( Statement ) );
               if ( pStatement != NULL )
               {
                   pStatement->pVariable = $1;
               }

               $$ = (void *)pStatement;
            }
          | SEMICOLON
            {
            }
          ;

declaration_list : declaration SEMICOLON declaration_list
                   {
                        Variable *pVariable = (Variable *)$1;
                        if ( pVariable != NULL )
                        {
                            pVariable->pNext = $3;
                        }

                        $$ = $1;

                        /* set up the local variable declarations */
                        SetDeclarations( pVariable );
                    }
                 | { $$ = NULL; }

declaration : type_specifier decl_id
            {
                $$ = CreateDeclaration( (uintptr_t)$1, $2 );
            }
            ;

type_specifier : FLOAT { $$ = (void *)VA_FLOAT; }
               | INT { $$ = (void *)VA_INT; }
               | SHORT { $$ = (void *)VA_SHORT; }
               | STRING { $$ = (void *)VA_STRING; }
               ;

selection_statement
		:	IF LPAREN expression RPAREN compound_statement   %prec "then"
			{ $$ = CreateVariable(VA_IF,$3, CreateVariable(VA_ELSE,$5,NULL)); }

		|	IF LPAREN expression RPAREN compound_statement ELSE compound_statement
			{ $$ = CreateVariable(VA_IF,$3, CreateVariable(VA_ELSE,$5,$7)); }
		;

compound_statement: LBRACE statement_list RBRACE
            { $$ = $2; }
        ;

timer_statement
        :   CREATE TIMER number number
            { $$ = CreateVariable(VA_CREATE_TIMER, $3, $4); }
        |   CREATE TIMER number identifier
            { $$ = CreateVariable(VA_CREATE_TIMER, $3, $4); }
        |   CREATE TICK number number
            { $$ = CreateVariable(VA_CREATE_TICK, $3, $4); }
        |   CREATE TICK number identifier
            { $$ = CreateVariable(VA_CREATE_TIMER, $3, $4); }
        |   DELETE TIMER number
            { $$ = CreateVariable(VA_DELETE_TIMER, $3, NULL); }
        ;

expression: assignment_expression
        {
            $$ = $1;
        }
        ;

assignment_expression
        :   timer_expression
        {
            $$ = $1;
        }
        |   identifier assignment_operator assignment_expression
        {
            Variable *pVariable = (Variable *)$1;
            if ( pVariable != NULL )
            {
                pVariable->lvalue = true;
                pVariable->assigned = true;
            }

            $$ = CreateVariable( (uintptr_t)$2, $1, $3 );
        }
        ;

assignment_operator
        : ASSIGN
        { $$ = (void *)VA_ASSIGN; }
        | TIMES_EQUALS
        { $$ = (void *)VA_TIMES_EQUALS; }
        | DIV_EQUALS
        { $$ = (void *)VA_DIV_EQUALS; }
        | PLUS_EQUALS
        { $$ = (void *)VA_PLUS_EQUALS; }
        | MINUS_EQUALS
        { $$ = (void *)VA_MINUS_EQUALS; }
        | AND_EQUALS
        { $$ = (void *)VA_AND_EQUALS; }
        | OR_EQUALS
        { $$ = (void *)VA_OR_EQUALS; }
        | XOR_EQUALS
        { $$ = (void *)VA_XOR_EQUALS; }
        ;

timer_expression
        : logical_OR_expression
          { $$ = $1; }
        | TIMER number
          {
            if ( $2 != NULL )
            {
                Variable *pVariable = $2;
                pVariable->operation = VA_TIMER;
            }

            $$ = CreateVariable( VA_EQUALS, $2,
                    CreateVariable( VA_ACTIVE_TIMER, NULL, NULL ));
          }
        ;

logical_OR_expression
        :   logical_AND_expression
        {
            $$ = $1;
        }
        |   logical_OR_expression OR logical_AND_expression
        {
            $$ = CreateVariable( VA_OR, $1, $3 );
        }
        ;

logical_AND_expression
        : inclusive_OR_expression
        {
            $$ = $1;
        }
        | logical_AND_expression AND inclusive_OR_expression
        {
            $$ = CreateVariable( VA_AND, $1, $3 );
        }
        ;

inclusive_OR_expression
        :   exclusive_OR_expression
        {
            $$ = $1;
        }
        |   inclusive_OR_expression BOR exclusive_OR_expression
        {
            $$ = CreateVariable( VA_BOR, $1, $3 );
        }
        ;

exclusive_OR_expression
        : AND_expression
        {
            $$ = $1;
        }
        | exclusive_OR_expression XOR AND_expression
        {
            $$ = CreateVariable( VA_XOR, $1, $3 );
        }
        ;

AND_expression
        :   equality_expression
        {
            $$ = $1;
        }
        |   AND_expression BAND equality_expression
        {
            $$ = CreateVariable( VA_BAND, $1, $3 );
        }
        ;

equality_expression
        :   relational_expression
        {
            $$ = $1;
        }
        |   equality_expression EQUALS relational_expression
        {
            $$ = CreateVariable( VA_EQUALS, $1, $3 );
        }
        |   equality_expression NOTEQUALS relational_expression
        {
            $$ = CreateVariable( VA_NOTEQUALS, $1, $3 );
        }
        ;

relational_expression
        :   shift_expression
        {
            $$ = $1;
        }
        |   relational_expression LT shift_expression
        {
            $$ = CreateVariable( VA_LT, $1, $3 );
        }
        |   relational_expression GT shift_expression
        {
            $$ = CreateVariable( VA_GT, $1, $3 );
        }
        |   relational_expression LTE shift_expression
        {
            $$ = CreateVariable( VA_LTE, $1, $3 );
        }
        |   relational_expression GTE shift_expression
        {
            $$ = CreateVariable( VA_GTE, $1, $3 );
        }
        ;

shift_expression
        :   additive_expression
        {
            $$ = $1;
        }
        |   shift_expression LSHIFT additive_expression
        {
            $$ = CreateVariable( VA_LSHIFT, $1, $3 );
        }
        |   shift_expression RSHIFT additive_expression
        {
            $$ = CreateVariable( VA_RSHIFT, $1, $3 );
        }
        ;

additive_expression
        :   multiplicative_expression
        {
            $$ = $1;
        }
        |   additive_expression ADD multiplicative_expression
        {
            $$ = CreateVariable( VA_ADD, $1, $3 );
        }
        |   additive_expression SUB multiplicative_expression
        {
            $$ = CreateVariable( VA_SUB, $1, $3 );
        }
        ;

multiplicative_expression
        :   unary_expression
        {
            $$ = $1;
        }
        |   multiplicative_expression MUL unary_expression
        {
            $$ = CreateVariable( VA_MUL, $1, $3 );
        }
        |   multiplicative_expression DIV unary_expression
        {
            $$ = CreateVariable( VA_DIV, $1, $3 );
        }
        ;

unary_expression
        :   typecast_expression
        {
            $$ = $1;
        }
        |   INC identifier
        {
            CheckUseBeforeAssign($2);
            $$ = CreateVariable( VA_INC, NULL, $2 );
        }
        |   DEC identifier
        {
            CheckUseBeforeAssign($2);
            $$ = CreateVariable( VA_DEC, NULL, $2 );
        }
        |   NOT unary_expression
        {
            $$ = CreateVariable( VA_NOT, $2, NULL );
        }
        ;

typecast_expression
        : postfix_expression
        {
            $$ = $1;
        }
        | float_cast number
        {
            $$ = CreateVariable(VA_TOFLOAT, $2, NULL );
        }
        | float_cast identifier
        {
            CheckUseBeforeAssign($2);
            $$ = CreateVariable( VA_TOFLOAT, $2, NULL );
        }
        | int_cast floatnum
        {
            $$ = CreateVariable( VA_TOINT, $2, NULL );
        }
        | int_cast identifier
        {
            CheckUseBeforeAssign($2);
            $$ = CreateVariable( VA_TOINT, $2, NULL );
        }
        | short_cast number
        {
            CheckUseBeforeAssign($2);
            $$ = CreateVariable( VA_TOSHORT, $2, NULL );
        }
        | short_cast floatnum
        {
            CheckUseBeforeAssign($2);
            $$ = CreateVariable( VA_TOSHORT, $2, NULL );
        }
        | short_cast identifier
        {
            CheckUseBeforeAssign($2);
            $$ = CreateVariable( VA_TOSHORT, $2, NULL );
        }
        | string_cast identifier
        {
            CheckUseBeforeAssign($2);
            $$ = CreateVariable( VA_TOSTRING, $2, NULL );
        }
        | LPAREN STRING string RPAREN identifier
        {
            CheckUseBeforeAssign($5);
            $$ = CreateVariable( VA_TOSTRING, $5, $3 );
        }
        ;

float_cast:	LPAREN FLOAT RPAREN
		;

int_cast : LPAREN INT RPAREN
		;

short_cast : LPAREN SHORT RPAREN
           ;

string_cast : LPAREN STRING RPAREN
            ;

postfix_expression
        :   primary_expression
        {
            $$ = $1;
        }
        |   identifier INC
        {
            CheckUseBeforeAssign($1);
            $$ = CreateVariable( VA_INC, $1, NULL );
        }
        |   identifier DEC
        {
            CheckUseBeforeAssign($1);
            $$ = CreateVariable( VA_DEC, $1, NULL );
        }
        ;

primary_expression
        :   identifier
            {
                CheckUseBeforeAssign($1);
                $$ = $1;
            }
        |   LPAREN expression RPAREN
            {
                $$ = $2;
            }
        |   floatnum
            {
                $$ = $1;
            }
        |   number
            {
                $$ = $1;
            }
        |   string
            {
                $$ = $1;
            }
        ;

script : SCRIPT
       {
          $$ = strdup( yytext );
       }
       ;

string : CHARSTR
        {
            $$ = NewString( yytext );
        }
       ;

number : NUM
    {
       $$ = NewNumber( yytext );
    }
    ;

floatnum : FLOATNUM
         {
            $$ = NewFloat( yytext );
         }
         ;

identifier : ID
   {
        Variable *pVariable;

        pVariable = NewIdentifier( pStateMachine->hVarServer, yytext, false );
        if( pVariable != NULL )
        {
            pVariable->lineno = getlineno();
        }

        $$ = pVariable;
   }
   ;

decl_id : ID
   {
        Variable *pVariable;

        pVariable = NewIdentifier( pStateMachine->hVarServer, yytext, true );
        if ( pVariable != NULL )
        {
            pVariable->lineno = getlineno();
        }

        $$ = pVariable;
   }
   ;

%%
#include <stdio.h>

/*============================================================================*/
/*  yyerror                                                                   */
/*!
    Handle parser error

	This procedure displays an error message with the line number on
	which the error occurred.

==============================================================================*/
void yyerror()
{
    printf("syntax error at line %d\n",getlineno() + 1);
    errorFlag = true;
}

/*============================================================================*/
/*  RequestNotifications                                                      */
/*!
    Request change notifications for all variables in an expression

    The RequestNotifications function recursively requests notifications
    for all system variables in an expression.

@param[in]
    hVarServer
        handle to the variable server

@param[in]
    pVariable
        pointer to a variable node at the head of an expression

@retval EOK no errors
@retval other one or more notification requests failed

==============================================================================*/
static int RequestNotifications( VARSERVER_HANDLE hVarServer,
                                 Variable *pVariable )
{
    int rcl;
    int rcr;
    int rc;
    int result = EOK;

    if ( pVariable != NULL )
    {
        rcl = RequestNotifications( hVarServer, pVariable->left );
        if ( rcl != EOK )
        {
            result = rcl;
        }

        rcr = RequestNotifications( hVarServer, pVariable->right );
        if ( rcr != EOK )
        {
            result = rcr;
        }

        if ( ( pVariable->operation == VA_SYSVAR ) &&
             ( pVariable->hVar != VAR_INVALID ) )
        {
            rc = VAR_Notify( hVarServer, pVariable->hVar, NOTIFY_MODIFIED );
            if ( rc != EOK )
            {
                syslog( LOG_ERR,
                        "Can't request change notification for %s on line %d\n",
                        pVariable->id ? pVariable->id : "unknown",
                        pVariable->lineno );
                result = rc;
            }
        }
    }

    return result;
}

/*============================================================================*/
/*  RequestNotification                                                       */
/*!
    Request a change notification for a variable

    The RequestNotification function requests a change notification
    for a variable given its name.  If the variable was found and
    the notification successfully registered, a handle to the variable
    is returned.

@param[in]
    hVarServer
        handle to the variable server

@param[in]
    varName
        pointer to a NUL terminated variable name

@retval handle to the variable
@retval VAR_INVALID if the notification request failed

==============================================================================*/
VAR_HANDLE RequestNotification(VARSERVER_HANDLE hVarServer, char *varName )
{
    VAR_HANDLE hVar;
    int result;

    hVar = VAR_FindByName( hVarServer, varName );
    if ( hVar != VAR_INVALID )
    {
        /* request change notification for variable */
        result = VAR_Notify( hVarServer, hVar, NOTIFY_MODIFIED );
        if ( result != EOK )
        {
            syslog( LOG_ERR,
                    "Cannot request change notification for %s\n",
                    varName );

            hVar = VAR_INVALID;
        }
    }
    else
    {
        syslog( LOG_ERR,
                "Cannot get handle for %s\n",
                varName );
    }

    return hVar;
}

