local sd = require "far2.simpledialog"

local F = far.Flags
local band, bor, lshift, rshift = bit64.band, bit64.bor, bit64.lshift, bit64.rshift
local bnot = function(arg) return band(bit64.bnot(arg), 0xFFFFFFFF) end

-- Стандартные цвета FAR Manager
local F_LIGHTGRAY    = F.FOREGROUND_BLUE + F.FOREGROUND_GREEN + F.FOREGROUND_RED
local F_WHITE        = F.FOREGROUND_BLUE + F.FOREGROUND_GREEN + F.FOREGROUND_RED + F.FOREGROUND_INTENSITY
local B_BLUE         = F.BACKGROUND_BLUE
local B_GREEN        = F.BACKGROUND_GREEN
local B_CYAN         = F.BACKGROUND_BLUE + F.BACKGROUND_GREEN
local B_RED          = F.BACKGROUND_RED
local B_MAGENTA      = F.BACKGROUND_BLUE + F.BACKGROUND_RED
local B_BROWN        = F.BACKGROUND_GREEN + F.BACKGROUND_RED
local B_LIGHTGRAY    = F.BACKGROUND_BLUE + F.BACKGROUND_GREEN + F.BACKGROUND_RED
local B_DARKGRAY     = F.BACKGROUND_INTENSITY
local B_LIGHTBLUE    = F.BACKGROUND_BLUE + F.BACKGROUND_INTENSITY
local B_LIGHTGREEN   = F.BACKGROUND_GREEN + F.BACKGROUND_INTENSITY
local B_LIGHTCYAN    = F.BACKGROUND_BLUE + F.BACKGROUND_GREEN + F.BACKGROUND_INTENSITY
local B_LIGHTRED     = F.BACKGROUND_RED + F.BACKGROUND_INTENSITY
local B_LIGHTMAGENTA = F.BACKGROUND_BLUE + F.BACKGROUND_RED + F.BACKGROUND_INTENSITY
local B_YELLOW       = F.BACKGROUND_GREEN + F.BACKGROUND_RED + F.BACKGROUND_INTENSITY
local B_WHITE        = F.BACKGROUND_BLUE + F.BACKGROUND_GREEN + F.BACKGROUND_RED + F.BACKGROUND_INTENSITY
local F_MASK         = F_WHITE
local B_MASK         = B_WHITE
----------------------------------------------------------------------------------------------------

local ColorDialogForeRGB, ColorDialogBackRGB

local function ReverseColorBytes(Color)
  return bor( band(Color,0x00ff00), band(rshift(Color,16), 0xff), lshift(band(Color,0xff), 16) )
end

local function ColorDialogForeRGBValue()
  return ReverseColorBytes(tonumber(ColorDialogForeRGB, 16) or 0)
end

local function ColorDialogBackRGBValue()
  return ReverseColorBytes(tonumber(ColorDialogBackRGB, 16) or 0)
end

local function SetByMask(Trg, Src, Mask)
  return bor(band(Trg,bnot(Mask)), band(Src,Mask))
end

local function EqualByMask(A1, A2, Mask)
  return band(A1,Mask) == band(A2,Mask)
end

local function GetColorDialog(aColor)
  aColor = aColor or 0x0F
  ColorDialogForeRGB = ("%06X"):format( ReverseColorBytes(band(rshift(aColor,16), 0xffffff)) )
  ColorDialogBackRGB = ("%06X"):format( ReverseColorBytes(band(rshift(aColor,40), 0xffffff)) )

  local FRB = bor(F.DIF_SETCOLOR, F.DIF_MOVESELECT) -- DIF_SETCOLOR is ignored by current far2l / far2m
  local TextSample = ("Text "):rep(7)

  local Items = {
    width=0;
    --[[   1 ]] {tp="dbox",   text="Color"},

    --[[   2 ]] {tp="sbox",  x1= 5, y1=2, x2=20, y2=8, text="Foreground"},
    --[[   3 ]] {tp="rbutt", x1= 7, y1=3, flags=FRB+F_LIGHTGRAY+F.DIF_GROUP, name="fore"},
    --[[   4 ]] {tp="rbutt", x1="", y1=4, flags=FRB+B_RED},
    --[[   5 ]] {tp="rbutt", x1="", y1=5, flags=FRB+F_LIGHTGRAY+B_DARKGRAY},
    --[[   6 ]] {tp="rbutt", x1="", y1=6, flags=FRB+B_LIGHTRED},
    --[[   7 ]] {tp="rbutt", x1=10, y1=3, flags=FRB+F_LIGHTGRAY+B_BLUE},
    --[[   8 ]] {tp="rbutt", x1="", y1=4, flags=FRB+B_MAGENTA},
    --[[   9 ]] {tp="rbutt", x1="", y1=5, flags=FRB+B_LIGHTBLUE},
    --[[  10 ]] {tp="rbutt", x1="", y1=6, flags=FRB+B_LIGHTMAGENTA},
    --[[  11 ]] {tp="rbutt", x1=13, y1=3, flags=FRB+B_GREEN},
    --[[  12 ]] {tp="rbutt", x1="", y1=4, flags=FRB+B_BROWN},
    --[[  13 ]] {tp="rbutt", x1="", y1=5, flags=FRB+B_LIGHTGREEN},
    --[[  14 ]] {tp="rbutt", x1="", y1=6, flags=FRB+B_YELLOW},
    --[[  15 ]] {tp="rbutt", x1=16, y1=3, flags=FRB+B_CYAN},
    --[[  16 ]] {tp="rbutt", x1="", y1=4, flags=FRB+B_LIGHTGRAY},
    --[[  17 ]] {tp="rbutt", x1="", y1=5, flags=FRB+B_LIGHTCYAN},
    --[[  18 ]] {tp="rbutt", x1="", y1=6, flags=FRB+B_WHITE},

    --[[  19 ]] {tp="sbox",  x1=22, y1=2, x2=37, y2=8, text="Background"},
    --[[  20 ]] {tp="rbutt", x1=24, y1=3, flags=FRB+F_LIGHTGRAY+F.DIF_GROUP, name="back"},
    --[[  21 ]] {tp="rbutt", x1="", y1=4, flags=FRB+B_RED},
    --[[  22 ]] {tp="rbutt", x1="", y1=5, flags=FRB+F_LIGHTGRAY+B_DARKGRAY},
    --[[  23 ]] {tp="rbutt", x1="", y1=6, flags=FRB+B_LIGHTRED},
    --[[  24 ]] {tp="rbutt", x1=27, y1=3, flags=FRB+F_LIGHTGRAY+B_BLUE},
    --[[  25 ]] {tp="rbutt", x1="", y1=4, flags=FRB+B_MAGENTA},
    --[[  26 ]] {tp="rbutt", x1="", y1=5, flags=FRB+B_LIGHTBLUE},
    --[[  27 ]] {tp="rbutt", x1="", y1=6, flags=FRB+B_LIGHTMAGENTA},
    --[[  28 ]] {tp="rbutt", x1=30, y1=3, flags=FRB+B_GREEN},
    --[[  29 ]] {tp="rbutt", x1="", y1=4, flags=FRB+B_BROWN},
    --[[  30 ]] {tp="rbutt", x1="", y1=5, flags=FRB+B_LIGHTGREEN},
    --[[  31 ]] {tp="rbutt", x1="", y1=6, flags=FRB+B_YELLOW},
    --[[  32 ]] {tp="rbutt", x1=33, y1=3, flags=FRB+B_CYAN},
    --[[  33 ]] {tp="rbutt", x1="", y1=4, flags=FRB+B_LIGHTGRAY},
    --[[  34 ]] {tp="rbutt", x1="", y1=5, flags=FRB+B_LIGHTCYAN},
    --[[  35 ]] {tp="rbutt", x1="", y1=6, flags=FRB+B_WHITE},

    --[[  36 ]] {tp="text";    x1=7;         width=5; text="RGB#:"},
    --[[  37 ]] {tp="fixedit"; x1=13; y1=""; width=6; mask="HHHHHH"; text=ColorDialogForeRGB, name="rgbFore"},
    --[[  38 ]] {tp="text";    x1=24; y1=""; width=5; text="RGB#:"},
    --[[  39 ]] {tp="fixedit"; x1=30; y1=""; width=6; mask="HHHHHH"; text=ColorDialogBackRGB, name="rgbBack"},

    --[[  40 ]] {tp="chbox", x1= 5, y1=9,  text="RGB Foregr."; name="rgbFore_Enb"; },
    --[[  41 ]] {tp="chbox", x1=22, y1="", text="RGB Backgr."; name="rgbBack_Enb"; },

    --[[  42 ]] {tp="text",  x1=5,  x2=37, flags=F.DIF_SETCOLOR, text=TextSample, ystep=2, name="sample"},
    --[[  43 ]] {tp="text",  x1="", x2="", flags=F.DIF_SETCOLOR, text=TextSample},
    --[[  44 ]] {tp="text",  x1="", x2="", flags=F.DIF_SETCOLOR, text=TextSample},
    --[[  45 ]] {tp="sep"},
    --[[  46 ]] {tp="butt",  default=1; centergroup=1; text="Set"},
    --[[  47 ]] {tp="butt",  cancel=1; centergroup=1; text="Cancel"},
  }

  local Dlg = sd.New(Items)
  local Pos, Elem = Dlg:Indexes()

  local CurColor = band(aColor, 0xFFFF)

  for i = Pos.fore, Pos.fore+15 do -- Foreground
    if EqualByMask(rshift(Items[i].flags,4), aColor, F_MASK) then
      Items[i].val=1; Items[i].focus=1; break
    end
  end

  for i = Pos.back, Pos.back+15 do -- Background
    if EqualByMask(Items[i].flags, aColor, B_MASK) then
      Items[i].val=1; break
    end
  end

  Elem.rgbFore_Enb.val = (0 ~= band(aColor, 0x0100))
  Elem.rgbBack_Enb.val = (0 ~= band(aColor, 0x0200))

  for i = Pos.sample, Pos.sample+2 do -- TextSample
    Items[i].flags = SetByMask(Items[i].flags, CurColor, F.DIF_COLORMASK)
  end

  local function UpdateRGBFromDialog(hDlg)
    ColorDialogForeRGB = hDlg:GetText(Pos.rgbFore)
    ColorDialogBackRGB = hDlg:GetText(Pos.rgbBack)
  end

  local function OnDrawn(hDlg)
    UpdateRGBFromDialog(hDlg)

    -- Trick to fix #1392:
    -- For foreground-colored boxes invert Fg&Bg colors and add COMMON_LVB_REVERSE_VIDEO attribute
    -- this will put real colors on them if mapping of colors is different for Fg and Bg indexes

    -- Ensure everything is on screen and will use console API then
    far.Text() -- original: ScrBuf.Flush()

    local DlgRect = hDlg:GetDlgRect()

    for ID = Pos.fore, Pos.fore+15 do
      local ItemRect = hDlg:GetItemPosition(ID)
      if ItemRect then
        ItemRect.Left   = ItemRect.Left   + DlgRect.Left
        ItemRect.Right  = ItemRect.Right  + DlgRect.Left
        ItemRect.Top    = ItemRect.Top    + DlgRect.Top
        ItemRect.Bottom = ItemRect.Bottom + DlgRect.Top
        for x = ItemRect.Left, ItemRect.Right do
          for y = ItemRect.Top, ItemRect.Bottom do
            win.EnsureColorsAreInverted(x, y)
          end
        end
      end
    end
  end

  Items.proc = function(hDlg, Msg, Par1, Par2)
    if Msg == F.DN_CTLCOLORDLGITEM then
      if Par1 >= Pos.fore and Par1 < Pos.fore+16 or Par1 >= Pos.back and Par1 < Pos.back+16 then
        Par2[1] = band(Items[Par1].flags, F.DIF_COLORMASK)
        return Par2
      elseif Par1 == Pos.sample then
        Par2[1] = band(CurColor, 0xFF)
        return Par2
      elseif Par1 >= Pos.sample+1 and Par1 <= Pos.sample+2 then
        Par2[1] = bor(
          lshift(ColorDialogForeRGBValue(), 16),
          lshift(ColorDialogBackRGBValue(), 40),
          0x0300 -- enable "truecolor" background and foreground
        )
        return Par2
      end

    elseif Msg == F.DN_BTNCLICK then
      if Par1 >= Pos.fore and Par1 <= Pos.back+15 then
        local DlgItem = hDlg:GetDlgItem(Par1)
        if Par1 <= Pos.fore+15 then   -- Fore
          CurColor = SetByMask(CurColor, rshift(DlgItem[9],4), F_MASK)
        elseif Par1 >= Pos.back then  -- Back
          CurColor = SetByMask(CurColor, DlgItem[9], B_MASK)
        end
      end

    elseif Msg == F.DN_DRAWDIALOGDONE then
      OnDrawn(hDlg)

    elseif Msg == F.DN_EDITCHANGE then
      if Par1 == Pos.rgbFore or Par1 == Pos.rgbBack then
        OnDrawn(hDlg)
      end

    elseif Msg == F.DN_CLOSE then
      UpdateRGBFromDialog(hDlg)

    end
  end

  local out = Dlg:Run()
  if out then
    local Color = band(CurColor, 0xFCFF)

    if out.rgbFore_Enb then
      Color = bor(Color, 0x0100, lshift(ColorDialogForeRGBValue(), 16))
    end

    if out.rgbBack_Enb then
      Color = bor(Color, 0x0200, lshift(ColorDialogBackRGBValue(), 40))
    end

    return Color
  end
end

return GetColorDialog
