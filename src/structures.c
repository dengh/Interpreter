/**
 * @brief: This is the file containing the implementations of the various functions that manipulate the data structures used by the interpretor
 * @file: structures.c
 * @author: Jonatan Waern, Daniel Engh, Adam Olevall, Mikael Holmberg
 * @date: 4/7 2013
 */
#include "structures.h"
#include <stdint.h>
#include <stdlib.h>

/**
 * Obtains the type of a value as an enum
 * @return the type of the value
 */
ValueType getType (Val v) {
  switch (v.type) {
  case 0:
    return ValueType_INT;
  case 1:
    return ValueType_LIST;
  case 2:
    return ValueType_CONSTANT;
  case 3:
    return ValueType_FUNCTION;
  }
  return ValueType_INT;
}

/**
 * Obtains an int value from a Val
 * @return: the value interpreted as an intptr_t
 */
intptr_t getIntVal(Val v) {
  return v.value.intval;
}

/**
 * Obtains a list from a val
 * @return: a pointer to the begining of the list identified by the Val
 */
ValList* getListVal(Val v) {
  return v.value.listStart;
}

/**
 * Obtains a the length of a list
 * @return: the length of the list that is identified by the Val
 */
int getListLength(Val v) {
  ValList* tempList = getListVal(v);  
  if(!tempList){
    return 0;
  }
  else{
    int i = 1;
    while(tempList->next){
      tempList = tempList->next;
      i++;
    }
    return i;
  }
}

/**
 * Checks wether two lists are equal
 * @return: returns 1 of the list identified by the first Val is equal to the list identified by the second Val, return 0 otherwise.
 */
int getListsEqual(Val arg1, Val arg2) {
  ValList* tempList1 = getListVal(arg1);
  ValList* tempList2 = getListVal(arg2);
  while(tempList1 && tempList2){
    if(getIntVal(tempList1->value) != getIntVal(tempList2->value))
      return 0;
    tempList1 = tempList1->next;
    tempList2 = tempList2->next;
  }
  if((!tempList1 && !tempList2)){
    return 1;
  }
  else{
    return 0;
  }
}
     
/**
 * Obtains a string from a Val
 * @return: a pointer to the begining of the string identified by the Val
 */
char* getCharVal(Val v) {
  return v.value.identifier;
}

/**
 * Obtains an argument from a TreeNode
 * @return: the n-th child of the TreeNode (indexed by 0)
 */
TreeNode* getArgNode (TreeNode* node, int ArgNum) {
  PointerListNode* argument = node->argList;
  for (int i = 0; i < ArgNum; i++) {argument = argument->next;}
  return argument->target;
}

/**
 * Obtains an identifier from a SymbolIdent
 * @return: the n-th identifier of the SymbolIdent (indexed by 0)
 */
char* getArgName (SymbolIdent* node, int ArgIndex) {
  NameListNode* argument = node->argNames;
  for (int i = 0; i < ArgIndex ; i++) {argument = argument->next;}
  return argument->name;
}

/**
 * Sets up a val according to specifications
 * @return: a new val with type and value according to arguments
 */
Val createVal(ValueType type, intptr_t value) {
  Val returnVal;
  returnVal.value.intval = value; //dubbelkod?
  switch (type) {
  case ValueType_INT:
    returnVal.type = 0;
    break;
  case ValueType_LIST:
    returnVal.type = 1;
    break;
  case ValueType_CONSTANT:
    returnVal.type = 2;
    break;
  case ValueType_FUNCTION:
    returnVal.type = 3;
    break;
  }
  returnVal.value.intval = value;
  return returnVal;
}

/**
 * Frees the memory that the value points to
 */
void freeVal(Val target) {
  //if (getType(target) == ValueType_LIST)
    //freeValList(target.value.listStart);
  if (getType(target) == ValueType_CONSTANT ||
      getType(target) == ValueType_FUNCTION)
    if (target.value.identifier)
      free(target.value.identifier);
}

/**
 * Frees the memory allocated to a Val list and to all things it may point to
 */
void freeValList(ValList* target) {
  if (target) {
    freeVal(target->value);
    freeValList(target->next);
    free(target);
  }
}

/**
 * Frees the memory allocated to a NameList and to all things it may point to
 * @warning: This frees the strings that the nodes point to
 */
void freeNameList(NameListNode* target) {
  if (target) {
    if (target->name)
      free(target->name);
    freeNameList(target->next);
    free(target);
  }
}

/**
 * Free the memory allocated to a pointerlist and to all things it may point to
 */
void freePointerList(PointerListNode* target) {
  if (target) {
    freeTree(target->target);
    freePointerList(target->next);
    free(target);
  }
}

/**
 * Frees the memory allocated to a TreeNode , and recursively to all its children and all things it may point to
 */
void freeTree(TreeNode* target) {
  if (target) {
    freePointerList(target->argList);
    freeVal(target->value);
    free(target);
  }
}

/**
 * Frees the memory allocated to a SymbolIdent, and recursively to all things it may point to
 */
void freeSymbol(SymbolIdent* target) {
  if (target) {
    if (target->name)
      free(target->name);
    freeNameList(target->argNames);
    freeTree(target->parseTree);
    free(target);
  }
}
