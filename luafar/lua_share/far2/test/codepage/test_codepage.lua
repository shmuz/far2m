local function test(dir)
  local pass, total = 0, 0
  far.RecursiveSearch(dir, "*.txt",
    function(item, fullpath)
      if not item.FileAttributes:find("d") then
        local ref = item.FileName:match("^%d+")
        if ref and ref ~= "65000" then -- exclude UTF-7 from test
          total = total + 1
          local cp = far.DetectCodePage(fullpath)
          if cp == tonumber(ref) then pass = pass+1 end
        end
      end
    end)
  return pass, total
end

return test

