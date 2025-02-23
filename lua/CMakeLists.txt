project(lua)

set(SOURCES
  src/lua.c
  src/lapi.c
  src/lcode.c
  src/ldebug.c
  src/ldo.c
  src/ldump.c
  src/lfunc.c
  src/lgc.c
  src/llex.c
  src/lmem.c
  src/lobject.c
  src/lopcodes.c
  src/lparser.c
  src/lstate.c
  src/lstring.c
  src/ltable.c
  src/ltm.c
  src/lundump.c
  src/lvm.c
  src/lzio.c
  src/lauxlib.c
  src/lbaselib.c
  src/ldblib.c
  src/liolib.c
  src/lmathlib.c
  src/loslib.c
  src/ltablib.c
  src/lstrlib.c
  src/loadlib.c
  src/linit.c
)

set(CMAKE_C_VISIBILITY_PRESET default)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLUA_USE_MKSTEMP") # suppress a warning

add_executable (${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} m)
