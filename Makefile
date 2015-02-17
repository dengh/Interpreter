#
# Author: Nikos Nikoleris <nikos.nikoleris@it.uu.se>
# Modified by: Jonatan Waern

DEBUG ?=y

CFLAGS=-pthread -std=c99 -D_XOPEN_SOURCE=600 -w
LDFLAGS=-pthread -lrt
SRC=./src
BUILD=./build
TEST=./tests
DOC=./doc

CC=gcc

ifeq ($(DEBUG), y)
	CFLAGS += -ggdb
	LDFLAGS += -ggdb
else
	CFLAGS += -O2
	LDFLAGS += -O2
endif

all: 	interpreter

demo:	all
	$(BUILD)/interpreter -f $(SRC)/demo

doc:	clean
	doxygen Doxyfile

test:	all $(SRC)/CU_interpreter.c
	$(CC) $(CFLAGS) $(SRC)/CU_interpreter.c $(SRC)/parser.tab.c $(SRC)/structures.c $(SRC)/lex.yy.c -o $(BUILD)/CU_interpreter -lcunit
	$(BUILD)/CU_interpreter
	$(BUILD)/interpreter -f $(TEST)/master_suite

run: 	all
	$(BUILD)/interpreter

debugmode: all
	$(BUILD)/interpreter -d

debug:	all
	gdb $(BUILD)/interpreter

interpreter: parser $(SRC)/interpreter.c $(SRC)/hashmap.c $(SRC)/hashmap.h
	$(CC) $(CFLAGS) $(SRC)/interpreter.c $(SRC)/parser.tab.c $(SRC)/structures.c $(SRC)/lex.yy.c $(SRC)/hashmap.c -o $(BUILD)/interpreter -lrt

parser: $(SRC)/tokenizer.l $(SRC)/parser.y $(SRC)/structures.h $(SRC)/structures.c
	bison $(SRC)/parser.y --defines=$(SRC)/parser.tab.h -o $(SRC)/parser.tab.c		
	flex -o $(SRC)/lex.yy.c $(SRC)/tokenizer.l 	

clean:
	ls $(BUILD)/ | grep -v "\." | (cd $(BUILD); xargs $(RM)) 
	$(RM) $(BUILD)/* $(BUILD)/*.o $(BUILD)/*.d $(SRC)/*~ $(TEST)/*~
	$(RM) $(SRC)/parser.tab.h $(SRC)/parser.tab.c $(SRC)/lex.yy.c
	$(RM) -r $(DOC)/html $(DOC)/latex

-include $(wildcard *.d)		