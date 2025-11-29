local AF = "my assertion failed"
local asrt = {}

function asrt.eq(a,b,m)      assert(a == b, m or AF)               return true; end
function asrt.neq(a,b,m)     assert(a ~= b, m or AF)               return true; end
function asrt.num(v,m)       assert(type(v)=="number", m or AF)    return v; end
function asrt.str(v,m)       assert(type(v)=="string", m or AF)    return v; end
function asrt.table(v,m)     assert(type(v)=="table", m or AF)     return v; end
function asrt.bool(v,m)      assert(type(v)=="boolean", m or AF)   return true; end
function asrt.func(v,m)      assert(type(v)=="function", m or AF)  return v; end
function asrt.udata(v,m)     assert(type(v)=="userdata", m or AF)  return v; end
function asrt.isnil(v,m)     assert(v==nil, m or AF)               return true; end
function asrt.isfalse(v,m)   assert(v==false, m or AF)             return true; end
function asrt.istrue(v,m)    assert(v==true, m or AF)              return true; end
function asrt.err(...)       assert(pcall(...)==false, AF)         return true; end
function asrt.noerr(...)     assert(pcall(...)==true, AF)          return true; end

function asrt.range(v, low, high)
  if low then assert(v >= low, v) end
  if high then assert(v <= high, v) end
  return v
end

function asrt.numint(v,m)
  assert(type(v)=="number" or bit64.type(v), m or AF)
  return v
end

return asrt
