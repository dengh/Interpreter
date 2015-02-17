%code requires {
#include "structures.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

  extern char* strdup(const char*);
  extern FILE* yyin;
  FILE* debugout;

}

%{
#include <stdio.h>
#include "structures.h"
#include "parser.h"
#define DPRINT(...) if (debugout) {fprintf(debugout,__VA_ARGS__);}

extern FILE* yyin;
FILE* debugout; 

SymbolIdent* it = NULL;
  
void yyerror(const char *str)
{
  //fprintf(stderr,"error: %s\n",str);
}
 
int yywrap()
{
  fclose(yyin); 
  yyrestart(stdin);
  return 0;
} 

SymbolIdent* parse(FILE* inStream, FILE* debugStream) {
  debugout = debugStream;
  if (inStream != NULL)
    yyin = inStream;
  if (!yyparse())
    return it;
  else //Parsing failed for whatever reason
    return NULL;
}

%}

%union {
       char* cval;
       TreeNode* TNval;
       NameListNode* NLNval;
       SymbolIdent* SIval;
       PointerListNode* PLNval;
       ValList* VLval;
       Val Vval;
       intptr_t i;
}
%type <TNval> expression term
%type <SIval> function constant base_expr
%type <Vval> value
%type <VLval> nodes list
%type <cval> argument
%type <NLNval> arguments
%type <PLNval> expressionlist
%type <cval> infix
%token <cval> NAME PLUS MINUS MULT DIV LESSER GREATER PATH
%token <i> NUMBER EQUAL
%token END FUNCTION VALUE LBRACKET RBRACKET LPARENS RPARENS COLON QUIT IF THEN ELSE COMMA FILEPATH
%left PLUS MINUS
%left MULT DIV
%left EQUAL
%%

declaration: function COLON 
	     {
	       it=$1; 
	       DPRINT("Made function\n"); 
	       YYACCEPT;
	     }
	     | constant COLON {
	       it=$1; 
	       DPRINT("Made value\n"); 
	       YYACCEPT;
	     }
	     | base_expr COLON {
	       it=$1; 
	       DPRINT("Made base expression\n"); 
	       YYACCEPT;
	     }
             | QUIT {it=(SymbolIdent*)(intptr_t)5; YYACCEPT;}
	     | FILEPATH PATH COLON
	     {
	       yyin=fopen($2,"r");
	       if (yyin == NULL) {
		 printf("Invalid file name or path\n");
		 yyin = stdin;
	       }
	       YYABORT;	 
	     }
            | error COLON {printf("Syntax error\n"); YYABORT;}
            | COLON {YYABORT;}
            ;

function: FUNCTION NAME LPARENS arguments RPARENS EQUAL expression
	  {
	    SymbolIdent* returnPointer = malloc(sizeof(SymbolIdent));
	    returnPointer->name = strdup($2);
	    returnPointer->argNames = $4;
	    returnPointer->parseTree = $7;
	    $$ = returnPointer;
	  }
	  ;

constant: VALUE NAME EQUAL expression
	  {
	    SymbolIdent* returnPointer = malloc(sizeof(SymbolIdent));
	    returnPointer->name = strdup($2);
	    returnPointer->argNames = NULL;
	    returnPointer->parseTree = $4;
	    $$ = returnPointer;
	  }
	  ;

base_expr: expression
	   {
	    SymbolIdent* returnPointer = malloc(sizeof(SymbolIdent));
	    returnPointer->name = NULL;
	    returnPointer->argNames = NULL;
	    returnPointer->parseTree = $1;
	    $$ = returnPointer;
	   }

arguments:		     {$$ = NULL;}
	 | argument
	 {
	    NameListNode* returnPointer = malloc(sizeof(NameListNode));
	    returnPointer->name = $1;
	    returnPointer->next = NULL;
	    $$ = returnPointer;
	 }
	 | argument COMMA arguments
	 {
	    NameListNode* returnPointer = malloc(sizeof(NameListNode));
	    returnPointer->name = $1;
	    returnPointer->next = $3;
	    $$ = returnPointer;
	 }
	 ;

argument: NAME		{$$ = strdup($1);}
	 ;

expressionlist:		{$$=NULL;}
	      | expression
	      {
		PointerListNode* returnPointer = malloc(sizeof(PointerListNode));
		returnPointer->next = NULL;
		returnPointer->target = $1;
		$$ = returnPointer;
	      }
	      | expression COMMA expressionlist
	      {
		PointerListNode* returnPointer = malloc(sizeof(PointerListNode));
		returnPointer->next = $3;
		returnPointer->target = $1;
		$$ = returnPointer;
	      }		 
	      ;

expression: expression infix term
	    {
		TreeNode* returnPointer = malloc(sizeof(TreeNode));
		PointerListNode* arg1 = malloc(sizeof(PointerListNode));
		PointerListNode* arg2 = malloc(sizeof(PointerListNode));
		arg1->target=$1;
		arg2->target=$3;
		arg1->next=arg2;
		arg2->next=NULL;
		returnPointer->argList = arg1;
		returnPointer->value = 
		createVal(ValueType_FUNCTION, (intptr_t) $2);
		$$ = returnPointer;
		DPRINT("Made expression infix function call to %s\n", $2);
	    }
	  | IF expression THEN expression ELSE expression
	    {
		TreeNode* returnPointer = malloc(sizeof(TreeNode));
		PointerListNode* arg1 = malloc(sizeof(PointerListNode));
		PointerListNode* arg2 = malloc(sizeof(PointerListNode));
		PointerListNode* arg3 = malloc(sizeof(PointerListNode));
		arg1->target=$2;
		arg2->target=$4;
		arg3->target=$6;
		arg1->next=arg2;
		arg2->next=arg3;
		arg3->next=NULL;
		returnPointer->argList = arg1;
		returnPointer->value = 
		createVal(ValueType_FUNCTION, (intptr_t) strdup("ite"));
		$$ = returnPointer;
		DPRINT("Made if-then-else expression\n");
	    }
	  | term {$$ = $1;}




term:	    NAME LPARENS expressionlist RPARENS 
	    {
		TreeNode* returnPointer = malloc(sizeof(TreeNode));
		returnPointer->argList = $3;
		returnPointer->value = 
		createVal(ValueType_FUNCTION, (intptr_t) $1);
		$$ = returnPointer;
		DPRINT("Made expression function call\n");
	    }
	  | NAME
	    {		
	    	TreeNode* returnPointer = malloc(sizeof(TreeNode));
		returnPointer->argList = NULL;
		returnPointer->value = 
		createVal(ValueType_CONSTANT, (intptr_t) $1);
		$$ = returnPointer;
		DPRINT("Made expression symbol reference to %s\n",$1);
	    }
          | value
	    {
		TreeNode* returnPointer = malloc(sizeof(TreeNode));
		returnPointer->argList = NULL;
		returnPointer->value = $1;
		$$ = returnPointer;
		DPRINT("Made expression constant value\n");
	    }
	  | LPARENS expression RPARENS {$$ = $2;}	    	  
	    ;

infix:	       PLUS	{$$ = $1;}
	     | MINUS	{$$ = $1;}
	     | MULT	{$$ = $1;}
	     | DIV	{$$ = $1;}
	     | EQUAL	{$$ = $1;}
             | LESSER   {$$ = $1;}
             | GREATER  {$$ = $1;}

value: list 		{
       			 $$=createVal(ValueType_LIST,(intptr_t) $1);
       			 DPRINT("Made list\n");
			}
     |  MINUS NUMBER 	{
     	      		 $$=createVal(ValueType_INT,-((intptr_t) $2));
			 DPRINT("Made negative number\n");
			}
     |  NUMBER 		{
			 $$=createVal(ValueType_INT,(intptr_t) $1);
			 DPRINT("Made number\n");
			}
     ;

list: LBRACKET nodes RBRACKET {$$=$2;}
    ;


nodes:	       	     	{$$=NULL;}
     | value		
     {
       	ValList* returnVal = malloc(sizeof(ValList));
	returnVal->value=$1;
	returnVal->next=NULL;
	 $$=returnVal;
     }
     | value COMMA nodes
     {
	ValList* returnVal = malloc(sizeof(ValList));
	returnVal->value=$1;
	returnVal->next=$3;
	$$=returnVal;
     }
     ;
