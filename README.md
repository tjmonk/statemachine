# statemachine
Finite State Machine Engine

## Overview

The statemachine service provides an engine to run a state machine
which interfaces with the VarServer for its transition signals.

## Prerequisites:

The actions scripting engine requires the following components:

- varserver : variable server ( https://github.com/tjmonk/varserver )
- libvaraction : actions library ( https://github.com/tjmonk/libvaraction )

## Build

```
$ ./build.sh &
```

---
## State Machine Language Overview

State Machines are defined using a C-like scripting language.
Each invocation of the state machine engine can load one state machine.

The state machine definition is contained withing a "statemachine" statement.
The statemachine has a name and description attribute, which are both
strings.

Like this:

```
    statemachine {
        name: "My State Machine"
        description: "My first very cool state machine"

        # state machine definition goes in here.
    }
```

### State definition

The statemachine contains one or more state definitions.
Each state is defined with a name, and three subsections:

- entry actions
- transitions
- exit actions

Every state machine MUST have the "init" state so that the state machine
engine knows where to start.

For example, here is a complete state machine definition, with only the init
state and with no logic in its subsections:

```
    # Empty state machine
    statemachine {
        name: "My State Machine"
        description: "My first very cool state machine"

        state init {
            entry {
                # entry actions go here and these are executed when
                # the state machine first transitions to this state
            }

            transition {
                # transition rules go here to conditionally transition
                # to a new state
            }

            exit {
                # exit actions go here and these are executed when
                # the state machine transitions from this state to a new state
            }
        }
    }

```
### Entry/Exit Script

The entry and exit sections of a state definition contain C-like scripting
language snippets which can perform logic such as starting/stopping timers,
performing conditional code execution, and reading/writing to VarServer
variables.

Entry scripts are executed when the state machine engine transitions into
the state.  Exit scripts are executed when the state machine engine transitions
out of the state.

See below for a description and examples of the scripting langauge.

### Transitions

Transitions specify the conditions underwhich the statemachine will transition
to a new state.  A transition block can contain one or more transitions.
Each transition starts with an identifier - the name of the transition target
state, followed by a colon, follwed by an expression.

An expression can be a timer expiry, or any other C-like expression that
involves a VarServer variable.  VarServer variables in transition expressions
are evaluated when they change value.

The following are examples of valid transition statements:

```
transition {
    # transition to state2 when timer 1 expires
    state2: timer 1

    # transition to state3 if /sys/test/a is > 0
    state3: /sys/test/a > 3
}
```

### Comments

Comments can be included in state machine scripts by prefixing
the comment line with a # symbol.

### Local Variables

The action script supports local variable declarations.  Currently the following
variable types are supported:

- string ( array of NUL terminated characters )
- float ( IEEE754 floating point )
- short (16-bits )
- int ( 32-bits )

Local variables can contain intermediate calculations for use in conditionals,
or before they are written out to VarServer variables.

Like C, Local variables may be specified at the top of a statement block:

```
{
    float x;
    int y;
    short z;
    string str;
}
```

Local variables can be assigned from constants, VarServer variables, and
expressions.

For example, the following are all valid:

```
    {
        x = 1.25;
        y = -32;
        z = 0xFF00;
        str = "This is a test";
    }
```

Note that currently signed values are contained in unsigned storage
locations and mapped to unsigned VarServer variables.  This may change
in future as more types are introduced to align with the full set
of VarServer data types.

### Type conversion

Assigning values to variables of different types, or creating expressions
with values of different types requires type conversion.  Like in C,
type conversion is performed by prefixing the source variable with the type of
the target variable in parenthesis.

String conversion is an exception.  When converting to a string, you often
will want to specify an optional string format specifier.

### Expressions

Complex expressions can be created just like in C.
For example,

```
    {
        short mask;

        mask = ( 0x0F << 4 ) | ( 1u << 3 ) | ( 1u << 2 ) | (1u << 1 );
    }
```

### Conditional Execution

Like C, state machine scripts can have conditional execution in the form of
if-then and if-then-else statements.  If conditions can be simple or
compound conditionals just like in C.

For example,

```
    if ( /sys/test/a > 10 && /sys/test/b > 10 ) {
        # do something
    } else {
        # do something else
    }
```

### Timers

Special script commands exists to perform timer management.  Two types of timers
exist:  a one-shot timer which fires once and stops, and a repeating tick
timer which fires on an interval.  Each timer has a number so it can be
referenced after it is created.  Up to 255 timers can exist within a
statemachine.   Timer intervals are specified in milliseconds.
Timer intervals can also be specified via a variable name.

Examples of timer management functions are shown below:

Create a one-shot timer that will fire in 1 second

```
create timer 1 1000
```

Create a repeating tick timer that will fire every 100 ms:

```
create tick 2 100
```

Delete a one-shot timer or tick timer.

```
delete timer 1
```


### Shell Script execution

The state machine engine can execute inline shell scripts that are contained
within three backticks.

Shell script snippets can occur anywhere an executable statement can occur.

An example of a shell script in a state machine might be as follows:

```

    state active {
        entry: {
            # capture the uptime

            ```
            uptime > /tmp/uptime.txt
            ```
        }

        transition: {
            inactive: /sys/test/a == 0
        }

        exit: {
            # clean up the activity log
            ```
            rm /tmp/uptime.txt
            ```
        }
    }
```

## Examples

### Example 1

Example 1 is a simple state machine with three states:  init (mandatory first
state), on, and off.  The state machine toggles between the on and off states
based on a timer.  It uses a single timer with different timeout values
for the on and off states

```
# Simple ON/OFF state machine

statemachine {
    name: "OnOffTest"
    description: "On/Off Test State Machine"

    state init {
        entry {
            create timer 1 2000;
        }

        transition {
            on: timer 1
        }

        exit {
        }
    }

    # in the on state, the /sys/test/a value is one
    state on {
        entry {
            # create a timer that will expire in 2 seconds
            create timer 1 2000;
            /sys/test/a = 1;
        }

        transition {
            # transition to the off state when the timer expires
            off: timer 1
        }

        exit {
            # delete the timer
            delete timer 1;
        }
    }

    # in the off state, the /sys/test/a value is zero
    state off {
        entry {
            # create a timer that will expire in 4 seconds
            create timer 1 4000;
            /sys/test/a = 0;
        }

        transition {
            # transition to the 'on' state when the timer expires
            on: timer 1
        }

        exit {
            # delete the timer
            delete timer 1;
        }
    }
}

```

### Run Example 1

```
$ varserver &

$ mkvar -t uint16 -n /sys/test/a

$ statemachine test/example1.sm &
```

### Example 2

Example 2 is a simple home alarm system.  It has 5 states:
- init (mandatory first state)
- disarmed
- arming
- armed
- alarm.

It uses the following variables:

- /sys/alarm/arm_delay - arming delay in seconds
- /sys/alarm/armed - flag indicating alarm is armed
- /sys/alarm/activate - user control to enable/disable the alarm
- /sys/alarm/siren - flag indicating alarm has gone off
- /sys/alarm/trigger - flag indicating intrusion is detected

```
    # Simple home alarm system

    statemachine {
        name: "Alarm System"
        description: "Simple Home Alarm system"

        state init {
            entry {
                # initialize all variables
                /sys/alarm/armed = 0;
                /sys/alarm/siren = 0;
                /sys/alarm/activate = 0;
                /sys/alarm/trigger = 0;
                create timer 1 100;
            }

            transition {
                disarmed: timer 1
            }

            exit {
                delete timer 1;
            }
        }

        # in the disarmed state any change to the intrusion state
        # will have no effect.
        state disarmed {
            entry {
                /sys/alarm/siren = 0;
                /sys/alarm/armed = 0;
            }

            transition {
                # transition to the off state when the timer expires
                arming: /sys/alarm/activate == 1
            }

            exit {
            }
        }

        # in the arming state we wait for the arming delay or transition
        # back to the disarmed state if the user deactivates the alarm
        state arming {
            entry {
                int armdelay;

                armdelay = /sys/alarm/arm_delay * 1000;
                create timer 1 armdelay;
            }

            transition {
                # transition to the 'armed' state when the timer expires
                armed: timer 1

                # transition back to disarmed if we deactivate the alarm
                # before the arming delay completes
                disarmed: /sys/alarm/activate == 0
            }

            exit {
                # delete the timer
                delete timer 1;
            }
        }

        # in the armed state, if intrusion is detected an alarm will sound
        # deactivation of the alarm will put us back into the disarmed state
        state armed {
            entry {
                /sys/alarm/armed = 1;
            }

            transition {
                # transition to the 'alarm' state if intrusion detected
                alarm: /sys/alarm/trigger == 1

                # transition back to disarmed if we deactivate the alarm
                disarmed: /sys/alarm/activate == 0
            }

            exit {
            }
        }

        # in the alarm state, the siren is activated for 60 seconds
        # the siren can be silenced and the alarm disarmed by deactivating
        # the alarm
        state alarm {
            entry {
                # turn the siren on
                /sys/alarm/siren = 1;

                # create a timer for 60 seconds
                create timer 1 60000;
            }

            transition {
                # go back to armed state when timer expires
                armed: timer 1

                # go to disarmed state if user deactivates the alarm
                disarmed: /sys/alarm/activate == 0
            }

            exit {
                # turn off the siren when we exit this state
                /sys/alarm/siren = 0;

                # delete the timer on exit
                delete timer 1;
            }
        }
    }

```

### Run Example 2

```
$ varserver &

$ mkvar -t uint16 -n /sys/alarm/arm_delay
$ mkvar -t uint16 -n /sys/alarm/armed
$ mkvar -t uint16 -n /sys/alarm/activate
$ mkvar -t uint16 -n /sys/alarm/siren
$ mkvar -t uint16 -n /sys/alarm/trigger
$ setvar /sys/alarm/arm_delay 30

$ statemachine -v test/example2.sm &
```

Set the /sys/alarm/activate and /sys/alarm/trigger variables to trigger the
alarm and exercise the alarm state machine.

## State Machine Language Definition

Refer to the language specification below for a full description of the state
machine declaration language.

```
statemachine : STATEMACHINE  LBRACE name description state_definition_list RBRACE
             ;

name: NAME COLON string
    ;

description :  DESCRIPTION COLON string
            ;

state_definition_list
		: 	state_definition state_definition_list
        ;

state_definition
		: 	STATE id LBRACE state_body RBRACE
		;

state_body : entry transblock exit
           ;

entry : ENTRY LBRACE declaration_list statement_list RBRACE
      ;

transblock : TRANSITION LBRACE transitionlist RBRACE
           ;

transitionlist : transition transitionlist
               ;

transition : id COLON expression
           ;

id : ID
   ;

exit : EXIT LBRACE declaration_list statement_list RBRACE
     ;

statement_list : statement statement_list
        ;

statement : expression SEMICOLON
          | selection_statement
          | script
          | timer_statement SEMICOLON
          | SEMICOLON
          ;

declaration_list : declaration SEMICOLON declaration_list
                 ;

declaration : type_specifier decl_id
            ;

type_specifier : FLOAT { $$ = (void *)VA_FLOAT; }
               | INT { $$ = (void *)VA_INT; }
               | SHORT { $$ = (void *)VA_SHORT; }
               | STRING { $$ = (void *)VA_STRING; }
               ;

selection_statement
		:	IF LPAREN expression RPAREN compound_statement   %prec "then"
		|	IF LPAREN expression RPAREN compound_statement ELSE compound_statement
		;

compound_statement: LBRACE statement_list RBRACE
        ;

timer_statement
        :   CREATE TIMER number number
        |   CREATE TIMER number identifier
        |   CREATE TICK number number
        |   CREATE TICK number identifier
        |   DELETE TIMER number
        ;

expression: assignment_expression
        ;

assignment_expression
        :   timer_expression
        |   identifier assignment_operator assignment_expression
        ;

assignment_operator
        : ASSIGN
        | TIMES_EQUALS
        | DIV_EQUALS
        | PLUS_EQUALS
        | MINUS_EQUALS
        | AND_EQUALS
        | OR_EQUALS
        | XOR_EQUALS
        ;

timer_expression
        : logical_OR_expression
        | TIMER number
        ;

logical_OR_expression
        :   logical_AND_expression
        |   logical_OR_expression OR logical_AND_expression
        ;

logical_AND_expression
        : inclusive_OR_expression
        | logical_AND_expression AND inclusive_OR_expression
        ;

inclusive_OR_expression
        :   exclusive_OR_expression
        |   inclusive_OR_expression BOR exclusive_OR_expression
        ;

exclusive_OR_expression
        : AND_expression
        | exclusive_OR_expression XOR AND_expression
        ;

AND_expression
        :   equality_expression
        |   AND_expression BAND equality_expression
        ;

equality_expression
        :   relational_expression
        |   equality_expression EQUALS relational_expression
        |   equality_expression NOTEQUALS relational_expression
        ;

relational_expression
        :   shift_expression
        |   relational_expression LT shift_expression
        |   relational_expression GT shift_expression
        |   relational_expression LTE shift_expression
        |   relational_expression GTE shift_expression
        ;

shift_expression
        :   additive_expression
        |   shift_expression LSHIFT additive_expression
        |   shift_expression RSHIFT additive_expression
        ;

additive_expression
        :   multiplicative_expression
        |   additive_expression ADD multiplicative_expression
        |   additive_expression SUB multiplicative_expression
        ;

multiplicative_expression
        :   unary_expression
        |   multiplicative_expression MUL unary_expression
        |   multiplicative_expression DIV unary_expression
        ;

unary_expression
        :   typecast_expression
        |   INC identifier
        |   DEC identifier
        |   NOT unary_expression
        ;

typecast_expression
        : postfix_expression
        | float_cast number
        | float_cast identifier
        | int_cast floatnum
        | int_cast identifier
        | short_cast number
        | short_cast floatnum
        | short_cast identifier
        | string_cast identifier
        | LPAREN STRING string RPAREN identifier
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
        |   identifier INC
        |   identifier DEC
        ;

primary_expression
        :   identifier
        |   LPAREN expression RPAREN
        |   floatnum
        |   number
        |   string
        ;

script : SCRIPT
       ;

string : CHARSTR
       ;

number : NUM
    ;

floatnum : FLOATNUM
         ;

identifier : ID
   ;

decl_id : ID
   ;

```
