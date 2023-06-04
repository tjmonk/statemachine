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
       Definitions
==============================================================================*/

#define YY_NO_INPUT

/*==============================================================================
        Includes
==============================================================================*/

#include "statemachine.tab.h"
#include "lineno.h"

%}

%x script
%x string

letter [a-zA-Z\_/]
digit [0-9]
nzdigit [1-9]

nl [\n]
delim [ \t]
ws {delim}+

statemachine "statemachine"
name "name"
description "description"
state "state"
create "create"
timer "timer"
tick "tick"
delete "delete"
sh "sh"
entry "entry"
exit "exit"
transition  "transition"

float "float"
int "int"
short "short"
string "string"

if "if"
else "else"
and "&&"
or "||"
not "!"
band "&"
bor "|"
xor "^"
lshift "<<"
rshift ">>"
inc "++"
dec "--"
comma ","

colon ":"
semicolon ";"
assign "="
equals "=="
notequals "!="
andequals "&="
orequals "|="
xorequals "^="
div_equals "/="
times_equals "*="
plus_equals "+="
minus_equals "-="
gt ">"
lt "<"
gte ">="
lte "<="
lparen "("
rparen ")"
lbrace "{"
rbrace "}"
lbracket "["
rbracket "]"
add "+"
sub "-"
mul "*"
div "/"
hash "#"
dquote "\""

backtick "`"
backticks {backtick}{backtick}{backtick}
script ([^```]+)

charstr [^\"]+
comment {hash}(.*)$
id {letter}({letter}|{digit})*
intnum [-]?({digit}|({nzdigit}{digit}*))[lLuU]*
hexnum 0[xX][0-9a-fA-F]+[uUlL]*
num {intnum}|{hexnum}
floatnum  [-+]?[0-9]*\.[0-9]+([eE][-+]?[0-9]+)?

%%
{ws} {/* No action for white space */}
{comment} { /* No action for comments */};
{nl} incrementLineNumber();
{statemachine} return(STATEMACHINE);
{name} return(NAME);
{description} return(DESCRIPTION);
{state} return(STATE);
{create} return(CREATE);
{timer} return(TIMER);
{tick} return(TICK);
{delete} return(DELETE);
{sh} return(SH);
{entry} return(ENTRY);
{exit} return(EXIT);
{transition} return(TRANSITION);

{if} return(IF);
{else} return(ELSE);
{and} return(AND);
{or} return(OR);
{not} return(NOT);
{inc} return(INC);
{dec} return(DEC);
{lshift} return(LSHIFT);
{rshift} return(RSHIFT);
{bor} return(BOR);
{band} return(BAND);
{xor} return(XOR);

{colon} return(COLON);
{comma} return(COMMA);
{semicolon} return(SEMICOLON);
{assign} return(ASSIGN);
{equals} return(EQUALS);
{notequals} return(NOTEQUALS);
{andequals} return(AND_EQUALS);
{orequals} return(OR_EQUALS);
{xorequals} return(XOR_EQUALS);
{div_equals} return(DIV_EQUALS);
{times_equals} return(TIMES_EQUALS);
{plus_equals} return(PLUS_EQUALS);
{minus_equals} return(MINUS_EQUALS);
{gt} return(GT);
{lt} return(LT);
{gte} return(GTE);
{lte} return(LTE);
{lparen} return(LPAREN);
{rparen} return(RPAREN);
{lbrace} return(LBRACE);
{rbrace} return(RBRACE);
{lbracket} return(LBRACKET);
{rbracket} return(RBRACKET);
{add} return(ADD);
{sub} return(SUB);
{mul} return(MUL);
{div} return(DIV);

{dquote} BEGIN(string);
<string>{
{charstr} return(CHARSTR);
{dquote} BEGIN(INITIAL);
}

{backticks} BEGIN(script);
<script>{
{script}  return(SCRIPT);
{backticks} BEGIN(INITIAL);
}

{float} return(FLOAT);
{int} return(INT);
{short} return(SHORT);
{string} return(STRING);

{floatnum} return(FLOATNUM);
{num} return(NUM);
{id} return (ID);

%%

int yywrap()
{
    return 1;
}
