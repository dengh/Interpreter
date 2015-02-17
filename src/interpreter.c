/**
 * @brief: This is the file containing the main execution function, and the associated assisting functions to run the interpreter loop
 * @file: interpreter.c
 * @author: Jonatan Waern, Daniel Engh, Adam Olevall, Mikael Holmberg
 * @date: 27/6 2013
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "structures.h"
#include "parser.h"
#include "hashmap.h"
#include <pthread.h>
#include <time.h>

#define DPRINT(...) if (debug) {fprintf(debug,__VA_ARGS__);}

int MAX_THREADS = 10;/** Maximum number of threads */
int NUM_THREADS = 0; /** Current number of threads */

char* DEF_FUN[] = {"plus","minus","mult", "divide", "equals", "greater", "lesser", "hd", "tl", "cons", "length", "time"}; /** These are the names of all the built-in functions, the array is used to make sure no redefinitions occur */
int DEF_NUM = 12; /** The number of built-in functions (usefull for iteration)*/

map_t symbolmap; /** This hashmap stores all user-defined functions and symbols*/
FILE* debug = NULL; /** The output file for debug information */

/**
 * This defines a tuple of thread id's and values.
 * Defines a tuple of thread id's and values, used to store returnvalues from threads
 */
typedef struct {
  pthread_t id; /** Id of thread */
  Val value; /** Returnvalue of thread */
} ThreadTuple;

/**
 * This defines a tuple of values and strings.
 * Defines a tuple of char* and values, used to bind certain identifiers to certain values
 */
typedef struct {
  Val value; /** The Value */
  char* ident; /** The identifier */
} ArgName;

/**
 * This defines a tuple of various types to be used when passing arguments to thread creation.
 * Defines a tuple of TreeNode*, ArgName*, int and Val*. Used as a struct to pass through a void*
 */
typedef struct {
  TreeNode* target; /** The ParseTree that the new thread will execute */
  ArgName* args; /** An array of ArgName structs that defines the stack bindings for the new walk */
  int num; /** The size of args */
  Val* returnVal; /** A pointer of where to write the result of the walk */
} ForkArgs;

int checkFork(ForkArgs*);
pthread_t doFork(ForkArgs*);

/**
 * Prints a value to stdout.
 * Prints a value to stdout, lists are printed as [node1,node2...nodeN], where a node can recursively be another list
 */
void valPrint(Val curr) {
  switch (getType(curr)) {
  case ValueType_INT:
    printf("%ld",getIntVal(curr));
    break;
  case ValueType_LIST:
    printf("[");
    ValList* tempNode = getListVal(curr);
    while (tempNode) {
      valPrint(tempNode->value);
      if (tempNode->next)
	printf(",");
      tempNode = tempNode->next;
    }
    printf("]");
    break;
  case ValueType_CONSTANT:
    printf("NI");
    break;
  case ValueType_FUNCTION:
    printf("NI");
    break;
  }
}

/**
 * Prints a value to the debugstream, if any.
 * Prints a value to the debugstream, lists are printed as [node1,node2...nodeN], where a node can recursively be another list. If the debugstream is not defined, doesn not print
 */
void dValPrint(Val curr) {
  if (debug) {
    switch (getType(curr)) {
    case ValueType_INT:
      fprintf(debug,"%ld",getIntVal(curr));
      break;
    case ValueType_LIST:
      fprintf(debug,"[");
      ValList* tempNode = getListVal(curr);
      while (tempNode) {
	dValPrint(tempNode->value);
	if (tempNode->next)
	  fprintf(debug,",");
	tempNode = tempNode->next;
      }
      fprintf(debug,"]");
      break;
    case ValueType_CONSTANT:
      fprintf(debug,"NI");
      break;
    case ValueType_FUNCTION:
      fprintf(debug,"NI");
      break;
    }
  }
}

/**
 * Evaluates a addition operation between two vals
 * @return: a val with value equal to the sum of the arguments
 */
Val evalPlus(Val arg1, Val arg2) {
  DPRINT("%ld: executing a plus operation\n", pthread_self());
  return createVal(ValueType_INT, getIntVal(arg1)+getIntVal(arg2));
}

/**
 * Evaluates a subtraction operation between two vals
 * @return: a val with value equal to the first argument minus the second
 */
Val evalMinus(Val arg1, Val arg2) {
  DPRINT("%ld: executing a minus operation\n", pthread_self());
  return createVal(ValueType_INT, getIntVal(arg1)-getIntVal(arg2));
}

/**
 * Evaluates a division operation between two vals
 * @return: a val with value equal to the first argument divided by the second
 */
Val evalDiv(Val arg1, Val arg2) {
  DPRINT("%ld: executing a division operation\n", pthread_self());
  return createVal(ValueType_INT, getIntVal(arg1)/getIntVal(arg2));
}

/**
 * Evaluates a multiplication operation between two vals
 * @return: a val with value equal to the product of the arguments
 */
Val evalMult(Val arg1, Val arg2) {
  DPRINT("%ld: executing a multiplication operation\n", pthread_self());
  return createVal(ValueType_INT, getIntVal(arg1)*getIntVal(arg2));
}

/**
 * Evaluates an equality operation between two vals
 * @return: a val with value 0 if the two vals are not equal, and with value 1 otherwise
 */
Val evalEqual(Val arg1, Val arg2) {
  DPRINT("%ld: executing a equality operation\n", pthread_self());
  if(getType(arg1) == ValueType_INT && getType(arg2) == ValueType_INT){
    return createVal(ValueType_INT, getCharVal(arg1) == getCharVal(arg2));
  }
  else if(getType(arg1) == ValueType_LIST && getType(arg2) == ValueType_LIST){
    return createVal(ValueType_INT, getListsEqual(arg1, arg2));
  }
  else{
    return createVal(ValueType_INT, 0);
  }
}

/**
 * Evaluates a header operation on a list
 * @return: the value of the first node in the list
 */
Val evalHead(Val arg) {
  DPRINT("%ld: executing a header operation\n", pthread_self());
  return getListVal(arg)->value;
}

/**
 * Evaluates a tail operation on a list
 * @return: a new value pointing to the second node of the list
 */
Val evalTail(Val arg) {
  DPRINT("%ld: executing a tail operation\n", pthread_self());
  return createVal(ValueType_LIST, (intptr_t) getListVal(arg)->next);
}

/**
 * Evaluates a length operation on a list
 * @return: the length of the list
 */
Val evalLength(Val arg) {
  DPRINT("%ld: executing a length operation\n", pthread_self());
  return createVal(ValueType_INT, getListLength(arg));
}

/**
 * Builds a listnode using a value and a list, the new node has the value of the argument value and has the first node of the argument list as its tail
 * @return: a new value pointing to the newly constructed node
 */
Val evalCons(Val arg1, Val arg2) {
  DPRINT("%ld: executing a consbox operation\n", pthread_self());
  ValList* newNode = malloc(sizeof(ValList));
  newNode->value = arg1;
  newNode->next = getListVal(arg2);
  return createVal(ValueType_LIST, (intptr_t) newNode);
}

/**
 * Evaluates wether a value is lesser than another, for ints this is a standard comparison Arg1<Arg2, for lists this compares the lengths of the lists.
 * @return: a new value with value 1 if the first argument is lesser than the second, 0 otherwise. If the arguments are of different types, a new value with value 0 is returned.
 */
Val evalLesser(Val arg1, Val arg2) {
  DPRINT("%ld: executing a less-than operation\n", pthread_self());
  if(getType(arg1) == ValueType_INT && getType(arg2) == ValueType_INT){
    return createVal(ValueType_INT, (getIntVal(arg1) < getIntVal(arg2)));
  }
  else if(getType(arg1) == ValueType_LIST && getType(arg2) == ValueType_LIST){
    return createVal(ValueType_INT, (getListLength(arg1) < getListLength(arg2)));
  }
  else{
    return createVal(ValueType_INT, 0);
  }
}

/**
 * Examines wether a string is equal to the identifier of one of the pre-defined functions
 * @return: 1 if the string is one of the pre-defined ones, 0 otherwise
 */
int exists(const char* str) {
  for (int i = 0; i < DEF_NUM; i++) {
    if (!strcmp(str,DEF_FUN[i]))
      return 1;
  }
  return 0;
}

/**
 * Recursively evaluates a parse tree
 * @param: The tree to be evaluated, an array of the local symbol bindings, and the number of local symbol bindings
 * @return: The values that the tree evaluates to
 */
Val eval(TreeNode* curr, ArgName args[], int argNum) {
  DPRINT("%ld: evaluating a node\n",pthread_self());
  switch (getType(curr->value)) {
  case ValueType_CONSTANT:
    for (int k=0; k < argNum; k++) {
      if (!strcmp(getCharVal(curr->value),args[k].ident)) {
	DPRINT("%ld: evaluated %s from arguments\n", pthread_self(), getCharVal(curr->value));
	return args[k].value;
      }
    }
  case ValueType_FUNCTION:
    if (!strcmp(getCharVal(curr->value),"ite")) {
      DPRINT("%ld: evaluated a if-then-else case\n", pthread_self());
      Val branchBool = eval(getArgNode(curr,0), args, argNum);
      if (branchBool.value.intval)
	return eval(getArgNode(curr,1),args,argNum);
      else
	return eval(getArgNode(curr,2),args,argNum);
    } else if (!strcmp(getCharVal(curr->value),"time")) {
      DPRINT("%ld: executing a timing operation", pthread_self());
      struct timespec tstart={0,0}, tend={0,0};
      clock_gettime(CLOCK_MONOTONIC, &tstart);
      eval(getArgNode(curr,0), args, argNum);      
      clock_gettime(CLOCK_MONOTONIC, &tend);
      return createVal(ValueType_INT, (intptr_t) (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec)-((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec)));
    } else { //Execute arguments
      DPRINT("%ld: executing arguments (if any)\n", pthread_self());
      int i = 0;
      PointerListNode* temp = curr->argList;
      while (temp) {temp = temp->next; i++;}
      ThreadTuple* argList = malloc(sizeof(ThreadTuple)*i);
      ForkArgs forkArgs[i];
      temp = curr->argList;
      int ignore = 1;
      for (int j = 0; j < i; j++) {
	forkArgs[j].target = temp->target;
	forkArgs[j].args = args;
	forkArgs[j].num = argNum;
	forkArgs[j].returnVal = &(argList[j].value);
	if (checkFork(&(forkArgs[j]))) {
	  if (ignore) {
	    ignore = 0;
	    argList[j].id = 0;
	  }
	  else
	    argList[j].id = doFork(&(forkArgs[j]));
	}
	else
	  argList[j].id = 0;
	temp = temp->next;
      }
      temp = curr->argList;
      for (int j = 0; j < i; j++) {
	if (!argList[j].id) {
	  argList[j].value = eval(temp->target,args,argNum);
	}
	temp = temp->next;
      }
      void* bogus;
      for (int j = 0; j < i; j++) {
	if (argList[j].id) {
	  pthread_join(argList[j].id, &bogus);
	}
      }
      
      if (!strcmp(getCharVal(curr->value),"plus")) {
	free(argList);
	return evalPlus(argList[0].value,argList[1].value);
      } else if (!strcmp(getCharVal(curr->value),"minus")) {
	free(argList);
	return evalMinus(argList[0].value,argList[1].value);
      } else if (!strcmp(getCharVal(curr->value),"mult")) {
	free(argList);
	return evalMult(argList[0].value,argList[1].value);
      } else if (!strcmp(getCharVal(curr->value),"divide")) {
	free(argList);
	return evalDiv(argList[0].value,argList[1].value);
      } else if (!strcmp(getCharVal(curr->value),"equals")) {
	free(argList);
	return evalEqual(argList[0].value,argList[1].value);
      } else if (!strcmp(getCharVal(curr->value),"hd")) {
	free(argList);
	return evalHead(argList[0].value);
      } else if (!strcmp(getCharVal(curr->value),"tl")) {
	free(argList);
	return evalTail(argList[0].value);
      } else if (!strcmp(getCharVal(curr->value),"length")) {
	free(argList);
	return evalLength(argList[0].value);
      } else if (!strcmp(getCharVal(curr->value),"cons")) {
	free(argList);
	return evalCons(argList[0].value,argList[1].value);
      } else if (!strcmp(getCharVal(curr->value),"lesser")) {
	free(argList);
	return evalLesser(argList[0].value,argList[1].value);
      } else if (!strcmp(getCharVal(curr->value),"greater")) {
	free(argList);
	return evalLesser(argList[1].value,argList[0].value);
      } else {
	SymbolIdent* symbolGot;
	hashmap_get(symbolmap, getCharVal(curr->value),&symbolGot);
	int k = 0;
	NameListNode* count_temp = symbolGot->argNames;
	while (count_temp) {
	  k++;
	  count_temp = count_temp->next;
	}
	count_temp = symbolGot->argNames;
	ArgName arguments[k];
	for (int l = 0; l < k; l++) {
	  arguments[l].value = argList[l].value;
	  arguments[l].ident = count_temp->name;
	  count_temp = count_temp->next;
	}
	DPRINT("%ld: evaluated user-defined symbol %s\n", pthread_self(), getCharVal(curr->value));
	free(argList);
	return eval(symbolGot->parseTree,arguments,k);
      }
      break;
    }
  case ValueType_INT:
  case ValueType_LIST:
    DPRINT("%ld: evaluated constant value ", pthread_self());
    dValPrint(curr->value);
    DPRINT("\n");
    return curr->value;
  }
}

/**
 * This function is the one which is called when creating a new thread.
 * @return: Always return 0
 */
void* prepSeqEval(void* arguments) {
  ForkArgs* args = (ForkArgs*) arguments;
  *(args->returnVal) = eval(args->target, args->args, args-> num);
  DPRINT("%ld: Finished working on tree %ld\n",pthread_self(), args->target);
  NUM_THREADS--;
  return 0;
}

/**
 * Creates a new thread
 * @param: The arguments for the new thread
 * @return: The thread id of the new thread
 */
pthread_t doFork(ForkArgs* args) {
  pthread_t tid;
  pthread_create(&tid, NULL, prepSeqEval, args);
  DPRINT("%ld: Created thread %ld working on tree %ld\n", pthread_self(), tid, args->target);
  NUM_THREADS++;
  return tid;
}

/**
 * Evaluates wether a new thread should be created
 * @param: The arguments the new thread would have
 * @return: 1 if a new thread should be created, 0 otherwise
 */
int checkFork(ForkArgs* args)
{
  if (NUM_THREADS < MAX_THREADS && getType(args->target->value) == ValueType_FUNCTION) {
    if (!exists(args->target->value.value.identifier)) {
      return 1;
    }
  }
  return 0; 
}

/**
 * Runs the interpretator loop
 * @param: Initial file to load from, may be stdin
 * @return: always returns 0
 */
int interpretate (FILE* in) {
  SymbolIdent* it = NULL;
  void* olololo;
  while(1){
    it = parse(in, debug);
    in = NULL;
    if (it == 5) {
      if (debug != stdin)
	fclose(debug);
      return 0;
    }
    else if(it){
      if(it->name){
	if(exists(it->name) || 
	   hashmap_get(symbolmap, it->name, &olololo) == MAP_OK) {
	  printf("redefinition is not allowed\n");
	}
	else {
	  if (it->argNames) {
	    hashmap_put(symbolmap, it->name, it);
	    printf("Defined function %s\n",it->name);
	  }
	  else {
	    SymbolIdent* newIdent = malloc(sizeof(SymbolIdent));
	    newIdent -> name = it->name;
	    newIdent -> argNames = NULL;
	    TreeNode* newNode = malloc(sizeof(TreeNode));
	    newNode->value = eval(it->parseTree,NULL,0);
	    newNode->argList = NULL;
	    newIdent -> parseTree = newNode;
	    hashmap_put(symbolmap, it->name, newIdent);
	    printf("Defined %s = ",it->name);
	    valPrint(newNode->value);
	    printf("\n");
	  } 
	}
      }
      else{
	Val calced = eval(it->parseTree, NULL,0);
	valPrint(calced);
	printf("\n");
	freeSymbol(it);
	freeVal(calced);
      }
    }
  }
  return 0;
}

/**
 *Initiates program
 * @param: Various flags
 * @return: Always returns 0
 */
int main(int argv, char* argc[]) {
  symbolmap = hashmap_new();
  FILE* in = stdin;
  if (argv > 1) {
    for (int n = 1; n < argv; n++) {
      if (!strcmp(argc[n],"-f")) {
	in = fopen(argc[n+1],"r");
	if (!in) {
	  printf("Failed to open %s\n", argc[n+1]);
	  in = stdin;
	}
	n++;
      } else if (!strcmp(argc[n],"-d")) {
	if ((argv == n+1) || argc[n+1][0] == '-') {
	  debug = stdout;
	}
	else {
	  debug = fopen(argc[n+1],"w");
	  printf("%s\n",argc[n+1]);
	  if (!debug)
	    printf("Failed to open debug file %s\n", argc[n+1]);
	  n++;
	}
      } else if (!strcmp(argc[n],"-s")) {
	MAX_THREADS = 0;
      }
    }
  }
  return interpretate(in);
}
