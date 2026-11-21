-- record_cast.lua — asciinema v2 (.cast) session recorder module for far2m

local FPS = 20 -- Default capture rate (20 FPS ≈ 1% CPU at 0.5 ms/frame)

local bit_band = bit and bit.band or function(a, b) return a % (b * 2) >= b and b or 0 end

-- Escape a raw ANSI byte string into a valid JSON string literal body
local json_map = {
  ['"'] = '\\"', ['\\'] = '\\\\', ['\b'] = '\\b',
  ['\f'] = '\\f', ['\n'] = '\\n', ['\r'] = '\\r', ['\t'] = '\\t',
}
local function json_escape(s)
  return (string.gsub(s, '[%z\1-\31\\"]', function(c)
    return json_map[c] or string.format("\\u%04x", string.byte(c))
  end))
end

-- Classic Windows Console / far2m GUI 16-color RGB palette (indexed 0..15 by I|R|G|B)
local VGA_PALETTE = {
  [0]  = {   0,   0,   0 }, -- Black
  [1]  = {   0,   0, 128 }, -- Dark Blue
  [2]  = {   0, 128,   0 }, -- Dark Green
  [3]  = {   0, 128, 128 }, -- Dark Cyan
  [4]  = { 128,   0,   0 }, -- Dark Red
  [5]  = { 128,   0, 128 }, -- Dark Magenta
  [6]  = { 128, 128,   0 }, -- Brown / Dark Yellow
  [7]  = { 192, 192, 192 }, -- Light Gray
  [8]  = { 128, 128, 128 }, -- Dark Gray
  [9]  = {   0,   0, 255 }, -- Bright Blue
  [10] = {   0, 255,   0 }, -- Bright Green
  [11] = {   0, 255, 255 }, -- Bright Cyan
  [12] = { 255,   0,   0 }, -- Bright Red
  [13] = { 255,   0, 255 }, -- Bright Magenta
  [14] = { 255, 255,   0 }, -- Yellow
  [15] = { 255, 255, 255 }, -- White
}

local F = far.Flags or {}
local FOREGROUND_TRUECOLOR = F.FOREGROUND_TRUECOLOR or 0x0100
local BACKGROUND_TRUECOLOR = F.BACKGROUND_TRUECOLOR or 0x0200

local function unpack_rgb(rgb)
  return bit_band(rgb, 0xFF),
         bit_band(math.floor(rgb / 0x100), 0xFF),
         bit_band(math.floor(rgb / 0x10000), 0xFF)
end

-- Convert a WinPort 64-bit CHAR_INFO attribute into an exact 24-bit RGB ANSI SGR sequence
local sgr_cache = {}
local function attr_to_sgr(attr)
  local key = tostring(attr)
  local cached = sgr_cache[key]
  if cached then return cached end

  local low, fg_rgb, bg_rgb
  if type(attr) == "number" then
    low = attr
    fg_rgb, bg_rgb = 0, 0
  else
    low = tonumber(bit64.band(attr, 0xFFFF))
    fg_rgb = tonumber(bit64.band(bit64.rshift(attr, 16), 0xFFFFFF))
    bg_rgb = tonumber(bit64.band(bit64.rshift(attr, 40), 0xFFFFFF))
  end

  local fr, fg, fb
  if bit_band(low, FOREGROUND_TRUECOLOR) ~= 0 then
    fr, fg, fb = unpack_rgb(fg_rgb)
  else
    local c = VGA_PALETTE[bit_band(low, 0x0F)]
    fr, fg, fb = c[1], c[2], c[3]
  end

  local br, bg, bb
  if bit_band(low, BACKGROUND_TRUECOLOR) ~= 0 then
    br, bg, bb = unpack_rgb(bg_rgb)
  else
    local c = VGA_PALETTE[bit_band(math.floor(low / 0x10), 0x0F)]
    br, bg, bb = c[1], c[2], c[3]
  end

  local extra = ""
  if bit_band(low, 0x8000) ~= 0 then extra = extra .. ";4" end -- COMMON_LVB_UNDERSCORE
  if bit_band(low, 0x4000) ~= 0 then extra = extra .. ";7" end -- COMMON_LVB_REVERSE_VIDEO

  local sgr = string.format("\27[0;38;2;%d;%d;%d;48;2;%d;%d;%d%sm",
                            fr, fg, fb, br, bg, bb, extra)
  sgr_cache[key] = sgr
  return sgr
end

-- Split a UTF-8 row string into W screen cells (attaching combining marks to their base cell)
local function split_cells(str, width)
  local cells, n = {}, 0
  for ch in string.gmatch(str, "[%z\1-\127\194-\244][\128-\191]*") do
    local b1, b2 = string.byte(ch, 1, 2)
    -- Attach common UTF-8 combining diacritical marks (U+0300..U+036F -> 0xCC 0x80..0xCD 0xAF)
    if n > 0 and (b1 == 0xCC or (b1 == 0xCD and b2 <= 0xAF)) then
      cells[n] = cells[n] .. ch
    else
      n = n + 1
      cells[n] = ch
    end
  end
  for i = n + 1, width do
    cells[i] = " "
  end
  return cells
end

local function get_size()
  local r = far.AdvControl("ACTL_GETFARRECT")
  return r.Right - r.Left + 1, r.Bottom - r.Top + 1
end

--- Start recording the current far2m session to an asciinema v2 (.cast) file.
--- @param opts table|nil Optional settings table:
---   - fps (number, default 20): Capture rate in frames per second.
---   - path (string, default "$TMPDIR/far2m_YYYYMMDD_HHMMSS.cast"): Destination .cast file path.
---   - silent (boolean, default false): If true, suppress the "Recording started" message box.
--- @return string|nil out_path Path to the active .cast file, or nil if already recording.
--- @return string|nil err Error reason when returning nil.
local function start(opts)
  if _G._far2m_cast_recorder then
    return nil, "already recording"
  end

  opts = opts or {}
  local fps = opts.fps or FPS
  local out_path = opts.path
    or ((win.GetEnv("TMPDIR") or "/tmp") .. string.format("/far2m_%s.cast", os.date("%Y%m%d_%H%M%S")))
  local fp = assert(io.open(out_path, "w"))

  local W, H = get_size()
  local t0 = win.Clock() -- Monotonic seconds with fractional part

  -- Write asciicast v2 header
  fp:write(string.format('{"version":2,"width":%d,"height":%d,"timestamp":%d,"env":{"TERM":"xterm-256color"}}\n',
    W, H, os.time()))

  local prev_rows, prev_cells, prev_attrs
  local prev_cx, prev_cy

  local timer = far.Timer(math.floor(1000 / fps), function()
    local cur_w, cur_h = get_size()
    local dt = win.Clock() - t0

    -- Emit an asciicast resize ("r") event if the console dimensions changed
    if cur_w ~= W or cur_h ~= H then
      W, H = cur_w, cur_h
      prev_rows, prev_cells, prev_attrs = nil, nil, nil
      fp:write(string.format('[%.6f,"r","%dx%d"]\n', dt, W, H))
    end

    local rows, attrs = far.GetScreenRect(1, 1, W, H)
    if not rows then return end

    local out = {}
    local cur_cells = {}
    local active_attr = nil
    local term_x, term_y = nil, nil
    local any_cell_changed = false

    for y = 1, H do
      local row_str = rows[y]
      local row_offset = (y - 1) * W

      -- Fast skip: check whether the row text or any cell attribute in this row changed
      local row_dirty = (not prev_rows) or (row_str ~= prev_rows[y])
      if not row_dirty then
        for x = 1, W do
          if attrs[row_offset + x] ~= prev_attrs[row_offset + x] then
            row_dirty = true
            break
          end
        end
      end

      if not row_dirty then
        cur_cells[y] = prev_cells[y]
      else
        local cells = split_cells(row_str, W)
        cur_cells[y] = cells
        local pcells = prev_cells and prev_cells[y]

        for x = 1, W do
          local idx = row_offset + x
          local ch, attr = cells[x], attrs[idx]
          if (not pcells) or ch ~= pcells[x] or attr ~= prev_attrs[idx] then
            if not any_cell_changed then
              out[#out + 1] = "\27[?25l" -- Hide cursor during frame update
              any_cell_changed = true
            end
            -- Only emit cursor movement when not advancing sequentially from the previous cell
            if term_y ~= y or term_x ~= x then
              out[#out + 1] = string.format("\27[%d;%dH", y, x)
            end
            if attr ~= active_attr then
              out[#out + 1] = attr_to_sgr(attr)
              active_attr = attr
            end
            out[#out + 1] = ch
            term_x, term_y = x + 1, y
          end
        end
      end
    end

    -- Track live console cursor position
    local csbi = win.GetConsoleScreenBufferInfo()
    local cx = csbi and (csbi.CursorPositionX + 1) or 1
    local cy = csbi and (csbi.CursorPositionY + 1) or 1

    if any_cell_changed or cx ~= prev_cx or cy ~= prev_cy then
      out[#out + 1] = string.format("\27[%d;%dH\27[?25h", cy, cx)
    end

    if #out > 0 then
      fp:write(string.format('[%.6f,"o","%s"]\n', dt, json_escape(table.concat(out))))
    end

    prev_rows, prev_cells, prev_attrs = rows, cur_cells, attrs
    prev_cx, prev_cy = cx, cy
  end)

  _G._far2m_cast_recorder = {
    path = out_path,
    stop = function(silent)
      timer:Close()
      fp:close()
      _G._far2m_cast_recorder = nil
      if not silent then
        far.Message("Saved recording to:\n" .. out_path, "asciinema recorder")
      end
      return out_path
    end,
  }

  if not opts.silent then
    far.Message("Recording started:\n" .. out_path, "asciinema recorder")
  end

  return out_path
end

--- Stop the active recording and finalize the .cast file.
--- @param opts table|nil Optional settings table:
---   - silent (boolean, default false): If true, suppress the "Saved recording" message box.
--- @return string|nil out_path Path to the saved .cast file, or nil if not recording.
local function stop(opts)
  local rec = _G._far2m_cast_recorder
  if rec then
    return rec.stop(opts and opts.silent)
  end
  return nil
end

--- Check whether a session recording is currently in progress.
--- @return boolean
local function recording()
  return _G._far2m_cast_recorder ~= nil
end

return {
  start = start,
  stop = stop,
  recording = recording,
}
