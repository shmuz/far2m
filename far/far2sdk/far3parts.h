#pragma once
#ifndef __FAR2SDK_FAR3PARTS_H__
#define __FAR2SDK_FAR3PARTS_H__

#include <windows.h>
#include "farcommon.h"

typedef DWORD COLORREF;

typedef uint32_t FAR3COLORFLAGS;
FAR_INLINE_CONSTANT FAR3COLORFLAGS
	FCF_FG_INDEX           = 0x00000001,
	FCF_BG_INDEX           = 0x00000002,
	FCF_FG_UNDERLINE_INDEX = 0x00000008,
	FCF_INDEXMASK          = 0x0000000B, // FCF_FG_INDEX | FCF_BG_INDEX | FCF_FG_UNDERLINE_INDEX

	// Legacy names, don't use
	FCF_FG_4BIT            = 0x00000001, // FCF_FG_INDEX
	FCF_BG_4BIT            = 0x00000002, // FCF_BG_INDEX
	FCF_4BITMASK           = 0x0000000B, // FCF_INDEXMASK

	FCF_INHERIT_STYLE      = 0x00000004,

	FCF_RAWATTR_MASK       = 0x0000FF00, // stored console attributes

	FCF_FG_BOLD            = 0x10000000,
	FCF_FG_ITALIC          = 0x20000000,
	FCF_FG_U_DATA0         = 0x40000000, // This is not a style flag, but a storage for one of 5 underline styles
	FCF_FG_U_DATA1         = 0x80000000, // This is not a style flag, but a storage for one of 5 underline styles
	FCF_FG_OVERLINE        = 0x01000000,
	FCF_FG_STRIKEOUT       = 0x02000000,
	FCF_FG_FAINT           = 0x04000000,
	FCF_FG_BLINK           = 0x08000000,
	FCF_INVERSE            = 0x00100000,
	FCF_FG_INVISIBLE       = 0x00200000,
	FCF_FG_U_DATA2         = 0x00400000, // This is not a style flag, but a storage for one of 5 underline styles

	FCF_FG_UNDERLINE_MASK  = 0xC0400000, // FCF_FG_U_DATA0 | FCF_FG_U_DATA1 | FCF_FG_U_DATA2,

	FCF_STYLE_MASK         = 0xFFF00000,

	FCF_NONE               = 0;

struct rgba
{
	unsigned char
		r,
		g,
		b,
		a;
};

struct color_index
{
	unsigned char
		i,
		reserved0,
		reserved1,
		a;
};

// #define INDEXMASK 0x000000ff
// #define COLORMASK 0x00ffffff
// #define ALPHAMASK 0xff000000

struct Far3Color
{
	FAR3COLORFLAGS Flags;
	union
	{
		COLORREF ForegroundColor;
		struct color_index ForegroundIndex;
		struct rgba ForegroundRGBA;
	}
#ifndef __cplusplus
	Foreground
#endif
	;
	union
	{
		COLORREF BackgroundColor;
		struct color_index BackgroundIndex;
		struct rgba BackgroundRGBA;
	}
#ifndef __cplusplus
	Background
#endif
	;
	union
	{
		COLORREF UnderlineColor;
		struct color_index UnderlineIndex;
		struct rgba UnderlineRGBA;
	}
#ifndef __cplusplus
	Underline
#endif
		;
	DWORD Reserved;
};

#endif // #ifndef __FAR2SDK_FAR3PARTS_H__
