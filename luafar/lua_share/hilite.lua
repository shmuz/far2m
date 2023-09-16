-- coding: utf-8
-- started 2010-06-07 by Shmuel Zeigerman

local unicode = require "unicode"
getmetatable("").__index = unicode.utf8

--[[---------------------------------------------------------------------------
Purpose:
  Get automatically highlighting (hot keys) for strings in a menu or dialog.
-------------------------------------------------------------------------------
Parameters:
  @Arr:
    An array of strings. The strings containing & are excluded from the
    processing but their highlighted characters will be considered reserved
    and not available for highlighting in other strings.
  @Patt (optional):
    Lua pattern for determining what characters can be highlighted.
    The default is '%w', but '%S', '%a', etc. can also be used.
-------------------------------------------------------------------------------
Returns:
  On success: an array of highlighted strings.
  On failure: nil, error message.
-------------------------------------------------------------------------------
--]]

local function hilite (Arr, Patt)
  Patt = Patt or "%w"
  local state, idx, wei, out = {}, {}, {}, {}
  ------------------------------------------------ initialize 'state' and 'idx'
  local patt2 = "%&(" .. Patt .. ")"
  for i, str in ipairs(Arr) do
    local _, n = str:lower():gsub(patt2, function(c) state[c]="reserved" end, 1)
    if n == 0 then idx[#idx+1] = i end
  end
  ------------------------------------------------------------ initialize 'wei'
  for _, v in ipairs(idx) do
    local t = {}
    for c in Arr[v]:lower():gmatch(Patt) do
      if not state[c] and not t[c] then
        wei[c] = (wei[c] or 0) + 1
        t[c] = true
      end
    end
  end
  ------------------------------------------------------------------------ sort
  local function get_weight (str)
    local w, t = 0, {}
    for c in str:lower():gmatch(Patt) do
      if not t[c] and not state[c] then w=w + 1/wei[c]; t[c]=true; end
    end
    return w
  end
  table.sort(idx, function(i1, i2)
    return get_weight(Arr[i1]) < get_weight(Arr[i2]) end)
  ---------------------------------------------------------------------- assign
  for _, v in ipairs(idx) do
    local found
    local s = Arr[v]:gsub(Patt,
      function(c)
        if not found and not state[c:lower()] then
          found = true
          state[c:lower()] = "assigned"
          return "&"..c
        end
      end)
    --print(s)
    if found then out[#out+1] = s
    else return nil, "failed on `" .. Arr[v] .. "'"
    end
  end
  return out
end

return hilite
