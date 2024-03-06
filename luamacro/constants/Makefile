# This makefile generates the file opcodes.lua that is part of LuaMacro plugin

ROOT  = $(HOME)/far2m
FAR = $(ROOT)/far

SCRIPT = make_list.lua
LUA = lua
TRG = constants.lua
INP = $(FAR)/src/macro/macroopcode.hpp \
      $(FAR)/src/macro/macrovalues.hpp
INC = -I$(FAR) -I$(FAR)/src/console -I$(FAR)/src/macro -I$(ROOT)/WinPort
CPPFLAGS = $(INC)

GEN = gen
OBJ = $(GEN).o
SRC = $(GEN).cpp

$(TRG): $(GEN)
	./$< $@

$(GEN): $(OBJ)
	g++ -o$@ $^

$(SRC): $(INP)
	$(LUA) $(SCRIPT) $@ $^

clean:
	rm $(TRG) $(GEN) $(OBJ) $(SRC)

.PHONY: clean
