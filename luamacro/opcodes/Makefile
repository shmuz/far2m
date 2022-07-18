# This makefile generates the file opcodes.lua that is part of LuaMacro plugin

ROOT  = $(HOME)/far2l
FAR2L = $(ROOT)/far2l/src

TRG = opcodes.lua
INC = -I$(FAR2L)/console -I$(FAR2L)/macro -I$(ROOT)/WinPort
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
