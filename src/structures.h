/**
 * @brief: This is the main header file, containing definitions for structures and the functions that manipulate these structures.
 * @file: structures.h
 * @author: Jonatan Waern
 * @date: 27/6 2013
 */


#ifndef STRUCTURE_HEADER
#define STRUCTURE_HEADER
#include <stdint.h>
struct PointerListNode;

/**
 *Enumerates different types of values
 */
typedef enum ValueType {
  ValueType_INT, 
  ValueType_LIST, 
  ValueType_CONSTANT, 
  ValueType_FUNCTION
} ValueType;

typedef struct ValList;

/**
 *Defines a value.
 *The interpretation of the value union within a value will depend on its type
 */
typedef struct {
  union {
    intptr_t intval; /** The int value */
    struct ValList* listStart; /** Pointer to first element of the list */
    char* identifier; /** Pointer to the string identifier */
  } value; /** The actual value of a Val, can be interpreted in various ways*/
  char type; /** Defines how to interpret the value union */
} Val;

/**
 *Defines a list of values.
 *Defines a node in a list of values.
 */
typedef struct ValList {
  Val value; /** The value of this list node */
  struct ValList* next; /** The next node */
} ValList;

/**
 *Defines a node in the parse tree.
 */
typedef struct TreeNode {
  struct PointerListNode* argList; /** The list of pointers to the nodes children*/
  Val value; /** The value of the node */
} TreeNode;

/**
 *Defines a list of strings.
 *Defines a node in a list of strings.
 */
typedef struct NameListNode {
  char* name; /** The string of this node */
  struct NameListNode* next; /** The next node */
} NameListNode;

/**
 * Defines a top-level identifier for a symbol or function.
 */
typedef struct SymbolIdent {
  char* name; /** The name of the symbol, if blank, it is merely an executable expression*/
  struct NameListNode* argNames; /** The list of the names of arguments, if blank, then the symbol is either an executable expression or a constant symbol*/
  struct TreeNode* parseTree; /** The parse tree */
} SymbolIdent;

/**
 * Defines a list of TreeNodes
 * Defines a node in a list of TreeNodes
 */
typedef struct PointerListNode {
  struct TreeNode* target; /** The TreeNode of this node*/
  struct PointerListNode* next; /** The next node */
} PointerListNode;

/**
 * Obtains the type of a value as an enum
 * @return the type of the value
 */
ValueType getType (Val v);
/**
 * Obtains an argument from a TreeNode
 * @return: the n-th child of the TreeNode (indexed by 0)
 */
TreeNode* getArgNode (TreeNode* t, int argnum);
/**
 * Obtains an identifier from a SymbolIdent
 * @return: the n-th identifier of the SymbolIdent (indexed by 0)
 */
char* getArgName (SymbolIdent* t, int argnum);
/**
 * Sets up a val according to specifications
 * @return: a new val with type and value according to arguments
 */
Val createVal(ValueType type, intptr_t value);
/**
 * Obtains an int value from a Val
 * @return: the value interpreted as an intptr_t
 */
intptr_t getIntVal (Val v);
/**
 * Obtains a string from a Val
 * @return: a pointer to the begining of the string identified by the Val
 */
char* getCharVal(Val v);
/**
 * Obtains a list from a val
 * @return: a pointer to the begining of the list identified by the Val
 */
ValList* getListVal (Val v);
/**
 * Obtains a the length of a list
 * @return: the length of the list that is identified by the Val
 */
int getListLength(Val v);
/**
 * Checks wether two lists are equal
 * @return: returns 1 of the list identified by the first Val is equal to the list identified by the second Val, return 0 otherwise.
 */
int getListsEqual(Val arg1, Val arg2);
/**
 * Frees the memory allocated to a SymbolIdent, and recursively to all things it may point to
 */
void freeSymbol(SymbolIdent*);
/**
 * Frees the memory allocated to a TreeNode , and recursively to all its children and all things it may point to
 */
void freeTree(TreeNode*);
/**
 * Free the memory allocated to a pointerlist and to all things it may point to
 */
void freePointerList(PointerListNode*);
/**
 * Frees the memory allocated to a NameList and to all things it may point to
 * @warning: This frees the strings that the nodes point to
 */
void freeNameList(NameListNode*);
/**
 * Frees the memory allocated to a Val list and to all things it may point to
 */
void freeValList(ValList*);
/**
 * Frees the memory that the value points to
 */
void freeVal(Val v);

#endif
