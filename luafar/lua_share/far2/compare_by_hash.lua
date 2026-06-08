--------------------------------------------------------------------------------
-- Started             : 2020-11-28
-- Author              : Shmuel Zeigerman
-- Minimal Far version : 3.0.3300
-- Far plugin          : LuaMacro, LF4Editor, LFSearch, LFHistory (any of them)
-- External dependency : md5.dll by PUC-Rio, with my unpublished extensions
-- Action:
--   * Select unique files and deselect non-unique ones (with regards to the opposite panel).
--   * Directories are always deselected.
--   * If the operation was interrupted by the user then no change in panels selection occurs.
--   * File names are ignored: only file sizes and file hashes are considered.
--------------------------------------------------------------------------------

local Title = "Compare panels by hash"
local F = far.Flags
local band = bit64.band
local dirsep = string.sub(package.config,1,1)

local function join(s1, s2)
  return s1=="" and s2 or s1:find(dirsep.."$") and s1..s2 or s1..dirsep..s2
end

local function is_dir(item)
  return item.FileAttributes:find("d") and true
end

local function ShowWaitMessage()
  far.Message("Please wait...", Title, "")
end

local function userbreak()
  if win.ExtractKey()=="ESCAPE" then
    if 1==far.Message("Break the operation?", Title, "&Yes;&No", "w") then
      return true
    end
    ShowWaitMessage()
  end
end

local function callback_limit() return true end

local function CompareByHash()
  local md5 = require "md5ex"
  local act = { List={}; Map={}; Size={} }
  local pas = { List={}; Map={}; Size={} }
  local Equal = {}

  -- stage 1: collect info
  ShowWaitMessage()
  for _,pan in ipairs {act,pas} do
    local pan_code = (pan==act and 1 or 0)
    panel.UpdatePanel(nil,pan_code) -- it's important to acquire actual information
    local info = panel.GetPanelInfo(nil,pan_code)
    for i=1,info.ItemsNumber do
      local v=panel.GetPanelItem(nil,pan_code,i)
      pan.List[i]=v
      if is_dir(v) then Equal[v]=true
      else pan.Size[v.FileSize]=true
      end
    end
  end

  -- stage 2: fill maps (leave only files which size is encountered in the other panel)
  for _,pan in ipairs {act,pas} do
    local other = pan==act and pas or act
    local pan_code = (pan==act and 1 or 0)
    local dir = panel.GetPanelDirectory(nil,pan_code).Name
    for _,v in ipairs(pan.List) do
      if not is_dir(v) and other.Size[v.FileSize] then
        pan.Map[v]=true
        v.fullname = join(dir, v.FileName) -- cache full name
      end
    end
  end

  -- stage 3: calculate partial and full hashes
  local offset = 0
  local step = 0x100000 -- 1 MiB
  while next(act.Map) do
    for _,pan in ipairs {act,pas} do
      pan.Hash, pan.PartHash = {}, {}
      for v in pairs(pan.Map) do
        if userbreak() then return end
        local hash,status = md5.fsum(v.fullname, offset, nil, callback_limit, step)
        if hash then
          if status then -- file processed till the end
            v.hash=hash
            pan.Hash[hash]=true
          else -- callback cancel
            v.parthash=hash
            pan.PartHash[hash]=true
          end
        else
          pan.Map[v]=nil -- file open failure
        end
      end
    end

    for _,pan in ipairs {act,pas} do
      local other = pan==act and pas or act
      for v in pairs(pan.Map) do
        if v.hash then
          if other.Hash[v.hash] then
            Equal[v]=true
          end
          pan.Map[v]=nil
        else
          if not other.PartHash[v.parthash] then
            pan.Map[v]=nil
          end
        end
      end
    end
    offset = offset + step
  end

  -- stage 4: select unique files and deselect non-unique ones (with regards to the opposite panel)
  for _,pan in ipairs {act,pas} do
    local pan_code = (pan==act and 1 or 0)
    for i,v in ipairs(pan.List) do
      panel.SetSelection(nil, pan_code, i, not Equal[v])
    end
    panel.RedrawPanel(nil,pan_code)
  end
  far.Message("Done", Title)
end

local function CanRun()
  for i=0,1 do
    local inf = panel.GetPanelInfo(nil,i)
    if band(inf.Flags, F.PFLAGS_VISIBLE)==0 or band(inf.Flags, F.PFLAGS_REALNAMES)==0 then
      return false
    end
  end
  return true
end

return {
  CanRun = CanRun;
  CompareByHash = CompareByHash;
}
