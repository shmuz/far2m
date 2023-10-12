-- Luacheck configuration file

if (...)=="far" then -- running from Far environment
  luafar = true
  luamacro = true

else
  local cfg = dofile(os.getenv("HOME")..
              "/far2m/_build/install/Plugins/luafar/lua_share/luacheck_config.lua")
  stds.luafar = cfg.luafar
  stds.luamacro = cfg.luamacro
  std = "max+luafar+luamacro"

end

ignore = {
  "212", -- unused argument
  "421", -- shadowing a local variable
}

-- quiet                     = nil -- Integer in range 0..3   0
-- color                     = nil -- Boolean   true
-- codes                     = nil -- Boolean   false
-- ranges                    = nil -- Boolean   false
-- formatter                 = nil -- String or function   "default"
-- cache                     = nil -- Boolean or string   false
-- jobs                      = nil -- Positive integer   1
-- exclude_files             = nil -- Array of strings   {}
-- include_files             = nil -- Array of strings   (Include all files)
-- global                    = nil -- Boolean   true
-- unused                    = nil -- Boolean   true
-- redefined                 = nil -- Boolean   true
-- unused_args               = nil -- Boolean   true
-- unused_secondaries        = nil -- Boolean   true
-- self                      = nil -- Boolean   true
-- std                       = nil -- String or set of standard globals   "max"
-- globals                   = nil -- Array of strings or field definition map   {}
-- new_globals               = nil -- Array of strings or field definition map   (Do not overwrite)
-- read_globals              = nil -- Array of strings or field definition map   {}
-- new_read_globals          = nil -- Array of strings or field definition map   (Do not overwrite)
-- not_globals               = nil -- Array of strings   {}
-- compat                    = nil -- Boolean   false
-- allow_defined             = nil -- Boolean   false
-- allow_defined_top         = nil -- Boolean   false
-- module                    = nil -- Boolean   false
-- max_line_length           = nil -- Number or false   120
-- max_code_line_length      = nil -- Number or false   120
-- max_string_line_length    = nil -- Number or false   120
-- max_comment_line_length   = nil -- Number or false   120
-- max_cyclomatic_complexity = nil -- Number or false   false
-- ignore                    = nil -- Array of patterns (see Patterns)   {}
-- enable                    = nil -- Array of patterns   {}
-- only                      = nil -- Array of patterns   (Do not filter)
