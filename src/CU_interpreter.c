#include "structures.h"
#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <CUnit/Basic.h>

int init_Basic(void)
{
  return 0;
}


int clean_Basic(void)
{
  return 0;
}

void testVAL(void)
{
  FILE* in;
  in = fopen("./tests/testULTIMATE", "r");
  SymbolIdent* it = parse(in, NULL);
  CU_ASSERT(!strcmp(it->name, "sumlist")); //name of function
  CU_ASSERT(!strcmp(it->argNames->name, "x")); //name of arg to function
  CU_ASSERT(!strcmp(getCharVal(it->parseTree->value), "ite")); // name of first expression in function
  CU_ASSERT(!strcmp(getCharVal(it->parseTree->argList->target->value), "length")); // name of first expression in if-then-else
  CU_ASSERT(!strcmp(getCharVal(it->parseTree->argList->target->argList->target->value), "x"));
  CU_ASSERT(!strcmp(getCharVal(it->parseTree->argList->next->target->value), "plus"));
  CU_ASSERT(!strcmp(getCharVal(it->parseTree->argList->next->target->argList->target->value), "hd"));
  CU_ASSERT(!strcmp(getCharVal(it->parseTree->argList->next->target->argList->target->argList->target->value), "x"));
  CU_ASSERT(!strcmp(getCharVal(it->parseTree->argList->next->target->argList->next->target->value), "sumlist"));
  CU_ASSERT(!strcmp(getCharVal(it->parseTree->argList->next->target->argList->next->target->argList->target->value), "tl"));
  CU_ASSERT(!strcmp(getCharVal(it->parseTree->argList->next->target->argList->next->target->argList->target->argList->target->value), "x"));
}
int main()
{
   CU_pSuite pSuite = NULL;

   /* initialize the CUnit test registry */
   if (CUE_SUCCESS != CU_initialize_registry())
      return CU_get_error();

   /* add a suite to the registry */
   pSuite = CU_add_suite("Basic", init_Basic, clean_Basic);
   if (NULL == pSuite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* add the tests to the suite */
   /* NOTE - ORDER IS IMPORTANT - MUST TEST fread() AFTER fprintf() */
   if ((NULL == CU_add_test(pSuite, "test of structures", testVAL)))
     {
       CU_cleanup_registry();
       return CU_get_error();
     }

   /* Run all tests using the CUnit Basic interface */
   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_basic_run_tests();
   CU_cleanup_registry();
   return CU_get_error();
}
