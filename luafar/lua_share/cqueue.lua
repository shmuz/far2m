-- cqueue --> circular queue
-- author  : Shmuel Zeigerman
-- started : 2016-03-01

-- Functions : new
-- Methods   : back, capacity, clear, empty, front, get, pop, push, set, size

local cqueue = {}
local cqmeta = {__index=cqueue}

function cqueue.new (maxsize)
  assert(type(maxsize)=="number" and maxsize>0, "new_cqueue, arg#1, positive number expected")
  return setmetatable({m_max=maxsize; m_size=0; m_start=0}, cqmeta)
end

-- helper function
function cqueue:index (offset)
  return (self.m_start + offset - 1) % self.m_max + 1
end

function cqueue:back()
  if self.m_size > 0 then return self[self:index(self.m_size)]
  else return nil
  end
end

function cqueue:capacity()
  return self.m_max
end

function cqueue:clear()
  self.m_size=0
end

function cqueue:empty()
  return self.m_size==0
end

function cqueue:front()
  if self.m_size > 0 then return self[self.m_start+1]
  else return nil
  end
end

function cqueue:get (i)
  if i>=1 and i<=self.m_size then
    return self[self:index(i)]
  end
  return nil
end

function cqueue:pop()
  if self.m_size > 0 then
    self.m_start = (self.m_start+1) % self.m_max
    self.m_size = self.m_size - 1
  end
end

function cqueue:push (val)
  if self.m_size < self.m_max then self.m_size = self.m_size + 1
  else self.m_start = (self.m_start+1) % self.m_max
  end
  self[self:index(self.m_size)] = val
end

function cqueue:set (i, val)
  if i>=1 and i<=self.m_size then
    self[self:index(i)] = val
  end
end

function cqueue:size()
  return self.m_size
end

return cqueue
