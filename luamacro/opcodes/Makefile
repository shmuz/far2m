# This makefile generates the file opcodes.lua that is part of LuaMacro plugin

ROOT  = $(HOME)/far2m
FAR2M = $(ROOT)/far

TRG = opcodes.lua
INC = -I$(FAR2M) -I$(FAR2M)/src/console -I$(FAR2M)/src/macro -I$(ROOT)/WinPort
EXE = generate
OBJ = opcodes.o
CPPFLAGS = $(INC)

$(TRG): $(EXE)
	./$<

$(EXE): $(OBJ)
	g++ -o$@ $^

opcodes.cpp: make_list.lua
	lua $^

clean:
	rm $(TRG) $(EXE) $(OBJ) opcodes.cpp

.PHONY: clean
