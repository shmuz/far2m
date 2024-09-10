local Shared = ...
local Unquote = Shared.Unquote
local FullExpand = Shared.FullExpand

local F = far.Flags
local bor = bit64.bor

local function Redirect(command)
  local fp = io.popen(command)
  if fp then
    local fname = far.MkTemp()
    local fp2 = io.open(fname, "w")
    if fp2 then
      for line in fp:lines() do fp2:write(line,"\n") end
      fp2:close()
      fp:close()
      return fname
    end
    fp:close()
  end
end

local function Command(prefix, text)
  if prefix == "edit" then
    local redir, cmd = text:match("^(<?)%s*(.+)")
    if redir == nil then
      return
    end
    cmd = FullExpand(Unquote(cmd))
    local flags = bor(F.EF_NONMODAL, F.EF_IMMEDIATERETURN, F.EF_ENABLE_F6)
    if redir == "<" then
      local tmpname = Redirect(cmd)
      if tmpname then
        flags = bor(flags, F.EF_DELETEONLYFILEONCLOSE, F.EF_DISABLEHISTORY)
        editor.Editor(tmpname,nil,nil,nil,nil,nil,flags)
      end
    else
      local line,col,fname = regex.match(cmd, [=[ \[ (\d+)? (?: ,(\d+))? \] \s* (.+) ]=], nil, "x")
      line  = line or (col and 1) or nil
      col   = col or nil
      fname = far.SplitCmdLine(fname or cmd)
      editor.Editor(fname,nil,nil,nil,nil,nil,flags,line,col)
    end
  ----------------------------------------------------------------------------
  elseif prefix == "view" then
    local redir, cmd = text:match("^(<?)%s*(.+)")
    if redir == nil then
      return
    end
    cmd = FullExpand(Unquote(cmd))
    local flags = bor(F.VF_NONMODAL, F.VF_IMMEDIATERETURN, F.VF_ENABLE_F6)
    if redir == "<" then
      local tmpname = Redirect(cmd)
      if tmpname then
        flags = bor(flags, F.VF_DELETEONLYFILEONCLOSE, F.VF_DISABLEHISTORY)
        viewer.Viewer(tmpname,nil,nil,nil,nil,nil,flags)
      end
    else
      cmd = far.SplitCmdLine(cmd)
      viewer.Viewer(cmd,nil,nil,nil,nil,nil,flags)
    end
  ----------------------------------------------------------------------------
  elseif prefix == "load" then
    text = FullExpand(Unquote(text))
    if text ~= "" then
      far.LoadPlugin("PLT_PATH", text)
    end
  ----------------------------------------------------------------------------
  elseif prefix == "unload" then
    text = FullExpand(Unquote(text))
    if text ~= "" then
      local plug = far.FindPlugin("PFM_MODULENAME", text)
      if plug then far.UnloadPlugin(plug) end
    end
  ----------------------------------------------------------------------------
  elseif prefix == "goto" then
    text = FullExpand(Unquote(text))
    if text ~= "" then
      local path_ok = true
      text = text:gsub("\\(.)", "%1")
      local path,filename = text:match("(.*/)(.*)")
      if path == nil then
        filename = text
      else
        if path:sub(1,1) ~= "/" then
          path = win.JoinPath(panel.GetPanelDirectory(nil,1).Name, path)
        end
        path_ok = panel.SetPanelDirectory(nil,1,path)
      end
      if path_ok and filename ~= "" then
        local info = panel.GetPanelInfo(nil,1)
        if info then
          filename = filename:lower()
          for ii=1,info.ItemsNumber do
            local item = panel.GetPanelItem(nil,1,ii)
            if not item then break end
            if filename == item.FileName:lower() then
              panel.RedrawPanel(nil,1,{TopPanelItem=1,CurrentItem=ii})
              break
            end
          end
        end
      end
    end
  end
end

return {
  Command = Command;
}
