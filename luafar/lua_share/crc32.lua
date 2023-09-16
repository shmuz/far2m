-- Taken from : https://github.com/luapower/crc32/blob/master/crc32.lua
-- Taken on   : 2023-06-19
-- Optimized  : bit functions made local

local ffi = require'ffi'
local bit = require'bit'
local band, bnot, bxor, rshift = bit.band, bit.bnot, bit.bxor, bit.rshift

local s_crc32 = ffi.new('const uint32_t[16]',
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c)

local function crc32(buf, sz, crc)
    crc = bnot(crc or 0)
    sz = tonumber(sz)
    for i = 0, sz-1 do
        crc = bxor(rshift(crc, 4), s_crc32[bxor(band(crc, 0xF), band(buf[i], 0xF))])
        crc = bxor(rshift(crc, 4), s_crc32[bxor(band(crc, 0xF), rshift(buf[i], 4))])
    end
    return bnot(crc)
end

return crc32
