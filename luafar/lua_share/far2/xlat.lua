-- started: 2022-03-27
-- author:  Shmuel Zeigerman

local rus1 = {
  "ё", "й", "ц", "у", "к", "е", "н", "г", "ш", "щ", "з", "х", "ъ", "ф", "ы", "в",
  "а", "п", "р", "о", "л", "д", "ж", "э", "я", "ч", "с", "м", "и", "т", "ь", "б",
  "ю", ".", "Ё", '"', "№", ";", ":", "?", "Й", "Ц", "У", "К", "Е", "Н", "Г", "Ш",
  "Щ", "З", "Х", "Ъ", "Ф", "Ы", "В", "А", "П", "Р", "О", "Л", "Д", "Ж", "Э", "/",
  "Я", "Ч", "С", "М", "И", "Т", "Ь", "Б", "Ю", ",",
}

local lat1 = {
  "`", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "a", "s", "d",
  "f", "g", "h", "j", "k", "l", ";", "'", "z", "x", "c", "v", "b", "n", "m", ",",
  ".", "/", "~", "@", "#", "$", "^", "&", "Q", "W", "E", "R", "T", "Y", "U", "I",
  "O", "P", "{", "}", "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", '"', "|",
  "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?",
}

local map_rus, map_lat = {}, {}
for i,r in ipairs(rus1) do
  local l = lat1[i]
  map_rus[r] = l
  map_lat[l] = r
end

-- the algorithm is not silly but also not too smart
local function xlat (Line, StartPos, EndPos)
  if StartPos or EndPos then
    Line = Line:sub(StartPos, EndPos)
  end
  local k, from = 0, 1
  local prev_src
  local t = {}
  for c in Line:gmatch(".") do
    k = k + 1
    t[k] = c
    local src
    if     map_rus[c] and not map_lat[c] then src = map_rus
    elseif map_lat[c] and not map_rus[c] then src = map_lat
    elseif map_lat[c] and map_rus[c] and from==k then src = prev_src
    end
    if src then
      prev_src = src
      for i=from,k do t[i]=src[t[i]] or t[i]; end
      from = k + 1
    end
  end
  return table.concat(t)
end

return xlat
