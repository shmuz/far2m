local prefix = "far2.test.regex."

local luatest = require (prefix .. "luatest")

-- returns: number of failures
local function test_library (lib, setfile, printfunc, verbose)
  local f = require (prefix .. setfile)
  local sets = f(lib)
  local n = 0 -- number of failures
  for _, set in ipairs (sets) do
    if verbose then
      printfunc (set.Name or "Unnamed set")
    end
    local err = luatest.test_set (set, lib)
    n = n + #err
    if verbose then
      for _,v in ipairs (err) do
        printfunc ("  Test " .. v.i)
        luatest.print_results (v, printfunc, "  ")
      end
    end
  end
  if verbose then
    printfunc ""
  end
  return n
end

local function main (printfunc, verbose)
  printfunc = printfunc or function() end
  local libname = "rexfar"
  local test = { "common_sets", "pcre_sets", "pcre_sets2" }
  local nerr = 0
  for _, setfile in ipairs (test) do
    if verbose then
      printfunc (("[lib: %s; file: %s]"):format (libname, setfile))
    end
    nerr = nerr + test_library (regex, setfile, printfunc, verbose)
  end
  printfunc ("Total number of failures: " .. nerr)
  return nerr
end

return main
