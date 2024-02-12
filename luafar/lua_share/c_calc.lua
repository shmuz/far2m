-- Started on:      2021-02-08
-- Started by:      Shmuel Zeigerman
-- Language:        Lua (version >= 5.1)
-- Dependencies:    (1) bit.dll (http://bitop.luajit.org/) - not needed for Lua versions >= 5.3

local bit, MAXBITS
if _VERSION < "Lua 5.3" then
  bit = _G.bit or _G.bit64 or require "bit" -- bit64 is part of luafar library
  MAXBITS = 32
else
  bit = {}
  bit.bnot   = load("local a=...; return ~a")
  bit.band   = load("local a,b=...; return a & b")
  bit.bor    = load("local a,b=...; return a | b")
  bit.bxor   = load("local a,b=...; return a ~ b")
  bit.lshift = load("local a,b=...; return a << b")
  bit.rshift = load("local a,b=...; return a >> b")
  MAXBITS = 64
end
local unpack = rawget(table, "unpack") or unpack

local Calc = {
  mInput = nil;
  mPos = nil;
  mGetVar = nil;
  mToken = nil;
}
local meta = { __index=Calc; }

local function NewCalc(str, getvar)
  local self = { mInput=str; mPos=1; mGetVar=getvar; }
  return setmetatable(self, meta)
end

-- Important: patterns matching 2 bytes must precede patterns matching 1 byte
local patterns = {
  "^(||)", "^(&&)", "^(==)", "^(!=)", "^(<=)", "^(>=)", "^(<<)", "^(>>)", -- match 2 byte text
  "^([<>%-+/%%*&%^|()!~,])",                                              -- match 1 byte text
}

function Calc:get_token()
  if self.mToken then return self.mToken; end

  local pos = self.mInput:find("%S", self.mPos) -- skip spaces
  if not pos then return nil; end

  -- look for operators and parentheses
  for _,patt in ipairs(patterns) do
    local from, to, val = self.mInput:find(patt, pos)
    if from then
      self.mPos, self.mToken = to+1, val
      return self.mToken
    end
  end

  -- look for hexadecimal numbers
  local from,to,val,w = self.mInput:find("^(0[xX]%x+)(%w?)", pos)
  if from then
    if w=="" then
      self.mPos, self.mToken = to+1, tonumber(val) -- NOT (val,16) as it will truncate to 0xffffffff
      return self.mToken
    end
    error("bad token on position "..pos)
  end

  -- look for binary numbers
  from,to,val,w = self.mInput:find("^0[bB]([01]+)(%w?)", pos)
  if from then
    if w=="" then
      self.mPos, self.mToken = to+1, tonumber(val,2)
      return self.mToken
    end
    error("bad token on position "..pos)
  end

  -- look for octal numbers
  from,to,val,w = self.mInput:find("^(0[0-7]+)(%w?)", pos)
  if from then
    if w=="" then
      self.mPos, self.mToken = to+1, tonumber(val,8)
      return self.mToken
    end
    error("bad token on position "..pos)
  end

  -- look for decimal and floating point numbers
  from,to,val,w = self.mInput:find("^(%d*%.?%d*[eE][+%-]?%d+)(%w?)", pos)
  if not from then
    from,to,val,w = self.mInput:find("^(%d*%.?%d*)(%w?)", pos)
  end
  if from and val~="" and val~="." then
    if w=="" then
      self.mPos, self.mToken = to+1, tonumber(val)
      return self.mToken
    end
    error("bad token on position "..pos)
  end

  -- look for variables (identifiers)
  from,to,val = self.mInput:find("^([_a-zA-Z][_a-zA-Z0-9]*)", pos)
  if from then
    self.mPos, self.mToken = to+1, self.mGetVar(val)
    if type(self.mToken) == "number" then
      return self.mToken
    elseif type(self.mToken) == "function" then
      self.mToken = { name=val; func=self.mToken } -- array part is for keeping function arguments
      return self.mToken
    else
      error("variable '" .. val .. "' is neither a number nor a function")
    end
  end

  error("bad token on position "..pos)
end

function Calc:get_term()
  local result
  local tok = self:get_token()
  self.mToken = nil
  if tok == "-" then -- unary minus
    result = -self:get_term();
  elseif tok == "+" then -- unary plus
    result = self:get_term();
  elseif tok == "!" then -- logical NOT
    result = self:get_term() == 0 and 1 or 0
  elseif tok == "~" then -- bitwise NOT
    result = bit.bnot(self:get_term())
  elseif tok == "(" then
    result = self:get_logic_or()
    tok = self:get_token()
    self.mToken = nil
    if not tok    then error ("')' expected") end
    if tok ~= ")" then error ("')' expected, got "..tok) end
  elseif type(tok) == "number" then
    result = tok
  elseif type(tok) == "table" then -- function call
    local tok2 = self:get_token()
    self.mToken = nil
    if tok2 ~= "(" then
      error ("'(' expected")
    end
    if self:get_token() == ")" then
      self.mToken = nil
    else
      while true do -- get all arguments
        local arg = self:get_logic_or()
        if not arg then error("function argument expected"); end
        table.insert(tok, arg)
        tok2 = self:get_token()
        self.mToken = nil
        if tok2 == ")" then break; end
        if tok2 ~= "," then error("',' or ')' expected"); end
      end
    end
    result = tok.func(unpack(tok))
    if type(result) ~= "number" then
      error("function '"..tok.name.."' returned a non-number")
    end
  end
  return result
end

function Calc:get_multiply_divide()
  local r = self:get_term()
  while true do
    local tok = self:get_token()
    if not (tok == "*" or tok == "/" or tok == "%") then
      return r
    end
    self.mToken = nil
    local r2 = self:get_term()
    if not r2 then error("multiplication/division operand expected") end
    if     tok == "*" then r = r * r2
    elseif tok == "/" then r = r / r2
    elseif tok == "%" then r = math.fmod(r, r2)
    end
  end
end

function Calc:get_add_subtract()
  local r = self:get_multiply_divide()
  while true do
    local tok = self:get_token()
    if not (tok == "+" or tok == "-") then
      return r
    end
    self.mToken = nil
    local r2 = self:get_multiply_divide()
    if not r2 then error("addition/subtraction operand expected") end
    if     tok == "+" then r = r + r2
    elseif tok == "-" then r = r - r2
    end
  end
end

function Calc:get_shift()
  local r = self:get_add_subtract()
  while true do
    local tok = self:get_token()
    if not (tok == "<<" or tok == ">>") then
      return r
    end
    self.mToken = nil
    local r2 = self:get_add_subtract()
    if not r2 then error("shift operand expected") end
    r2 = math.floor( math.max(0,r2) )
    r = (r2 >= MAXBITS) and 0 or (tok == "<<") and bit.lshift(r,r2) or bit.rshift(r,r2)
  end
end

function Calc:get_less_more()
  local r = self:get_shift()
  while true do
    local tok = self:get_token()
    if not (tok == "<" or tok == "<=" or tok == ">" or tok == ">=") then
      return r
    end
    self.mToken = nil
    local r2 = self:get_shift()
    if not r2 then error("comparison operand expected") end
    if     tok == "<"  then r = (r <  r2)  and 1 or 0
    elseif tok == "<=" then r = (r <= r2)  and 1 or 0
    elseif tok == ">"  then r = (r >  r2)  and 1 or 0
    elseif tok == ">=" then r = (r >= r2)  and 1 or 0
    end
  end
end

function Calc:get_equal()
  local r = self:get_less_more()
  while true do
    local tok = self:get_token()
    if not (tok == "==" or tok == "!=") then
      return r
    end
    self.mToken = nil
    local r2 = self:get_less_more()
    if not r2 then error("equality operand expected") end
    if tok == "==" then r = (r2 == r) and 1 or 0
    else r = (r2 == r) and 0 or 1
    end
  end
end

function Calc:get_bitwise_and()
  local r = self:get_equal()
  while true do
    local tok = self:get_token()
    if tok ~= "&" then
      return r
    end
    self.mToken = nil
    local r2 = self:get_equal()
    if not r2 then error("bitwise AND operand expected") end
    r = bit.band(r, r2)
  end
end

function Calc:get_bitwise_xor()
  local r = self:get_bitwise_and()
  while true do
    local tok = self:get_token()
    if tok ~= "^" then
      return r
    end
    self.mToken = nil
    local r2 = self:get_bitwise_and()
    if not r2 then error("bitwise XOR operand expected") end
    r = bit.bxor(r, r2)
  end
end

function Calc:get_bitwise_or()
  local r = self:get_bitwise_xor()
  while true do
    local tok = self:get_token()
    if tok ~= "|" then
      return r
    end
    self.mToken = nil
    local r2 = self:get_bitwise_xor()
    if not r2 then error("bitwise OR operand expected") end
    r = bit.bor(r, r2)
  end
end

function Calc:get_logic_and()
  local r = self:get_bitwise_or()
  while true do
    local tok = self:get_token()
    if tok ~= "&&" then
      return r
    end
    self.mToken = nil
    local r2 = self:get_bitwise_or()
    if not r2 then error("logical AND operand expected") end
    r = (r ~= 0 and r2 ~= 0) and 1 or 0
  end
end

function Calc:get_logic_or (toplevel)
  local r = self:get_logic_and()
  if not r then
    error(toplevel and "empty expression" or "incomplete expression")
  end
  while true do
    local tok = self:get_token()
    if not tok then return r; end
    if tok ~= "||" then
      if not toplevel then return r; end
      error("misplaced token "..tok)
    end
    self.mToken = nil
    local r2 = self:get_logic_and()
    if not r2 then error("logical OR operand expected") end
    r = (r == 0 and r2 == 0) and 0 or 1
  end
end

local function expr (str, getvar)
  getvar = getvar or function() end
  assert(type(getvar)=="function", "arg#2 is not a function")
  local calc = NewCalc(str, getvar)
  return calc:get_logic_or(true)
end

return {
  expr = expr;
}
