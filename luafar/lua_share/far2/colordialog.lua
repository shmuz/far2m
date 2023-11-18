-- luacheck: ignore 211 (unused variable)

local sd = require "far2.simpledialog"

local F = far.Flags
local band, bor, lshift, rshift = bit64.band, bit64.bor, bit64.lshift, bit64.rshift
local bnot = function(arg) return band(bit64.bnot(arg), 0xFFFFFFFF) end

-- Стандартные цвета FAR Manager
local F_BLACK        = 0
local F_BLUE         = F.FOREGROUND_BLUE
local F_GREEN        = F.FOREGROUND_GREEN
local F_CYAN         = F.FOREGROUND_BLUE + F.FOREGROUND_GREEN
local F_RED          = F.FOREGROUND_RED
local F_MAGENTA      = F.FOREGROUND_BLUE + F.FOREGROUND_RED
local F_BROWN        = F.FOREGROUND_GREEN + F.FOREGROUND_RED
local F_LIGHTGRAY    = F.FOREGROUND_BLUE + F.FOREGROUND_GREEN + F.FOREGROUND_RED
local F_DARKGRAY     = F.FOREGROUND_INTENSITY
local F_LIGHTBLUE    = F.FOREGROUND_BLUE + F.FOREGROUND_INTENSITY
local F_LIGHTGREEN   = F.FOREGROUND_GREEN + F.FOREGROUND_INTENSITY
local F_LIGHTCYAN    = F.FOREGROUND_BLUE + F.FOREGROUND_GREEN + F.FOREGROUND_INTENSITY
local F_LIGHTRED     = F.FOREGROUND_RED + F.FOREGROUND_INTENSITY
local F_LIGHTMAGENTA = F.FOREGROUND_BLUE + F.FOREGROUND_RED + F.FOREGROUND_INTENSITY
local F_YELLOW       = F.FOREGROUND_GREEN + F.FOREGROUND_RED + F.FOREGROUND_INTENSITY
local F_WHITE        = F.FOREGROUND_BLUE + F.FOREGROUND_GREEN + F.FOREGROUND_RED + F.FOREGROUND_INTENSITY
local B_BLACK        = 0
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
local function FarTrueColorFromRGB(rgb, used)
  return {
    Flags = used or (used==nil and rgb~=0) and 1 or 0;
    R = band(rgb, 0xff);
    G = band(rshift(rgb, 8), 0xff);
    B = band(rshift(rgb, 16), 0xff);
  }
end
----------------------------------------------------------------------------------------------------

local ColorDialogForeRGB, ColorDialogBackRGB

local function ReverseColorBytes(Color)
  return bor( band(Color,0x00ff00), band(rshift(Color,16), 0xff), lshift(band(Color,0xff), 16) )
end

local function ColorDialogForeRGBValue()
  return ReverseColorBytes(tonumber(ColorDialogForeRGB, 16))
end

local function ColorDialogBackRGBValue()
  return ReverseColorBytes(tonumber(ColorDialogBackRGB, 16))
end

local function ColorDialogForeRGBMask()
  return lshift(ColorDialogForeRGBValue(), 16)
end

local function ColorDialogBackRGBMask()
  return lshift(ColorDialogBackRGBValue(), 40)
end

local function GetColorDialog(aColor)
  ColorDialogForeRGB = ("%06X"):format( ReverseColorBytes(band(rshift(aColor,16), 0xffffff)) )
  ColorDialogBackRGB = ("%06X"):format( ReverseColorBytes(band(rshift(aColor,40), 0xffffff)) )

  aColor = aColor or 0x0F
  local FRB = bor(F.DIF_SETCOLOR, F.DIF_MOVESELECT)
  local TextSample = ("Text "):rep(7)

  local Items = {
    data=aColor;
    width=0;
    --[[   1 ]] {tp="dbox",   text="Color"},

    --[[   2 ]] {tp="sbox",  x1= 5, y1=2, x2=20, y2=8, text="Foreground"},
    --[[   3 ]] {tp="rbutt", x1= 7, y1=3, flags=FRB+F_LIGHTGRAY+B_BLACK+F.DIF_GROUP, name="fore"},
    --[[   4 ]] {tp="rbutt", x1="", y1=4, flags=FRB+F_BLACK+B_RED},
    --[[   5 ]] {tp="rbutt", x1="", y1=5, flags=FRB+F_LIGHTGRAY+B_DARKGRAY},
    --[[   6 ]] {tp="rbutt", x1="", y1=6, flags=FRB+F_BLACK+B_LIGHTRED},
    --[[   7 ]] {tp="rbutt", x1=10, y1=3, flags=FRB+F_LIGHTGRAY+B_BLUE},
    --[[   8 ]] {tp="rbutt", x1="", y1=4, flags=FRB+F_BLACK+B_MAGENTA},
    --[[   9 ]] {tp="rbutt", x1="", y1=5, flags=FRB+F_BLACK+B_LIGHTBLUE},
    --[[  10 ]] {tp="rbutt", x1="", y1=6, flags=FRB+F_BLACK+B_LIGHTMAGENTA},
    --[[  11 ]] {tp="rbutt", x1=13, y1=3, flags=FRB+F_BLACK+B_GREEN},
    --[[  12 ]] {tp="rbutt", x1="", y1=4, flags=FRB+F_BLACK+B_BROWN},
    --[[  13 ]] {tp="rbutt", x1="", y1=5, flags=FRB+F_BLACK+B_LIGHTGREEN},
    --[[  14 ]] {tp="rbutt", x1="", y1=6, flags=FRB+F_BLACK+B_YELLOW},
    --[[  15 ]] {tp="rbutt", x1=16, y1=3, flags=FRB+F_BLACK+B_CYAN},
    --[[  16 ]] {tp="rbutt", x1="", y1=4, flags=FRB+F_BLACK+B_LIGHTGRAY},
    --[[  17 ]] {tp="rbutt", x1="", y1=5, flags=FRB+F_BLACK+B_LIGHTCYAN},
    --[[  18 ]] {tp="rbutt", x1="", y1=6, flags=FRB+F_BLACK+B_WHITE},

    --[[  19 ]] {tp="sbox",  x1=22, y1=2, x2=37, y2=8, text="Background"},
    --[[  20 ]] {tp="rbutt", x1=24, y1=3, flags=FRB+F_LIGHTGRAY+B_BLACK+F.DIF_GROUP, name="back"},
    --[[  21 ]] {tp="rbutt", x1="", y1=4, flags=FRB+F_BLACK+B_RED},
    --[[  22 ]] {tp="rbutt", x1="", y1=5, flags=FRB+F_LIGHTGRAY+B_DARKGRAY},
    --[[  23 ]] {tp="rbutt", x1="", y1=6, flags=FRB+F_BLACK+B_LIGHTRED},
    --[[  24 ]] {tp="rbutt", x1=27, y1=3, flags=FRB+F_LIGHTGRAY+B_BLUE},
    --[[  25 ]] {tp="rbutt", x1="", y1=4, flags=FRB+F_BLACK+B_MAGENTA},
    --[[  26 ]] {tp="rbutt", x1="", y1=5, flags=FRB+F_BLACK+B_LIGHTBLUE},
    --[[  27 ]] {tp="rbutt", x1="", y1=6, flags=FRB+F_BLACK+B_LIGHTMAGENTA},
    --[[  28 ]] {tp="rbutt", x1=30, y1=3, flags=FRB+F_BLACK+B_GREEN},
    --[[  29 ]] {tp="rbutt", x1="", y1=4, flags=FRB+F_BLACK+B_BROWN},
    --[[  30 ]] {tp="rbutt", x1="", y1=5, flags=FRB+F_BLACK+B_LIGHTGREEN},
    --[[  31 ]] {tp="rbutt", x1="", y1=6, flags=FRB+F_BLACK+B_YELLOW},
    --[[  32 ]] {tp="rbutt", x1=33, y1=3, flags=FRB+F_BLACK+B_CYAN},
    --[[  33 ]] {tp="rbutt", x1="", y1=4, flags=FRB+F_BLACK+B_LIGHTGRAY},
    --[[  34 ]] {tp="rbutt", x1="", y1=5, flags=FRB+F_BLACK+B_LIGHTCYAN},
    --[[  35 ]] {tp="rbutt", x1="", y1=6, flags=FRB+F_BLACK+B_WHITE},

    --[[  36 ]] {tp="text";    x1=7;         width=5; text="RGB#:"},
    --[[  37 ]] {tp="fixedit"; x1=13; y1=""; width=6; mask="HHHHHH"; text=ColorDialogForeRGB, name="rgbFore"},
    --[[  38 ]] {tp="text";    x1=24; y1=""; width=5; text="RGB#:"},
    --[[  39 ]] {tp="fixedit"; x1=30; y1=""; width=6; mask="HHHHHH"; text=ColorDialogBackRGB, name="rgbBack"},

    --[[  40 ]] {tp="chbox", x1= 5, y1=9,  text="&Transparent"; name="transpFore"; },
    --[[  41 ]] {tp="chbox", x1=22, y1="", text="T&ransparent"; name="transpBack"; },

    --[[  42 ]] {tp="text",  x1=5,  x2=37, flags=F.DIF_SETCOLOR, text=TextSample, ystep=2, name="sample"},
    --[[  43 ]] {tp="text",  x1="", x2="", flags=F.DIF_SETCOLOR, text=TextSample},
    --[[  44 ]] {tp="text",  x1="", x2="", flags=F.DIF_SETCOLOR, text=TextSample},
    --[[  45 ]] {tp="sep"},
    --[[  46 ]] {tp="butt",  default=1; centergroup=1; text="Set"},
    --[[  47 ]] {tp="butt",  cancel=1; centergroup=1; text="Cancel"},
  }

  local Dlg = sd.New(Items)
  local Pos = Dlg:Indexes()

  local CurColor = band(aColor, 0xFFFF)

  for i = Pos.fore, Pos.fore+15 do -- Foreground
    if rshift(band(Items[i].flags, B_MASK), 4) == band(aColor, F_MASK) then
      Items[i].val=1; Items[i].focus=1; break
    end
  end

  for i = Pos.back, Pos.back+15 do -- Background
    if band(Items[i].flags, B_MASK) == band(aColor, B_MASK) then
      Items[i].val=1; break
    end
  end

  for i = Pos.sample, Pos.sample+2 do -- TextSample
    Items[i].flags = bor(band(Items[i].flags, bnot(F.DIF_COLORMASK)), CurColor)
  end

  local function UpdateRGBFromDialog(hDlg)
    ColorDialogForeRGB = hDlg:send(F.DM_GETTEXT, Pos.rgbFore)
    ColorDialogBackRGB = hDlg:send(F.DM_GETTEXT, Pos.rgbBack)
  end

  local function OnDrawn(hDlg)
    UpdateRGBFromDialog(hDlg)

    -- Trick to fix #1392:
    -- For foreground-colored boxes invert Fg&Bg colors and add COMMON_LVB_REVERSE_VIDEO attribute
    -- this will put real colors on them if mapping of colors is different for Fg and Bg indexes

    -- Ensure everything is on screen and will use console API then
    far.Text() -- original: ScrBuf.Flush()

    local DlgRect = hDlg:send(F.DM_GETDLGRECT)

    for ID = Pos.fore, Pos.fore+15 do
      local ItemRect = hDlg:send(F.DM_GETITEMPOSITION, ID)
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

  local function CtlColorDlgItem(hDlg, ID, UseTrueColor)
    if UseTrueColor then
      local ditc = {
        Normal = {
          Fore = FarTrueColorFromRGB(ColorDialogForeRGBValue());
          Back = FarTrueColorFromRGB(ColorDialogBackRGBValue());
        }}
      hDlg:send(F.DM_SETTRUECOLOR, ID, ditc);
    end

    local Color = hDlg:send(F.DM_GETDLGDATA)
    return band(Color,0xFF)
  end

  Items.proc = function(hDlg, Msg, Par1, Par2)
    if Msg == F.DN_CTLCOLORDLGITEM then
      if Par1 >= Pos.sample and Par1 <= Pos.sample+2 then
        Par2[1] = CtlColorDlgItem(hDlg, Par1, Par1 >= Pos.sample+1)
        return Par2
      end

    elseif Msg == F.DN_BTNCLICK then
      if Par1 >= Pos.fore and Par1 <= Pos.back+15 then
        local color = hDlg:send(F.DM_GETDLGDATA)
        local DlgItem = hDlg:send(F.DM_GETDLGITEM, Par1)
        local NewColor = color

        if Par1 <= Pos.fore+15 then   -- Fore
          NewColor = band(NewColor, bnot(0x0F))
          NewColor = bor(NewColor, rshift(band(DlgItem[9],B_MASK), 4))
        elseif Par1 >= Pos.back then  -- Back
          NewColor = band(NewColor, bnot(0xF0))
          NewColor = bor(NewColor, band(DlgItem[9],B_MASK))
        end

        if NewColor ~= CurColor then
          CurColor = NewColor
          hDlg:send(F.DM_SETDLGDATA, CurColor)
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
    local Color = band(CurColor, 0xffff)

    if out.transpFore then
      Color = bor(Color, 0x0F00)
    else
      Color = band(Color, 0xF0FF)
    end

    if out.transpBack then
      Color = bor(Color, 0xF000)
    else
      Color = band(Color, 0x0FFF)
    end

    Color = bor(Color, ColorDialogForeRGBMask())
    Color = bor(Color, ColorDialogBackRGBMask())

    return Color
  end
end

return GetColorDialog
