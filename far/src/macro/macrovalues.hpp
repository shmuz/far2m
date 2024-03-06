#ifndef MACROVALUES_HPP
#define MACROVALUES_HPP

// This file defines values used by both Far and plugin LuaMacro

typedef unsigned int MACROFLAGS_MFLAGS;
static const MACROFLAGS_MFLAGS
	MFLAGS_NONE                    = 0,
	// public flags, read from/saved to config
	MFLAGS_ENABLEOUTPUT            = (1 <<  0), // не подавлять обновление экрана во время выполнения макроса
	MFLAGS_NOSENDKEYSTOPLUGINS     = (1 <<  1), // НЕ передавать плагинам клавиши во время записи/воспроизведения макроса
	MFLAGS_RUNAFTERFARSTART        = (1 <<  3), // этот макрос запускается при старте ФАРа
	MFLAGS_EMPTYCOMMANDLINE        = (1 <<  4), // запускать, если командная линия пуста
	MFLAGS_NOTEMPTYCOMMANDLINE     = (1 <<  5), // запускать, если командная линия не пуста
	MFLAGS_EDITSELECTION           = (1 <<  6), // запускать, если есть выделение в редакторе
	MFLAGS_EDITNOSELECTION         = (1 <<  7), // запускать, если есть нет выделения в редакторе
	MFLAGS_SELECTION               = (1 <<  8), // активная:  запускать, если есть выделение
	MFLAGS_PSELECTION              = (1 <<  9), // пассивная: запускать, если есть выделение
	MFLAGS_NOSELECTION             = (1 << 10), // активная:  запускать, если есть нет выделения
	MFLAGS_PNOSELECTION            = (1 << 11), // пассивная: запускать, если есть нет выделения
	MFLAGS_NOFILEPANELS            = (1 << 12), // активная:  запускать, если это плагиновая панель
	MFLAGS_PNOFILEPANELS           = (1 << 13), // пассивная: запускать, если это плагиновая панель
	MFLAGS_NOPLUGINPANELS          = (1 << 14), // активная:  запускать, если это файловая панель
	MFLAGS_PNOPLUGINPANELS         = (1 << 15), // пассивная: запускать, если это файловая панель
	MFLAGS_NOFOLDERS               = (1 << 16), // активная:  запускать, если текущий объект "файл"
	MFLAGS_PNOFOLDERS              = (1 << 17), // пассивная: запускать, если текущий объект "файл"
	MFLAGS_NOFILES                 = (1 << 18), // активная:  запускать, если текущий объект "папка"
	MFLAGS_PNOFILES                = (1 << 19), // пассивная: запускать, если текущий объект "папка"
	// private flags, for runtime purposes only
	MFLAGS_POSTFROMPLUGIN          = (1 << 28); // последовательность пришла от АПИ

enum MACRO_OP
{
	OP_ISEXECUTING                 = 1,
	OP_ISDISABLEOUTPUT             = 2,
	OP_HISTORYDISABLEMASK          = 3,
	OP_ISHISTORYDISABLE            = 4,
	OP_ISTOPMACROOUTPUTDISABLED    = 5,
	OP_ISPOSTMACROENABLED          = 6,
	OP_POSTNEWMACRO                = 7,
	OP_SETMACROVALUE               = 8,
	OP_GETINPUTFROMMACRO           = 9,
	OP_TRYTOPOSTMACRO              = 10,
	OP_GETLASTERROR                = 11,
};

enum MACRO_IMPORT
{
	IMP_RESTORE_MACROCHAR          = 1,
	IMP_SCRBUF_LOCK                = 2,
	IMP_SCRBUF_UNLOCK              = 3,
	IMP_SCRBUF_RESETLOCKCOUNT      = 4,
	IMP_SCRBUF_GETLOCKCOUNT        = 5,
	IMP_SCRBUF_SETLOCKCOUNT        = 6,
	IMP_GET_USEINTERNALCLIPBOARD   = 7,
	IMP_SET_USEINTERNALCLIPBOARD   = 8,
	IMP_KEYNAMETOKEY               = 9,
	IMP_KEYTOTEXT                  = 10,
};

#endif
