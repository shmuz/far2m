local ffi = require "ffi"
ffi.cdef [=[
#pragma pack(2)

typedef int FarLangMsgID;
typedef uint32_t FarKey;
typedef struct _INPUT_RECORD INPUT_RECORD;
typedef struct _CHAR_INFO    CHAR_INFO;

enum FARMESSAGEFLAGS
{
	FMSG_WARNING             = 0x00000001,
	FMSG_ERRORTYPE           = 0x00000002,
	FMSG_KEEPBACKGROUND      = 0x00000004,
	FMSG_LEFTALIGN           = 0x00000010,

	FMSG_ALLINONE            = 0x00000020,

	FMSG_MB_OK               = 0x00010000,
	FMSG_MB_OKCANCEL         = 0x00020000,
	FMSG_MB_ABORTRETRYIGNORE = 0x00030000,
	FMSG_MB_YESNO            = 0x00040000,
	FMSG_MB_YESNOCANCEL      = 0x00050000,
	FMSG_MB_RETRYCANCEL      = 0x00060000,
};

typedef int (__stdcall *FARAPIMESSAGE)(
	INT_PTR PluginNumber,
	DWORD Flags,
	const wchar_t *HelpTopic,
	const wchar_t * const *Items,
	int ItemsNumber,
	int ButtonsNumber
);

typedef int (__stdcall *FARAPIMESSAGEV3)(
	INT_PTR PluginNumber,
	const GUID* Id,
	DWORD Flags,
	const wchar_t *HelpTopic,
	const wchar_t * const *Items,
	int ItemsNumber,
	int ButtonsNumber
);

enum DialogItemTypes
{
	DI_TEXT,
	DI_VTEXT,
	DI_SINGLEBOX,
	DI_DOUBLEBOX,
	DI_EDIT,
	DI_PSWEDIT,
	DI_FIXEDIT,
	DI_BUTTON,
	DI_CHECKBOX,
	DI_RADIOBUTTON,
	DI_COMBOBOX,
	DI_LISTBOX,

	DI_USERCONTROL=255,
};




enum FarDialogItemFlags
{
	DIF_NONE                  = 0,
	DIF_COLORMASK             = 0x000000ffU,
	DIF_SETCOLOR              = 0x00000100U,
	DIF_BOXCOLOR              = 0x00000200U,
	DIF_GROUP                 = 0x00000400U,
	DIF_LEFTTEXT              = 0x00000800U,
	DIF_MOVESELECT            = 0x00001000U,
	DIF_SHOWAMPERSAND         = 0x00002000U,
	DIF_CENTERGROUP           = 0x00004000U,
	DIF_NOBRACKETS            = 0x00008000U,
	DIF_MANUALADDHISTORY      = 0x00008000U,
	DIF_SEPARATOR             = 0x00010000U,
	DIF_SEPARATOR2            = 0x00020000U,
	DIF_EDITOR                = 0x00020000U,
	DIF_LISTNOAMPERSAND       = 0x00020000U,
	DIF_LISTNOBOX             = 0x00040000U,
	DIF_HISTORY               = 0x00040000U,
	DIF_BTNNOCLOSE            = 0x00040000U,
	DIF_CENTERTEXT            = 0x00040000U,
	DIF_SETSHIELD             = 0x00080000U,
	DIF_EDITEXPAND            = 0x00080000U,
	DIF_DROPDOWNLIST          = 0x00100000U,
	DIF_USELASTHISTORY        = 0x00200000U,
	DIF_MASKEDIT              = 0x00400000U,
	DIF_SELECTONENTRY         = 0x00800000U,
	DIF_3STATE                = 0x00800000U,
	DIF_EDITPATH              = 0x01000000U,
	DIF_LISTWRAPMODE          = 0x01000000U,
	DIF_NOAUTOCOMPLETE        = 0x02000000U,
	DIF_LISTAUTOHIGHLIGHT     = 0x02000000U,
	DIF_LISTNOCLOSE           = 0x04000000U,
	DIF_HIDDEN                = 0x10000000U,
	DIF_READONLY              = 0x20000000U,
	DIF_NOFOCUS               = 0x40000000U,
	DIF_DISABLE               = 0x80000000U,
};

enum FarMessagesProc
{
	DM_FIRST=0,
	DM_CLOSE,
	DM_ENABLE,
	DM_ENABLEREDRAW,
	DM_GETDLGDATA,
	DM_GETDLGITEM,
	DM_GETDLGRECT,
	DM_GETTEXT,
	DM_GETTEXTLENGTH,
	DM_KEY,
	DM_MOVEDIALOG,
	DM_SETDLGDATA,
	DM_SETDLGITEM,
	DM_SETFOCUS,
	DM_REDRAW,
	DM_SETREDRAW=DM_REDRAW,
	DM_SETTEXT,
	DM_SETMAXTEXTLENGTH,
	DM_SETTEXTLENGTH=DM_SETMAXTEXTLENGTH,
	DM_SHOWDIALOG,
	DM_GETFOCUS,
	DM_GETCURSORPOS,
	DM_SETCURSORPOS,
	DM_GETTEXTPTR,
	DM_SETTEXTPTR,
	DM_SHOWITEM,
	DM_ADDHISTORY,

	DM_GETCHECK,
	DM_SETCHECK,
	DM_SET3STATE,

	DM_LISTSORT,
	DM_LISTGETITEM,
	DM_LISTGETCURPOS,
	DM_LISTSETCURPOS,
	DM_LISTDELETE,
	DM_LISTADD,
	DM_LISTADDSTR,
	DM_LISTUPDATE,
	DM_LISTINSERT,
	DM_LISTFINDSTRING,
	DM_LISTINFO,
	DM_LISTGETDATA,
	DM_LISTSETDATA,
	DM_LISTSETTITLES,
	DM_LISTGETTITLES,

	DM_RESIZEDIALOG,
	DM_SETITEMPOSITION,

	DM_GETDROPDOWNOPENED,
	DM_SETDROPDOWNOPENED,

	DM_SETHISTORY,

	DM_GETITEMPOSITION,
	DM_SETMOUSEEVENTNOTIFY,

	DM_EDITUNCHANGEDFLAG,

	DM_GETITEMDATA,
	DM_SETITEMDATA,

	DM_LISTSET,
	DM_LISTSETMOUSEREACTION,

	DM_GETCURSORSIZE,
	DM_SETCURSORSIZE,

	DM_LISTGETDATASIZE,

	DM_GETSELECTION,
	DM_SETSELECTION,

	DM_GETEDITPOSITION,
	DM_SETEDITPOSITION,

	DM_SETCOMBOBOXEVENT,
	DM_GETCOMBOBOXEVENT,

	DM_GETCONSTTEXTPTR,
	DM_GETDLGITEMSHORT,
	DM_SETDLGITEMSHORT,

	DM_GETDIALOGINFO,

	DM_SETREADONLY,
	DM_GETDEFAULTCOLOR, // Param1 - Item ID, Param2 - uint64_t * -> uint64_t ItemColors[4]

	DM_GETTRUECOLOR,	// Param1 - Item ID, Param2 - uint64_t * -> uint64_t ItemColors[4]
	DM_GETCOLOR = DM_GETTRUECOLOR,
	DM_SETTRUECOLOR,	// Param1 - Item ID, Param2 - uint64_t * -> uint64_t ItemColors[4]
	DM_SETCOLOR = DM_SETTRUECOLOR,

	DM_SETTEXTPTRSILENT,

	DN_FIRST=0x1000,
	DN_BTNCLICK,
	DN_CTLCOLORDIALOG,
	DN_CTLCOLORDLGITEM,
	DN_CTLCOLORDLGLIST,
	DN_DRAWDIALOG,
	DN_DRAWDLGITEM,
	DN_EDITCHANGE,
	DN_ENTERIDLE,
	DN_GOTFOCUS,
	DN_HELP,
	DN_HOTKEY,
	DN_INITDIALOG,
	DN_KILLFOCUS,
	DN_LISTCHANGE,
	DN_MOUSECLICK,
	DN_DRAGGED,
	DN_RESIZECONSOLE,
	DN_MOUSEEVENT,
	DN_DRAWDIALOGDONE,
	DN_LISTHOTKEY,

	DN_GETDIALOGINFO=DM_GETDIALOGINFO,

	DN_CLOSE=DM_CLOSE,
	DN_KEY=DM_KEY,


	DM_USER=0x4000,

};

enum FARCHECKEDSTATE
{
	BSTATE_UNCHECKED = 0,
	BSTATE_CHECKED   = 1,
	BSTATE_3STATE    = 2,
	BSTATE_TOGGLE    = 3,
};

enum FARLISTMOUSEREACTIONTYPE
{
	LMRT_ONLYFOCUS   = 0,
	LMRT_ALWAYS      = 1,
	LMRT_NEVER       = 2,
};

enum FARCOMBOBOXEVENTTYPE
{
	CBET_KEY         = 0x00000001,
	CBET_MOUSE       = 0x00000002,
};

enum LISTITEMFLAGS
{
	LIF_SELECTED           = 0x00010000U,
	LIF_CHECKED            = 0x00020000U,
	LIF_SEPARATOR          = 0x00040000U,
	LIF_DISABLE            = 0x00080000U,
	LIF_GRAYED             = 0x00100000U,
	LIF_HIDDEN             = 0x00200000U,
	LIF_DELETEUSERDATA     = 0x80000000U,
};

struct FarListItem
{
	DWORD Flags;
	const wchar_t *Text;
	DWORD Reserved[3];
};

struct FarListUpdate
{
	int Index;
	struct FarListItem Item;
};

struct FarListInsert
{
	int Index;
	struct FarListItem Item;
};

struct FarListGetItem
{
	int ItemIndex;
	struct FarListItem Item;
};

struct FarListPos
{
	int SelectPos;
	int TopPos;
};

enum FARLISTFINDFLAGS
{
	LIFIND_EXACTMATCH = 0x00000001,
};

struct FarListFind
{
	int StartIndex;
	const wchar_t *Pattern;
	DWORD Flags;
	DWORD Reserved;
};

struct FarListDelete
{
	int StartIndex;
	int Count;
};

enum FARLISTINFOFLAGS
{
	LINFO_SHOWNOBOX             = 0x00000400,
	LINFO_AUTOHIGHLIGHT         = 0x00000800,
	LINFO_REVERSEHIGHLIGHT      = 0x00001000,
	LINFO_WRAPMODE              = 0x00008000,
	LINFO_SHOWAMPERSAND         = 0x00010000,
};

struct FarListInfo
{
	DWORD Flags;
	int ItemsNumber;
	int SelectPos;
	int TopPos;
	int MaxHeight;
	int MaxLength;
	DWORD Reserved[6];
};

struct FarListItemData
{
	int   Index;
	int   DataSize;
	void *Data;
	DWORD Reserved;
};

struct FarList
{
	int ItemsNumber;
	struct FarListItem *Items;
};

struct FarListTitles
{
	int   TitleLen;
	const wchar_t *Title;
	int   BottomLen;
	const wchar_t *Bottom;
};

struct FarListColors
{
	DWORD  Flags;
	DWORD  Reserved;
	int    ColorCount;
	uint64_t *Colors;
};

struct FarDialogItem
{
	int Type;
	int X1,Y1,X2,Y2;
	int Focus;
	union
	{
		DWORD_PTR Reserved;
		int Selected;
		const wchar_t *History;
		const wchar_t *Mask;
		struct FarList *ListItems;
		int  ListPos;
		CHAR_INFO *VBuf;
	}
	Param
	;
	DWORD Flags;
	int DefaultButton;

	const wchar_t *PtrData;
	size_t MaxLen; // terminate 0 not included (if == 0 string size is unlimited)
};

struct FarDialogItemData
{
	size_t  PtrLength;
	wchar_t *PtrData;
};

struct FarDialogEvent
{
	HANDLE hDlg;
	int Msg;
	int Param1;
	LONG_PTR Param2;
	LONG_PTR Result;
};

struct OpenDlgPluginData
{
	int ItemNumber;
	HANDLE hDlg;
	GUID ItemGuid;
};

struct DialogInfo
{
	int StructSize;
	GUID Id;
	DWORD Owner;
};

enum FARDIALOGFLAGS
{
	FDLG_WARNING             = 0x00000001,
	FDLG_SMALLDIALOG         = 0x00000002,
	FDLG_NODRAWSHADOW        = 0x00000004,
	FDLG_NODRAWPANEL         = 0x00000008,
	FDLG_NONMODAL            = 0x00000010,
	FDLG_KEEPCONSOLETITLE    = 0x00000020,
	FDLG_REGULARIDLE         = 0x00000040 // causes dialog to receive DN_ENTERIDLE at least once per second
};

typedef LONG_PTR(__stdcall *FARWINDOWPROC)(
	HANDLE   hDlg,
	int      Msg,
	int      Param1,
	LONG_PTR Param2
);

typedef LONG_PTR(__stdcall *FARAPISENDDLGMESSAGE)(
	HANDLE   hDlg,
	int      Msg,
	int      Param1,
	LONG_PTR Param2
);

typedef LONG_PTR(__stdcall *FARAPIDEFDLGPROC)(
	HANDLE   hDlg,
	int      Msg,
	int      Param1,
	LONG_PTR Param2
);

typedef HANDLE(__stdcall *FARAPIDIALOGINIT)(
	INT_PTR               PluginNumber,
	int                   X1,
	int                   Y1,
	int                   X2,
	int                   Y2,
	const wchar_t        *HelpTopic,
	struct FarDialogItem *Item,
	unsigned int          ItemsNumber,
	DWORD                 Reserved,
	DWORD                 Flags,
	FARWINDOWPROC         DlgProc,
	LONG_PTR              Param
);

typedef HANDLE(__stdcall *FARAPIDIALOGINITV3)(
	INT_PTR               PluginNumber,
	const GUID           *Id,
	int                   X1,
	int                   Y1,
	int                   X2,
	int                   Y2,
	const wchar_t        *HelpTopic,
	struct FarDialogItem *Item,
	unsigned int          ItemsNumber,
	DWORD                 Reserved,
	DWORD                 Flags,
	FARWINDOWPROC         DlgProc,
	LONG_PTR              Param
);

typedef int (__stdcall *FARAPIDIALOGRUN)(
	HANDLE hDlg
);

typedef void (__stdcall *FARAPIDIALOGFREE)(
	HANDLE hDlg
);

struct FarMenuItem
{
	const wchar_t *Text;
	int  Selected;
	int  Checked;
	int  Separator;
};

enum MENUITEMFLAGS
{
	MIF_NONE   = 0,
	MIF_SELECTED   = 0x00010000U,
	MIF_CHECKED    = 0x00020000U,
	MIF_SEPARATOR  = 0x00040000U,
	MIF_DISABLE    = 0x00080000U,
	MIF_GRAYED     = 0x00100000U,
	MIF_HIDDEN     = 0x00200000U,
};

struct FarMenuItemEx
{
	DWORD Flags;
	const wchar_t *Text;
	DWORD AccelKey;
	DWORD Reserved;
	DWORD_PTR UserData;
};

enum FARMENUFLAGS
{
	FMENU_SHOWAMPERSAND        = 0x00000001,
	FMENU_WRAPMODE             = 0x00000002,
	FMENU_AUTOHIGHLIGHT        = 0x00000004,
	FMENU_REVERSEAUTOHIGHLIGHT = 0x00000008,
	FMENU_USEEXT               = 0x00000020,
	FMENU_CHANGECONSOLETITLE   = 0x00000040,
};

typedef int (__stdcall *FARAPIMENU)(
	INT_PTR             PluginNumber,
	int                 X,
	int                 Y,
	int                 MaxHeight,
	DWORD               Flags,
	const wchar_t      *Title,
	const wchar_t      *Bottom,
	const wchar_t      *HelpTopic,
	const int          *BreakKeys,
	int                *BreakCode,
	const struct FarMenuItem *Item,
	int                 ItemsNumber
);

typedef int (*FARMENUCALLBACK)(void *CallbackData, int Pos, FarKey Key);

enum FARMENUCALLBACKRETURN
{
	FMCB_CANCELMENU     = -1,
	FMCB_PROCESSKEY     = 0,
	FMCB_DONTPROCESSKEY = 1,
	FMCB_RETURNCURPOS   = 2,
};

typedef int (__stdcall *FARAPIMENUV2)(
	INT_PTR             PluginNumber,
	const GUID         *Id,
	int                 X,
	int                 Y,
	int                 MaxHeight,
	DWORD               Flags,
	const wchar_t      *Title,
	const wchar_t      *Bottom,
	const wchar_t      *HelpTopic,
	const int          *BreakKeys,
	int                *BreakCode,
	const struct FarMenuItem *Item,
	int                 ItemsNumber,
	FARMENUCALLBACK     Callback,
	void               *CallbackData
);


enum PLUGINPANELITEMFLAGS
{
	PPIF_PROCESSDESCR           = 0x80000000,
	PPIF_SELECTED               = 0x40000000,
	PPIF_USERDATA               = 0x20000000,
};

struct FAR_FIND_DATA
{
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	uint64_t nPhysicalSize;
	uint64_t nFileSize;
	DWORD    dwFileAttributes;
	DWORD    dwUnixMode;
	const wchar_t *lpwszFileName;
};

struct PluginPanelItem
{
	struct FAR_FIND_DATA FindData;
	DWORD_PTR     UserData;
	DWORD         Flags;
	DWORD         NumberOfLinks;
	const wchar_t *Description;
	const wchar_t *Owner;
	const wchar_t *Group;
	const wchar_t * const *CustomColumnData;
	int           CustomColumnNumber;
	DWORD         CRC32;
	DWORD_PTR     Reserved[2];
};

struct SortingPanelItem
{
	FILETIME             CreationTime;
	FILETIME             LastAccessTime;
	FILETIME             LastWriteTime;
	FILETIME             ChangeTime;
	uint64_t             FileSize;
	uint64_t             AllocationSize;
	const wchar_t*       FileName;
	const wchar_t*       Description;
	const wchar_t*       Owner;
	const wchar_t*       const *CustomColumnData;
	int                  CustomColumnNumber;
	DWORD                Flags;
	DWORD_PTR            UserData;
	DWORD                FileAttributes;
	DWORD                NumberOfLinks;
	DWORD                CRC32;
	int                  Position;
	int                  SortGroup;
};

enum PANELINFOFLAGS
{
	PFLAGS_SHOWHIDDEN         = 0x00000001,
	PFLAGS_HIGHLIGHT          = 0x00000002,
	PFLAGS_REVERSESORTORDER   = 0x00000004,
	PFLAGS_USESORTGROUPS      = 0x00000008,
	PFLAGS_SELECTEDFIRST      = 0x00000010,
	PFLAGS_REALNAMES          = 0x00000020,
	PFLAGS_NUMERICSORT        = 0x00000040,
	PFLAGS_PANELLEFT          = 0x00000080,
	PFLAGS_DIRECTORIESFIRST   = 0x00000100,
	PFLAGS_USECRC32           = 0x00000200,
	PFLAGS_CASESENSITIVESORT  = 0x00000400,
	PFLAGS_HL_MARKERS_NOSHOW  = 0x00000800,
	PFLAGS_HL_MARKERS_NOALIGN = 0x00001000,
	PFLAGS_PLUGIN             = 0x00002000,
	PFLAGS_VISIBLE            = 0x00004000,
	PFLAGS_FOCUS              = 0x00008000,
};

enum PANELINFOTYPE
{
	PTYPE_FILEPANEL,
	PTYPE_TREEPANEL,
	PTYPE_QVIEWPANEL,
	PTYPE_INFOPANEL
};

struct PanelInfo
{
	int PanelType;
	int Plugin;
	RECT PanelRect;
	int ItemsNumber;
	int SelectedItemsNumber;
	int CurrentItem;
	int TopPanelItem;
	int Visible;
	int Focus;
	int ViewMode;
	int SortMode;
	DWORD Flags;
	DWORD Reserved;

	HANDLE PluginHandle;
	HANDLE OwnerHandle;
	DWORD OwnerID;
};


struct PanelRedrawInfo
{
	int CurrentItem;
	int TopPanelItem;
};

struct CmdLineSelect
{
	int SelStart;
	int SelEnd;
};

struct FarPanelLocation
{
	const wchar_t *PluginName; // set to -1 if its plain directory navigation
	const wchar_t *HostFile; // if set the OpenFilePlugin is used and Item is ignored, otherwise its normal plugin
	LONG_PTR Item; // ignored if HostFile is not NULL
	const wchar_t *Path;
};

enum FILE_CONTROL_COMMANDS
{
	FCTL_CLOSEPLUGIN,
	FCTL_GETPANELINFO,
	FCTL_UPDATEPANEL,
	FCTL_REDRAWPANEL,
	FCTL_GETCMDLINE,
	FCTL_SETCMDLINE,
	FCTL_SETSELECTION,
	FCTL_SETVIEWMODE,
	FCTL_INSERTCMDLINE,
	FCTL_SETUSERSCREEN,
	FCTL_SETPANELDIR,
	FCTL_SETCMDLINEPOS,
	FCTL_GETCMDLINEPOS,
	FCTL_SETSORTMODE,
	FCTL_SETSORTORDER,
	FCTL_GETCMDLINESELECTEDTEXT,
	FCTL_SETCMDLINESELECTION,
	FCTL_GETCMDLINESELECTION,
	FCTL_CHECKPANELSEXIST,
	FCTL_SETNUMERICSORT,
	FCTL_GETUSERSCREEN,
	FCTL_ISACTIVEPANEL,
	FCTL_GETPANELITEM,
	FCTL_GETSELECTEDPANELITEM,
	FCTL_GETCURRENTPANELITEM,
	FCTL_GETPANELDIR,
	FCTL_GETCOLUMNTYPES,
	FCTL_GETCOLUMNWIDTHS,
	FCTL_BEGINSELECTION,
	FCTL_ENDSELECTION,
	FCTL_CLEARSELECTION,
	FCTL_SETDIRECTORIESFIRST,
	FCTL_GETPANELFORMAT,
	FCTL_GETPANELHOSTFILE,
	FCTL_SETCASESENSITIVESORT,
	FCTL_GETPANELPLUGINHANDLE, // Param2 points to value of type HANDLE, sets that value to handle of plugin that renders that panel or INVALID_HANDLE_VALUE
	FCTL_SETPANELLOCATION, // Param2 points to FarPanelLocation
	FCTL_RESERVED1,
	FCTL_RESERVED2,
	FCTL_GETPANELPREFIX,
	FCTL_SETACTIVEPANEL,
};

typedef int (__stdcall *FARAPICONTROL)(
	HANDLE hPlugin,
	int Command,
	int Param1,
	LONG_PTR Param2
);

typedef void (__stdcall *FARAPITEXT)(
	int X,
	int Y,
	uint64_t Color,
	const wchar_t *Str
);

typedef HANDLE(__stdcall *FARAPISAVESCREEN)(int X1, int Y1, int X2, int Y2);

typedef void (__stdcall *FARAPIRESTORESCREEN)(HANDLE hScreen);

typedef void (__stdcall *FARAPIFREESCREEN)(HANDLE hScreen);

typedef int (__stdcall *FARAPIGETDIRLIST)(
	const wchar_t *Dir,
	struct FAR_FIND_DATA **pPanelItem,
	int *pItemsNumber
);

typedef int (__stdcall *FARAPIGETPLUGINDIRLIST)(
	INT_PTR PluginNumber,
	HANDLE hPlugin,
	const wchar_t *Dir,
	struct PluginPanelItem **pPanelItem,
	int *pItemsNumber
);

typedef void (__stdcall *FARAPIFREEDIRLIST)(struct FAR_FIND_DATA *PanelItem, int nItemsNumber);
typedef void (__stdcall *FARAPIFREEPLUGINDIRLIST)(struct PluginPanelItem *PanelItem, int nItemsNumber);

enum VIEWER_FLAGS
{
	VF_NONMODAL              = 0x00000001,
	VF_DELETEONCLOSE         = 0x00000002,
	VF_ENABLE_F6             = 0x00000004,
	VF_DISABLEHISTORY        = 0x00000008,
	VF_IMMEDIATERETURN       = 0x00000100,
	VF_DELETEONLYFILEONCLOSE = 0x00000200,
};

typedef int (__stdcall *FARAPIVIEWER)(
	const wchar_t *FileName,
	const wchar_t *Title,
	int X1,
	int Y1,
	int X2,
	int Y2,
	DWORD Flags,
	UINT CodePage
);

enum EDITOR_FLAGS
{
	EF_NONMODAL              = 0x00000001,
	EF_CREATENEW             = 0x00000002,
	EF_ENABLE_F6             = 0x00000004,
	EF_DISABLEHISTORY        = 0x00000008,
	EF_DELETEONCLOSE         = 0x00000010,
	EF_IMMEDIATERETURN       = 0x00000100,
	EF_DELETEONLYFILEONCLOSE = 0x00000200,
	EF_LOCKED                = 0x00000400,

	EF_OPENMODE_MASK         = 0xF0000000,
	EF_OPENMODE_NEWIFOPEN    = 0x10000000,
	EF_OPENMODE_USEEXISTING  = 0x20000000,
	EF_OPENMODE_BREAKIFOPEN  = 0x30000000,
	EF_OPENMODE_RELOADIFOPEN = 0x40000000,
};

enum EDITOR_EXITCODE
{
	EEC_OPEN_ERROR          = 0,
	EEC_MODIFIED            = 1,
	EEC_NOT_MODIFIED        = 2,
	EEC_LOADING_INTERRUPTED = 3,
};

typedef int (__stdcall *FARAPIEDITOR)(
	const wchar_t *FileName,
	const wchar_t *Title,
	int X1,
	int Y1,
	int X2,
	int Y2,
	DWORD Flags,
	int StartLine,
	int StartChar,
	UINT CodePage
);

typedef int (__stdcall *FARAPICMPNAME)(
	const wchar_t *Pattern,
	const wchar_t *String,
	int SkipPath
);


typedef const wchar_t*(__stdcall *FARAPIGETMSG)(
	INT_PTR PluginNumber,
	FarLangMsgID MsgId
);


enum FarHelpFlags
{
	FHELP_NOSHOWERROR = 0x80000000,
	FHELP_SELFHELP    = 0x00000000,
	FHELP_FARHELP     = 0x00000001,
	FHELP_CUSTOMFILE  = 0x00000002,
	FHELP_CUSTOMPATH  = 0x00000004,
	FHELP_USECONTENTS = 0x40000000,
};

typedef BOOL (__stdcall *FARAPISHOWHELP)(
	const wchar_t *ModuleName,
	const wchar_t *Topic,
	DWORD Flags
);

enum ADVANCED_CONTROL_COMMANDS
{
	ACTL_GETFARVERSION        = 0,
	ACTL_GETSYSWORDDIV        = 2,
	ACTL_WAITKEY              = 3,
	ACTL_GETCOLOR             = 4,
	ACTL_GETARRAYCOLOR        = 5,
	ACTL_EJECTMEDIA           = 6,
	ACTL_GETWINDOWINFO        = 9,
	ACTL_GETWINDOWCOUNT       = 10,
	ACTL_SETCURRENTWINDOW     = 11,
	ACTL_COMMIT               = 12,
	ACTL_GETFARHWND           = 13,
	ACTL_GETSYSTEMSETTINGS    = 14,
	ACTL_GETPANELSETTINGS     = 15,
	ACTL_GETINTERFACESETTINGS = 16,
	ACTL_GETCONFIRMATIONS     = 17,
	ACTL_GETDESCSETTINGS      = 18,
	ACTL_SETARRAYCOLOR        = 19,
	ACTL_GETPLUGINMAXREADDATA = 21,
	ACTL_GETDIALOGSETTINGS    = 22,
	ACTL_GETSHORTWINDOWINFO   = 23,
	ACTL_REDRAWALL            = 27,
	ACTL_SYNCHRO              = 28,
	ACTL_SETPROGRESSSTATE     = 29,
	ACTL_SETPROGRESSVALUE     = 30,
	ACTL_QUIT                 = 31,
	ACTL_GETFARRECT           = 32,
	ACTL_GETCURSORPOS         = 33,
	ACTL_SETCURSORPOS         = 34,
	ACTL_PROGRESSNOTIFY       = 35,

	ACTL_WINPORTBACKEND       = 40,
};

enum FAR_MACRO_CONTROL_COMMANDS
{
	MCTL_LOADALL           = 0,
	MCTL_SAVEALL           = 1,
	MCTL_SENDSTRING        = 2,
	MCTL_GETSTATE          = 5,
	MCTL_GETAREA           = 6,
	MCTL_ADDMACRO          = 7,
	MCTL_DELMACRO          = 8,
	MCTL_GETLASTERROR      = 9,
	MCTL_EXECSTRING        = 10,
};


enum FarSystemSettings
{
	FSS_DELETETORECYCLEBIN             = 0x00000002,
	FSS_WRITETHROUGH                   = 0x00000004,
	FSS_RESERVED                       = 0x00000008,
	FSS_SAVECOMMANDSHISTORY            = 0x00000020,
	FSS_SAVEFOLDERSHISTORY             = 0x00000040,
	FSS_SAVEVIEWANDEDITHISTORY         = 0x00000080,
	FSS_USEWINDOWSREGISTEREDTYPES      = 0x00000100,
	FSS_AUTOSAVESETUP                  = 0x00000200,
	FSS_SCANSYMLINK                    = 0x00000400,
};

enum FarPanelSettings
{
	FPS_SHOWHIDDENANDSYSTEMFILES       = 0x00000001,
	FPS_HIGHLIGHTFILES                 = 0x00000002,
	FPS_AUTOCHANGEFOLDER               = 0x00000004,
	FPS_SELECTFOLDERS                  = 0x00000008,
	FPS_ALLOWREVERSESORTMODES          = 0x00000010,
	FPS_SHOWCOLUMNTITLES               = 0x00000020,
	FPS_SHOWSTATUSLINE                 = 0x00000040,
	FPS_SHOWFILESTOTALINFORMATION      = 0x00000080,
	FPS_SHOWFREESIZE                   = 0x00000100,
	FPS_SHOWSCROLLBAR                  = 0x00000200,
	FPS_SHOWBACKGROUNDSCREENSNUMBER    = 0x00000400,
	FPS_SHOWSORTMODELETTER             = 0x00000800,
};

enum FarDialogSettings
{
	FDIS_HISTORYINDIALOGEDITCONTROLS    = 0x00000001,
	FDIS_PERSISTENTBLOCKSINEDITCONTROLS = 0x00000002,
	FDIS_AUTOCOMPLETEININPUTLINES       = 0x00000004,
	FDIS_BSDELETEUNCHANGEDTEXT          = 0x00000008,
	FDIS_DELREMOVESBLOCKS               = 0x00000010,
	FDIS_MOUSECLICKOUTSIDECLOSESDIALOG  = 0x00000020,
};

enum FarInterfaceSettings
{
	FIS_CLOCKINPANELS                  = 0x00000001,
	FIS_CLOCKINVIEWERANDEDITOR         = 0x00000002,
	FIS_MOUSE                          = 0x00000004,
	FIS_SHOWKEYBAR                     = 0x00000008,
	FIS_ALWAYSSHOWMENUBAR              = 0x00000010,
	FIS_SHOWTOTALCOPYPROGRESSINDICATOR = 0x00000100,
	FIS_SHOWCOPYINGTIMEINFO            = 0x00000200,
	FIS_USECTRLPGUPTOCHANGEDRIVE       = 0x00000800,
	FIS_SHOWTOTALDELPROGRESSINDICATOR  = 0x00001000,
};

enum FarConfirmationsSettings
{
	FCS_COPYOVERWRITE                  = 0x00000001,
	FCS_MOVEOVERWRITE                  = 0x00000002,
	FCS_DRAGANDDROP                    = 0x00000004,
	FCS_DELETE                         = 0x00000008,
	FCS_DELETENONEMPTYFOLDERS          = 0x00000010,
	FCS_INTERRUPTOPERATION             = 0x00000020,
	FCS_DISCONNECTNETWORKDRIVE         = 0x00000040,
	FCS_RELOADEDITEDFILE               = 0x00000080,
	FCS_CLEARHISTORYLIST               = 0x00000100,
	FCS_EXIT                           = 0x00000200,
	FCS_OVERWRITEDELETEROFILES         = 0x00000400,
};

enum FarDescriptionSettings
{
	FDS_UPDATEALWAYS                   = 0x00000001,
	FDS_UPDATEIFDISPLAYED              = 0x00000002,
	FDS_SETHIDDEN                      = 0x00000004,
	FDS_UPDATEREADONLY                 = 0x00000008,
};

enum FAREJECTMEDIAFLAGS
{
	EJECT_NO_MESSAGE                    = 0x00000001,
	EJECT_LOAD_MEDIA                    = 0x00000002,
};

struct ActlEjectMedia
{
	DWORD Letter;
	DWORD Flags;
};


enum FARKEYMACROFLAGS
{
	KMFLAGS_SILENTCHECK         = 0x00000001,
	KMFLAGS_NOSENDKEYSTOPLUGINS = 0x00000002,
	KMFLAGS_ENABLEOUTPUT        = 0x00000004,
	KMFLAGS_LANGMASK            = 0x00000070, // 3 bits reserved for 8 languages
	KMFLAGS_LUA                 = 0x00000000,
	KMFLAGS_MOONSCRIPT          = 0x00000010,
	KMFLAGS_NONE                = 0,
};

enum FARMACROSENDSTRINGCOMMAND
{
	MSSC_POST              =0,
	MSSC_CHECK             =2,
};

enum FARMACROAREA
{
	MACROAREA_OTHER                = 0,
	MACROAREA_SHELL                = 1,
	MACROAREA_VIEWER               = 2,
	MACROAREA_EDITOR               = 3,
	MACROAREA_DIALOG               = 4,
	MACROAREA_SEARCH               = 5,
	MACROAREA_DISKS                = 6,
	MACROAREA_MAINMENU             = 7,
	MACROAREA_MENU                 = 8,
	MACROAREA_HELP                 = 9,
	MACROAREA_INFOPANEL            =10,
	MACROAREA_QVIEWPANEL           =11,
	MACROAREA_TREEPANEL            =12,
	MACROAREA_FINDFOLDER           =13,
	MACROAREA_USERMENU             =14,
	MACROAREA_SHELLAUTOCOMPLETION  =15,
	MACROAREA_DIALOGAUTOCOMPLETION =16,
	MACROAREA_GRABBER              =17,
	MACROAREA_COMMON            =255,
};

enum FARMACROSTATE
{
	MACROSTATE_NOMACRO          = 0,
	MACROSTATE_EXECUTING        = 1,
	MACROSTATE_EXECUTING_COMMON = 2,
	MACROSTATE_RECORDING        = 3,
	MACROSTATE_RECORDING_COMMON = 4,
};

enum FARMACROPARSEERRORCODE
{
	MPEC_SUCCESS = 0,
	MPEC_ERROR   = 1,
};

struct MacroParseResult
{
	size_t StructSize;
	DWORD ErrCode;
	COORD ErrPos;
	const wchar_t *ErrSrc;
};

struct MacroSendMacroText
{
	size_t StructSize;
	DWORD Flags;
	DWORD AKey;
	const wchar_t *SequenceText;
};

enum FARADDKEYMACROFLAGS
{
	AKMFLAGS_NONE = 0,
};

typedef intptr_t (__stdcall *FARMACROCALLBACK)(void* Id,DWORD Flags);

struct MacroAddMacro
{
	size_t StructSize;
	void* Id;
	const wchar_t *SequenceText;
	const wchar_t *Description;
	DWORD Flags;
	const wchar_t *AKey;
	enum FARMACROAREA Area;
	FARMACROCALLBACK Callback;
	int Priority;
};

enum FARMACROVARTYPE
{
	FMVT_UNKNOWN                = 0,
	FMVT_INTEGER                = 1,
	FMVT_STRING                 = 2,
	FMVT_DOUBLE                 = 3,
	FMVT_BOOLEAN                = 4,
	FMVT_BINARY                 = 5,
	FMVT_POINTER                = 6,
	FMVT_NIL                    = 7,
	FMVT_ARRAY                  = 8,
	FMVT_PANEL                  = 9,
	FMVT_ERROR                  = 10,
};

struct FarMacroValue
{
	enum FARMACROVARTYPE Type;
	union
	{
		int64_t        Integer;
		int64_t        Boolean;
		double         Double;
		const wchar_t *String;
		void          *Pointer;
		struct
		{
			const void *Data;
			size_t Size;
		} Binary;
		struct
		{
			struct FarMacroValue *Values;
			size_t Count;
		} Array;
	}
	Value
	;

};

struct FarMacroCall
{
	size_t StructSize;
	size_t Count;
	struct FarMacroValue *Values;
	void (__stdcall *Callback)(void *CallbackData, struct FarMacroValue *Values, size_t Count);
	void *CallbackData;
};

struct FarGetValue
{
	size_t StructSize;
	intptr_t Type;
	struct FarMacroValue Value;
};

struct MacroExecuteString
{
	size_t StructSize;
	DWORD Flags;
	const wchar_t *SequenceText;
	size_t InCount;
	struct FarMacroValue *InValues;
	size_t OutCount;
	const struct FarMacroValue *OutValues;
};

struct FarMacroLoad
{
	size_t StructSize;
	const wchar_t *Path;
	unsigned long long Flags;
};

struct MacroPluginReturn
{
	intptr_t ReturnType;
	size_t Count;
	struct FarMacroValue *Values;
};

enum MACROCALLTYPE
{
	MCT_MACROPARSE         = 0,
	MCT_LOADMACROS         = 1,
	MCT_ENUMMACROS         = 2,
	MCT_WRITEMACROS        = 3,
	MCT_GETMACRO           = 4,
	MCT_RECORDEDMACRO      = 5,
	MCT_DELMACRO           = 6,
	MCT_RUNSTARTMACRO      = 7,
	MCT_EXECSTRING         = 8,
	MCT_PANELSORT          = 9,
	MCT_GETCUSTOMSORTMODES = 10,
	MCT_ADDMACRO           = 11,
	MCT_KEYMACRO           = 12,
	MCT_CANPANELSORT       = 13,
};

enum MACROPLUGINRETURNTYPE
{
	MPRT_NORMALFINISH,
	MPRT_ERRORFINISH,
	MPRT_ERRORPARSE,
	MPRT_KEYS,
	MPRT_PRINT,
	MPRT_PLUGINCALL,
	MPRT_PLUGINMENU,
	MPRT_PLUGINCONFIG,
	MPRT_PLUGINCOMMAND,
	MPRT_USERMENU,
	MPRT_HASNOMACRO,

	MPRT_FILEASSOCIATIONS,
	MPRT_FILEHIGHLIGHT,
	MPRT_FILEPANELMODES,
	MPRT_FOLDERSHORTCUTS,
	MPRT_FILEMASKGROUPS,
};

struct OpenMacroPluginInfo
{
	enum MACROCALLTYPE CallType;
	struct FarMacroCall *Data;
	struct MacroPluginReturn Ret;
};

struct OpenMacroInfo
{
	size_t StructSize;
	size_t Count;
	struct FarMacroValue *Values;
};

typedef int64_t (__stdcall *FARAPICALLFAR)(int CheckCode, struct FarMacroCall* Data);

struct MacroPrivateInfo
{
	size_t StructSize;
	FARAPICALLFAR CallFar;
};

enum FARCOLORFLAGS
{
	FCLR_REDRAW                 = 0x00000001,
};

struct FarSetColors
{
	DWORD Flags;
	int StartIndex;
	int ColorCount;
	uint64_t *Colors;
};

struct FarTrueColor
{
	unsigned char R;
	unsigned char G;
	unsigned char B;
	unsigned char Flags; // bit one - 'active' flag, others - ignored and must be set to zero
};

struct FarTrueColorForeAndBack
{
	struct FarTrueColor Fore;
	struct FarTrueColor Back;
};

struct DialogItemTrueColors
{
	struct FarTrueColorForeAndBack Normal;
	struct FarTrueColorForeAndBack Hilighted;
	struct FarTrueColorForeAndBack Frame;
	struct FarTrueColorForeAndBack Reserved;
};

enum FARCOLORDIALOGFLAGS
{
	FCD_RGB             = 0x00000001,
	FCD_FONTSTYLES      = 0x00000002,
	FCD_USEMASK         = 0x00000004,
	FCD_ALLCONTROLS     = (FCD_RGB | FCD_FONTSTYLES | FCD_USEMASK),
};

struct ColorDialogData
{
	DWORD Flags;
	uint64_t Color;
	uint64_t Mask;
};

typedef void (__stdcall *FARAPITEXTV2)(
	int X,
	int Y,
	const struct ColorDialogData *Color,
	const wchar_t *Str
);

enum WINDOWINFO_TYPE
{
	WTYPE_PANELS=1,
	WTYPE_VIEWER,
	WTYPE_EDITOR,
	WTYPE_DIALOG,
	WTYPE_VMENU,
	WTYPE_HELP,
};

enum WINDOWINFO_FLAGS
{
	WIF_MODIFIED = 0x0001,
	WIF_CURRENT  = 0x0002,
	WIF_MODAL    = 0x0004,
	WIF_NONE     = 0,
};

struct WindowInfo
{
	int  Pos;
	int  Type;
	int  Modified;
	int  Current;
	wchar_t *TypeName;
	int TypeNameSize;
	wchar_t *Name;
	int NameSize;
	DWORD Flags;
	intptr_t Id;
};

enum PROGRESSTATE
{
	PGS_NOPROGRESS   =0x0,
	PGS_INDETERMINATE=0x1,
	PGS_NORMAL       =0x2,
	PGS_ERROR        =0x4,
	PGS_PAUSED       =0x8,
};

struct PROGRESSVALUE
{
	uint64_t Completed;
	uint64_t Total;
};

typedef INT_PTR(__stdcall *FARAPIADVCONTROL)(
	INT_PTR ModuleNumber,
	int Command,
	void *Param1,
	void *Param2
);

enum VIEWER_CONTROL_COMMANDS
{
	VCTL_GETINFO,
	VCTL_QUIT,
	VCTL_REDRAW,
	VCTL_SETKEYBAR,
	VCTL_SETPOSITION,
	VCTL_SELECT,
	VCTL_SETMODE,
	VCTL_GETFILENAME,
};

enum VIEWER_OPTIONS
{
	VOPT_SAVEFILEPOSITION=1,
	VOPT_AUTODETECTCODEPAGE=2,
};

enum VIEWER_SETMODE_TYPES
{
	VSMT_HEX,
	VSMT_WRAP,
	VSMT_WORDWRAP,
};

enum VIEWER_SETMODEFLAGS_TYPES
{
	VSMFL_REDRAW    = 0x00000001,
};

struct ViewerSetMode
{
	int Type;
	union
	{
		int iParam;
		wchar_t *wszParam;
	} Param;
	DWORD Flags;
	DWORD Reserved;
};

struct ViewerSelect
{
	int64_t BlockStartPos;
	int     BlockLen;
};

enum VIEWER_SETPOS_FLAGS
{
	VSP_NOREDRAW    = 0x0001,
	VSP_PERCENT     = 0x0002,
	VSP_RELATIVE    = 0x0004,
	VSP_NORETNEWPOS = 0x0008,
};

struct ViewerSetPosition
{
	DWORD Flags;
	int64_t StartPos;
	int64_t LeftPos;
};

struct ViewerMode
{
	UINT CodePage;
	int Wrap;
	int WordWrap;
	int Hex;
	int Processed;
	DWORD Reserved[3];
};

struct ViewerInfo
{
	int    StructSize;
	int    ViewerID;
	const wchar_t *FileName;
	int64_t FileSize;
	int64_t FilePos;
	int    WindowSizeX;
	int    WindowSizeY;
	DWORD  Options;
	int    TabSize;
	struct ViewerMode CurMode;
	int64_t LeftPos;
};

typedef int (__stdcall *FARAPIVIEWERCONTROL)(
	int Command,
	void *Param
);

typedef int (__stdcall *FARAPIVIEWERCONTROLV2)(
	int ViewerID,
	int Command,
	void *Param
);

enum VIEWER_EVENTS
{
	VE_READ       =0,
	VE_CLOSE      =1,

	VE_GOTFOCUS   =6,
	VE_KILLFOCUS  =7,
};


enum EDITOR_EVENTS
{
	EE_READ       =0,
	EE_SAVE       =1,
	EE_REDRAW     =2,
	EE_CLOSE      =3,

	EE_GOTFOCUS   =6,
	EE_KILLFOCUS  =7,
};

enum DIALOG_EVENTS
{
	DE_DLGPROCINIT    =0,
	DE_DEFDLGPROCINIT =1,
	DE_DLGPROCEND     =2,
};

enum SYNCHRO_EVENTS
{
	SE_COMMONSYNCHRO  =0,
};

enum EDITOR_CONTROL_COMMANDS
{
	ECTL_GETSTRING,
	ECTL_SETSTRING,
	ECTL_INSERTSTRING,
	ECTL_DELETESTRING,
	ECTL_DELETECHAR,
	ECTL_INSERTTEXT,
	ECTL_GETINFO,
	ECTL_SETPOSITION,
	ECTL_SELECT,
	ECTL_REDRAW,
	ECTL_TABTOREAL,
	ECTL_REALTOTAB,
	ECTL_EXPANDTABS,
	ECTL_SETTITLE,
	ECTL_READINPUT,
	ECTL_PROCESSINPUT,
	ECTL_ADDCOLOR,
	ECTL_GETCOLOR,
	ECTL_SAVEFILE,
	ECTL_QUIT,
	ECTL_SETKEYBAR,
	ECTL_PROCESSKEY,
	ECTL_SETPARAM,
	ECTL_GETBOOKMARKS,
	ECTL_TURNOFFMARKINGBLOCK,
	ECTL_DELETEBLOCK,
	ECTL_ADDSTACKBOOKMARK,
	ECTL_PREVSTACKBOOKMARK,
	ECTL_NEXTSTACKBOOKMARK,
	ECTL_CLEARSTACKBOOKMARKS,
	ECTL_DELETESTACKBOOKMARK,
	ECTL_GETSTACKBOOKMARKS,
	ECTL_UNDOREDO,
	ECTL_GETFILENAME,
	ECTL_ADDTRUECOLOR,
	ECTL_GETTRUECOLOR,

	ECTL_RESERVED1,
	ECTL_RESERVED2,
	ECTL_GETTITLE,
	ECTL_INSERTTEXT_V2,
};

enum EDITOR_SETPARAMETER_TYPES
{
	ESPT_TABSIZE,
	ESPT_EXPANDTABS,
	ESPT_AUTOINDENT,
	ESPT_CURSORBEYONDEOL,
	ESPT_CHARCODEBASE,
	ESPT_CODEPAGE,
	ESPT_SAVEFILEPOSITION,
	ESPT_LOCKMODE,
	ESPT_SETWORDDIV,
	ESPT_GETWORDDIV,
	ESPT_SHOWWHITESPACE,
	ESPT_SETBOM,
};



struct EditorSetParameter
{
	int Type;
	union
	{
		int iParam;
		wchar_t *wszParam;
		DWORD Reserved1;
	} Param;
	DWORD Flags;
	DWORD Size;
};


enum EDITOR_UNDOREDO_COMMANDS
{
	EUR_BEGIN,
	EUR_END,
	EUR_UNDO,
	EUR_REDO
};


struct EditorUndoRedo
{
	int Command;
	DWORD_PTR Reserved[3];
};

struct EditorGetString
{
	int StringNumber;
	const wchar_t *StringText;
	const wchar_t *StringEOL;
	int StringLength;
	int SelStart;
	int SelEnd;
};


struct EditorSetString
{
	int StringNumber;
	const wchar_t *StringText;
	const wchar_t *StringEOL;
	int StringLength;
};

enum EXPAND_TABS
{
	EXPAND_NOTABS,
	EXPAND_ALLTABS,
	EXPAND_NEWTABS
};


enum EDITOR_OPTIONS
{
	EOPT_EXPANDALLTABS     = 0x00000001,
	EOPT_PERSISTENTBLOCKS  = 0x00000002,
	EOPT_DELREMOVESBLOCKS  = 0x00000004,
	EOPT_AUTOINDENT        = 0x00000008,
	EOPT_SAVEFILEPOSITION  = 0x00000010,
	EOPT_AUTODETECTCODEPAGE= 0x00000020,
	EOPT_CURSORBEYONDEOL   = 0x00000040,
	EOPT_EXPANDONLYNEWTABS = 0x00000080,
	EOPT_SHOWWHITESPACE    = 0x00000100,
	EOPT_BOM               = 0x00000200,
};


enum EDITOR_BLOCK_TYPES
{
	BTYPE_NONE,
	BTYPE_STREAM,
	BTYPE_COLUMN
};

enum EDITOR_CURRENTSTATE
{
	ECSTATE_MODIFIED       = 0x00000001,
	ECSTATE_SAVED          = 0x00000002,
	ECSTATE_LOCKED         = 0x00000004,
};


struct EditorInfo
{
	int EditorID;
	int WindowSizeX;
	int WindowSizeY;
	int TotalLines;
	int CurLine;
	int CurPos;
	int CurTabPos;
	int TopScreenLine;
	int LeftPos;
	int Overtype;
	int BlockType;
	int BlockStartLine;
	DWORD Options;
	int TabSize;
	int BookMarkCount;
	DWORD CurState;
	UINT CodePage;

	DWORD Reserved[4];
	int SessionBookmarkCount;
};

struct EditorBookMarks
{
	long *Line;
	long *Cursor;
	long *ScreenLine;
	long *LeftPos;
	DWORD Reserved[4];
};

struct EditorSetPosition
{
	int CurLine;
	int CurPos;
	int CurTabPos;
	int TopScreenLine;
	int LeftPos;
	int Overtype;
};


struct EditorSelect
{
	int BlockType;
	int BlockStartLine;
	int BlockStartPos;
	int BlockWidth;
	int BlockHeight;
};


struct EditorConvertPos
{
	int StringNumber;
	int SrcPos;
	int DestPos;
};


enum EDITORCOLORFLAGS
{
	ECF_TAB1 = 0x10000,
	ECF_TABMARKFIRST = ECF_TAB1,
	ECF_AUTODELETE = 0x20000,
};

struct EditorColor
{
	int StringNumber;
	int ColorItem;
	int StartPos;
	int EndPos;
	int Color;
};

struct EditorTrueColor
{
	struct EditorColor Base;
	struct FarTrueColorForeAndBack TrueColor;
};

struct EditorSaveFile
{
	const wchar_t *FileName;
	const wchar_t *FileEOL;
	UINT CodePage;
};

typedef int (__stdcall *FARAPIEDITORCONTROL)(
	int Command,
	void *Param
);

typedef int (__stdcall *FARAPIEDITORCONTROLV2)(
	int EditorID,
	int Command,
	void *Param
);

enum INPUTBOXFLAGS
{
	FIB_ENABLEEMPTY      = 0x00000001,
	FIB_PASSWORD         = 0x00000002,
	FIB_EXPANDENV        = 0x00000004,
	FIB_NOUSELASTHISTORY = 0x00000008,
	FIB_BUTTONS          = 0x00000010,
	FIB_NOAMPERSAND      = 0x00000020,
	FIB_EDITPATH         = 0x01000000,
	FIB_NONE             = 0,
};

typedef int (__stdcall *FARAPIINPUTBOX)(
	const wchar_t *Title,
	const wchar_t *SubTitle,
	const wchar_t *HistoryName,
	const wchar_t *SrcText,
	wchar_t *DestText,
	int   DestLength,
	const wchar_t *HelpTopic,
	DWORD Flags
);

typedef int (__stdcall *FARAPIINPUTBOXV3)(
	INT_PTR PluginNumber,
	const GUID *Id,
	const wchar_t *Title,
	const wchar_t *SubTitle,
	const wchar_t *HistoryName,
	const wchar_t *SrcText,
	wchar_t *DestText,
	int   DestLength,
	const wchar_t *HelpTopic,
	DWORD Flags
);

typedef int (__stdcall *FARAPIPLUGINSCONTROL)(
	HANDLE hHandle,
	int Command,
	int Param1,
	LONG_PTR Param2
);

typedef intptr_t (__stdcall *FARAPIPLUGINSCONTROLV3)(
	HANDLE hHandle,
	int Command,
	intptr_t Param1,
	void* Param2
);

typedef int (__stdcall *FARAPIFILEFILTERCONTROL)(
	HANDLE hHandle,
	int Command,
	int Param1,
	LONG_PTR Param2
);

typedef int (__stdcall *FARAPIREGEXPCONTROL)(
	HANDLE hHandle,
	int Command,
	LONG_PTR Param
);

typedef int (__stdcall *FARAPIMACROCONTROL)(
	DWORD PluginId,
	int Command,
	int Param1,
	void* Param2
);

typedef int (__stdcall *FARAPICOLORDIALOG)(
	INT_PTR PluginNumber,
	struct ColorDialogData *Data,
	DWORD Flags
);

struct DetectCodePageInfo
{
	size_t StructSize;
	const wchar_t* FileName;
};

typedef int (__stdcall *FARSTDDETECTCODEPAGE)(struct DetectCodePageInfo* Info);
typedef int (__cdecl *FARSTDSNPRINTF)(wchar_t *Buffer,size_t Sizebuf,const wchar_t *Format,...);
typedef int (__cdecl *FARSTDSSCANF)(const wchar_t *Buffer, const wchar_t *Format,...);
typedef void (__stdcall *FARSTDQSORT)(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));
typedef void (__stdcall *FARSTDQSORTEX)(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *,void *userparam),void *userparam);
typedef void   *(__stdcall *FARSTDBSEARCH)(const void *key, const void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));
typedef int (__stdcall *FARSTDGETFILEOWNER)(const wchar_t *Computer,const wchar_t *Name,wchar_t *Owner,int Size);
typedef int (__stdcall *FARSTDGETFILEGROUP)(const wchar_t *Computer,const wchar_t *Name,wchar_t *Group,int Size);
typedef int (__stdcall *FARSTDGETNUMBEROFLINKS)(const wchar_t *Name);
typedef int (__stdcall *FARSTDATOI)(const wchar_t *s);
typedef int64_t (__stdcall *FARSTDATOI64)(const wchar_t *s);
typedef wchar_t   *(__stdcall *FARSTDITOA64)(int64_t value, wchar_t *string, int radix);
typedef wchar_t   *(__stdcall *FARSTDITOA)(int value, wchar_t *string, int radix);
typedef wchar_t   *(__stdcall *FARSTDLTRIM)(wchar_t *Str);
typedef wchar_t   *(__stdcall *FARSTDRTRIM)(wchar_t *Str);
typedef wchar_t   *(__stdcall *FARSTDTRIM)(wchar_t *Str);
typedef wchar_t   *(__stdcall *FARSTDTRUNCSTR)(wchar_t *Str,int MaxCells);
typedef wchar_t   *(__stdcall *FARSTDTRUNCPATHSTR)(wchar_t *Str,int MaxCells);
typedef wchar_t   *(__stdcall *FARSTDQUOTESPACEONLY)(wchar_t *Str);
typedef const wchar_t*(__stdcall *FARSTDPOINTTONAME)(const wchar_t *Path);
typedef int (__stdcall *FARSTDGETPATHROOT)(const wchar_t *Path,wchar_t *Root, int DestSize);
typedef BOOL (__stdcall *FARSTDADDENDSLASH)(wchar_t *Path);
typedef int (__stdcall *FARSTDCOPYTOCLIPBOARD)(const wchar_t *Data);
typedef wchar_t *(__stdcall *FARSTDPASTEFROMCLIPBOARD)(void);
typedef FarKey (__stdcall *FARSTDINPUTRECORDTOKEY)(const INPUT_RECORD *r);
typedef int (__stdcall *FARSTDLOCALISLOWER)(wchar_t Ch);
typedef int (__stdcall *FARSTDLOCALISUPPER)(wchar_t Ch);
typedef int (__stdcall *FARSTDLOCALISALPHA)(wchar_t Ch);
typedef int (__stdcall *FARSTDLOCALISALPHANUM)(wchar_t Ch);
typedef wchar_t (__stdcall *FARSTDLOCALUPPER)(wchar_t LowerChar);
typedef wchar_t (__stdcall *FARSTDLOCALLOWER)(wchar_t UpperChar);
typedef void (__stdcall *FARSTDLOCALUPPERBUF)(wchar_t *Buf,int Length);
typedef void (__stdcall *FARSTDLOCALLOWERBUF)(wchar_t *Buf,int Length);
typedef void (__stdcall *FARSTDLOCALSTRUPR)(wchar_t *s1);
typedef void (__stdcall *FARSTDLOCALSTRLWR)(wchar_t *s1);
typedef int (__stdcall *FARSTDLOCALSTRICMP)(const wchar_t *s1,const wchar_t *s2);
typedef int (__stdcall *FARSTDLOCALSTRNICMP)(const wchar_t *s1,const wchar_t *s2,int n);
typedef int (__stdcall *FARSTDLOCALSTRCMP)(const wchar_t *s1,const wchar_t *s2);
typedef int (__stdcall *FARSTDLOCALSTRNCMP)(const wchar_t *s1,const wchar_t *s2,int n);

enum PROCESSNAME_FLAGS
{
	PN_CMPNAME          = 0x00000000U,
	PN_CMPNAMELIST      = 0x00010000U,
	PN_GENERATENAME     = 0x00020000U,
	PN_CHECKMASK        = 0x00030000U,
	PN_SKIPPATH         = 0x01000000U,
	PN_SHOWERRORMESSAGE = 0x02000000U,
	PN_RESERVED1        = 0x04000000U,
	PN_CASESENSITIVE    = 0x08000000U,
	PN_NONE             = 0
};

typedef int (__stdcall *FARSTDPROCESSNAME)(const wchar_t *param1, wchar_t *param2, DWORD size, DWORD flags);

typedef void (__stdcall *FARSTDUNQUOTE)(wchar_t *Str);

enum XLATMODE
{
	XLAT_SWITCHKEYBLAYOUT  = 0x00000001U, // unsupported
	XLAT_SWITCHKEYBBEEP    = 0x00000002U, // unsupported
	XLAT_USEKEYBLAYOUTNAME = 0x00000004U, // unsupported
	XLAT_CONVERTALLCMDLINE = 0x00010000U, // deprecated
};

typedef size_t (__stdcall *FARSTDKEYTOKEYNAME)(FarKey Key,wchar_t *KeyText,size_t Size);

typedef wchar_t*(__stdcall *FARSTDXLAT)(wchar_t *Line,int StartPos,int EndPos,DWORD Flags);

typedef FarKey (__stdcall *FARSTDKEYNAMETOKEY)(const wchar_t *Name);

typedef BOOL (__stdcall *FARSTDKEYNAMETOINPUTRECORD)(const wchar_t *Name,INPUT_RECORD* Key);

typedef int (__stdcall *FRSUSERFUNC)(
	const struct FAR_FIND_DATA *FData,
	const wchar_t *FullName,
	void *Param
);

enum FRSMODE
{
	FRS_RETUPDIR             = 0x01,
	FRS_RECUR                = 0x02,
	FRS_SCANSYMLINK          = 0x04,
};

typedef void (__stdcall *FARSTDRECURSIVESEARCH)(const wchar_t *InitDir,const wchar_t *Mask,FRSUSERFUNC Func,DWORD Flags,void *Param);
typedef int (__stdcall *FARSTDMKTEMP)(wchar_t *Dest, DWORD size, const wchar_t *Prefix);
typedef void (__stdcall *FARSTDDELETEBUFFER)(void *Buffer);

enum MKLINKOP
{
	FLINK_HARDLINK         = 1,
	FLINK_JUNCTION         = 2,
	FLINK_VOLMOUNT         = 3,
	FLINK_SYMLINKFILE      = 4,
	FLINK_SYMLINKDIR       = 5,
	FLINK_SYMLINK          = 6,

	FLINK_SHOWERRMSG       = 0x10000,
	FLINK_DONOTUPDATEPANEL = 0x20000,
};
typedef int (__stdcall *FARSTDMKLINK)(const wchar_t *Src,const wchar_t *Dest,DWORD Flags);
typedef int (__stdcall *FARGETREPARSEPOINTINFO)(const wchar_t *Src, wchar_t *Dest,int DestSize);

enum CONVERTPATHMODES
{
	CPM_FULL,
	CPM_REAL,
	CPM_NATIVE,
};

typedef int (__stdcall *FARCONVERTPATH)(enum CONVERTPATHMODES Mode, const wchar_t *Src, wchar_t *Dest, int DestSize);

typedef DWORD (__stdcall *FARGETCURRENTDIRECTORY)(DWORD Size,wchar_t* Buffer);

enum FARFORMATFILESIZEFLAGS
{
	FFFS_COMMAS                 = 0x01000000,
	FFFS_FLOATSIZE              = 0x02000000,
	FFFS_SHOWBYTESINDEX         = 0x04000000,
	FFFS_ECONOMIC               = 0x08000000,
	FFFS_THOUSAND               = 0x10000000,
	FFFS_MINSIZEINDEX           = 0x20000000,
	FFFS_MINSIZEINDEX_MASK      = 0x00000003,
	FFFS_NONE                   = 0,
};

typedef size_t (__stdcall *FARFORMATFILESIZE)(uint64_t Size, int Width, DWORD Flags, wchar_t *Dest, size_t DestSize);

enum EXECUTEFLAGS
{
	EF_HIDEOUT    = 0x01, // dont display output of the command
	EF_NOWAIT     = 0x02, // dont wait for command completion
	EF_SUDO       = 0x04, // command must be run with root privileges
	EF_NOTIFY     = 0x08, // notify when command completed (if such notifications enabled in settings)
	EF_NOCMDPRINT = 0x10, // dont print command in command line nor include it to history
	EF_OPEN       = 0x20, // use desktop shell (if present) to open command (e.g. URLs, documents..)
	EF_MAYBGND    = 0x40  // allow put command to background mode
};

typedef int (__stdcall *FAREXECUTE)(const wchar_t *CmdStr, unsigned int ExecFlags);
typedef int (__stdcall *FAREXECUTE_LIBRARY)(const wchar_t *Library, const wchar_t *Symbol, const wchar_t *CmdStr, unsigned int ExecFlags);
typedef void (__stdcall *FARDISPLAYNOTIFICATION)(const wchar_t *action, const wchar_t *object);
typedef int (__stdcall *FARDISPATCHNTRTHRDCALLS)();
typedef void (__stdcall *FARBACKGROUNDTASK)(const wchar_t *Info, BOOL Started);
typedef size_t (__stdcall *FARSTRCELLSCOUNT)(const wchar_t *Str, size_t CharsCount);
typedef size_t (__stdcall *FARSTRSIZEOFCELLS)(const wchar_t *Str, size_t CharsCount, size_t *CellsCount, BOOL RoundUp);

enum BOX_DEF_SYMBOLS
{
	BS_X_B0,          // 0xB0
	BS_X_B1,          // 0xB1
	BS_X_B2,          // 0xB2
	BS_V1,            // 0xB3
	BS_R_H1V1,        // 0xB4
	BS_R_H2V1,        // 0xB5
	BS_R_H1V2,        // 0xB6
	BS_RT_H1V2,       // 0xB7
	BS_RT_H2V1,       // 0xB8
	BS_R_H2V2,        // 0xB9
	BS_V2,            // 0xBA
	BS_RT_H2V2,       // 0xBB
	BS_RB_H2V2,       // 0xBC
	BS_RB_H1V2,       // 0xBD
	BS_RB_H2V1,       // 0xBE
	BS_RT_H1V1,       // 0xBF
	BS_LB_H1V1,       // 0xC0
	BS_B_H1V1,        // 0xC1
	BS_T_H1V1,        // 0xC2
	BS_L_H1V1,        // 0xC3
	BS_H1,            // 0xC4
	BS_C_H1V1,        // 0xC5
	BS_L_H2V1,        // 0xC6
	BS_L_H1V2,        // 0xC7
	BS_LB_H2V2,       // 0xC8
	BS_LT_H2V2,       // 0xC9
	BS_B_H2V2,        // 0xCA
	BS_T_H2V2,        // 0xCB
	BS_L_H2V2,        // 0xCC
	BS_H2,            // 0xCD
	BS_C_H2V2,        // 0xCE
	BS_B_H2V1,        // 0xCF
	BS_B_H1V2,        // 0xD0
	BS_T_H2V1,        // 0xD1
	BS_T_H1V2,        // 0xD2
	BS_LB_H1V2,       // 0xD3
	BS_LB_H2V1,       // 0xD4
	BS_LT_H2V1,       // 0xD5
	BS_LT_H1V2,       // 0xD6
	BS_C_H1V2,        // 0xD7
	BS_C_H2V1,        // 0xD8
	BS_RB_H1V1,       // 0xD9
	BS_LT_H1V1,       // 0xDA
	BS_X_DB,          // 0xDB
	BS_X_DC,          // 0xDC
	BS_X_DD,          // 0xDD
	BS_X_DE,          // 0xDE
	BS_X_DF,          // 0xDF
};


typedef struct FarStandardFunctions
{
	int StructSize;

	FARSTDATOI                 atoi;
	FARSTDATOI64               atoi64;
	FARSTDITOA                 itoa;
	FARSTDITOA64               itoa64;
	FARSTDSSCANF               sscanf;
	FARSTDQSORT                qsort;
	FARSTDBSEARCH              bsearch;
	FARSTDQSORTEX              qsortex;
	FARSTDSNPRINTF             snprintf;

	DWORD_PTR                  Reserved[7];
	const WCHAR *              BoxSymbols; // indexed via BOX_DEF_SYMBOLS

	FARSTDLOCALISLOWER         LIsLower;
	FARSTDLOCALISUPPER         LIsUpper;
	FARSTDLOCALISALPHA         LIsAlpha;
	FARSTDLOCALISALPHANUM      LIsAlphanum;
	FARSTDLOCALUPPER           LUpper;
	FARSTDLOCALLOWER           LLower;
	FARSTDLOCALUPPERBUF        LUpperBuf;
	FARSTDLOCALLOWERBUF        LLowerBuf;
	FARSTDLOCALSTRUPR          LStrupr;
	FARSTDLOCALSTRLWR          LStrlwr;
	FARSTDLOCALSTRICMP         LStricmp;
	FARSTDLOCALSTRNICMP        LStrnicmp;

	FARSTDUNQUOTE              Unquote;
	FARSTDLTRIM                LTrim;
	FARSTDRTRIM                RTrim;
	FARSTDTRIM                 Trim;
	FARSTDTRUNCSTR             TruncStr;
	FARSTDTRUNCPATHSTR         TruncPathStr;
	FARSTDQUOTESPACEONLY       QuoteSpaceOnly;
	FARSTDPOINTTONAME          PointToName;
	FARSTDGETPATHROOT          GetPathRoot;
	FARSTDADDENDSLASH          AddEndSlash;
	FARSTDCOPYTOCLIPBOARD      CopyToClipboard;
	FARSTDPASTEFROMCLIPBOARD   PasteFromClipboard;
	FARSTDKEYTOKEYNAME         FarKeyToName;
	FARSTDKEYNAMETOKEY         FarNameToKey;
	FARSTDINPUTRECORDTOKEY     FarInputRecordToKey;
	FARSTDXLAT                 XLat;
	FARSTDGETFILEOWNER         GetFileOwner;
	FARSTDGETNUMBEROFLINKS     GetNumberOfLinks;
	FARSTDRECURSIVESEARCH      FarRecursiveSearch;
	FARSTDMKTEMP               MkTemp;
	FARSTDDELETEBUFFER         DeleteBuffer;
	FARSTDPROCESSNAME          ProcessName;
	FARSTDMKLINK               MkLink;
	FARCONVERTPATH             ConvertPath;
	FARGETREPARSEPOINTINFO     GetReparsePointInfo;
	FARGETCURRENTDIRECTORY     GetCurrentDirectory;
	FAREXECUTE                 Execute;
	FAREXECUTE_LIBRARY         ExecuteLibrary;
	FARDISPLAYNOTIFICATION     DisplayNotification;
	FARDISPATCHNTRTHRDCALLS    DispatchInterThreadCalls;
	FARBACKGROUNDTASK          BackgroundTask;
	FARSTRCELLSCOUNT           StrCellsCount;
	FARSTRSIZEOFCELLS          StrSizeOfCells;
	void*                      RESERVED[2];

	FARSTDDETECTCODEPAGE       DetectCodePage;
	FARSTDKEYNAMETOINPUTRECORD FarNameToInputRecord;
	FARSTDGETFILEGROUP         GetFileGroup;
	FARFORMATFILESIZE          FormatFileSize;
	FARSTDLOCALSTRICMP         LStrcmp;
	FARSTDLOCALSTRNICMP        LStrncmp;
} FARSTANDARDFUNCTIONS;

struct PluginStartupInfo
{
	int StructSize;
	const wchar_t *ModuleName;
	INT_PTR ModuleNumber;
	const wchar_t *RootKey;
	FARAPIMENU             Menu;
	FARAPIMESSAGE          Message;
	FARAPIGETMSG           GetMsg;
	FARAPICONTROL          Control;
	FARAPISAVESCREEN       SaveScreen;
	FARAPIRESTORESCREEN    RestoreScreen;
	FARAPIGETDIRLIST       GetDirList;
	FARAPIGETPLUGINDIRLIST GetPluginDirList;
	FARAPIFREEDIRLIST      FreeDirList;
	FARAPIFREEPLUGINDIRLIST FreePluginDirList;
	FARAPIVIEWER           Viewer;
	FARAPIEDITOR           Editor;
	FARAPICMPNAME          CmpName;
	FARAPITEXT             Text;
	FARAPIEDITORCONTROL    EditorControl;

	FARSTANDARDFUNCTIONS  *FSF;

	FARAPISHOWHELP         ShowHelp;
	FARAPIADVCONTROL       AdvControl;
	FARAPIINPUTBOX         InputBox;
	FARAPIDIALOGINIT       DialogInit;
	FARAPIDIALOGRUN        DialogRun;
	FARAPIDIALOGFREE       DialogFree;

	FARAPISENDDLGMESSAGE   SendDlgMessage;
	FARAPIDEFDLGPROC       DefDlgProc;
	DWORD_PTR              Reserved;
	FARAPIVIEWERCONTROL    ViewerControl;
	FARAPIPLUGINSCONTROL   PluginsControl;
	FARAPIFILEFILTERCONTROL FileFilterControl;
	FARAPIREGEXPCONTROL    RegExpControl;

	void*                  RESERVED[2];
	FARAPIMACROCONTROL     MacroControl;
	FARAPIEDITORCONTROLV2  EditorControlV2;
	FARAPIVIEWERCONTROLV2  ViewerControlV2;
	FARAPIPLUGINSCONTROLV3 PluginsControlV3;
	FARAPICOLORDIALOG      ColorDialog;
	FARAPIFREESCREEN       FreeScreen;
	const void*            Private;
	int                    LuafarLoaded;
	FARAPIDIALOGINITV3     DialogInitV3;
	FARAPITEXTV2           TextV2;
	FARAPIMESSAGEV3        MessageV3;
	FARAPIMENUV2           MenuV2;
	FARAPIINPUTBOXV3       InputBoxV3;
};


enum PLUGIN_FLAGS
{
	PF_PRELOAD        = 0x0001, // early dlopen and initialize plugin
	PF_DISABLEPANELS  = 0x0002,
	PF_EDITOR         = 0x0004,
	PF_VIEWER         = 0x0008,
	PF_FULLCMDLINE    = 0x0010,
	PF_DIALOG         = 0x0020,
	PF_PREOPEN        = 0x8000 // early dlopen plugin but initialize it later, when it will be really needed
};

struct PluginInfo
{
	int StructSize;
	DWORD Flags;
	const wchar_t * const *DiskMenuStrings;
	int *Reserved0;
	int DiskMenuStringsNumber;
	const wchar_t * const *PluginMenuStrings;
	int PluginMenuStringsNumber;
	const wchar_t * const *PluginConfigStrings;
	int PluginConfigStringsNumber;
	const wchar_t *CommandPrefix;
	DWORD SysID;
	const GUID *DiskMenuGuids;
	const GUID *PluginMenuGuids;
	const GUID *PluginConfigGuids;
};



struct InfoPanelLine
{
	const wchar_t *Text;
	const wchar_t *Data;
	int  Separator;
};

struct PanelMode
{
	const wchar_t *ColumnTypes;
	const wchar_t *ColumnWidths;
	const wchar_t * const *ColumnTitles;
	int    FullScreen;
	int    DetailedStatus;
	int    AlignExtensions;
	int    CaseConversion;
	const wchar_t *StatusColumnTypes;
	const wchar_t *StatusColumnWidths;
	DWORD  Reserved[2];
};


enum OPENPLUGININFO_FLAGS
{
	OPIF_USEFILTER               = 0x00000001,
	OPIF_USESORTGROUPS           = 0x00000002,
	OPIF_USEHIGHLIGHTING         = 0x00000004,
	OPIF_ADDDOTS                 = 0x00000008,
	OPIF_RAWSELECTION            = 0x00000010,
	OPIF_REALNAMES               = 0x00000020,
	OPIF_SHOWNAMESONLY           = 0x00000040,
	OPIF_SHOWRIGHTALIGNNAMES     = 0x00000080,
	OPIF_SHOWPRESERVECASE        = 0x00000100,
	OPIF_COMPAREFATTIME          = 0x00000400,
	OPIF_EXTERNALGET             = 0x00000800,
	OPIF_EXTERNALPUT             = 0x00001000,
	OPIF_EXTERNALDELETE          = 0x00002000,
	OPIF_EXTERNALMKDIR           = 0x00004000,
	OPIF_USEATTRHIGHLIGHTING     = 0x00008000,
	OPIF_USECRC32                = 0x00010000,
	OPIF_HL_MARKERS_NOSHOW       = 0x00020000,
	OPIF_HL_MARKERS_NOALIGN      = 0x00040000,
};


enum OPENPLUGININFO_SORTMODES
{
	SM_DEFAULT,
	SM_UNSORTED,
	SM_NAME,
	SM_EXT,
	SM_MTIME,
	SM_CTIME,
	SM_ATIME,
	SM_SIZE,
	SM_DESCR,
	SM_OWNER,
	SM_COMPRESSEDSIZE,
	SM_NUMLINKS,
	SM_FULLNAME,
	SM_CHTIME,

	SM_COUNT,

	SM_USER = 100000
};


struct KeyBarTitles
{
	wchar_t *Titles[12];
	wchar_t *CtrlTitles[12];
	wchar_t *AltTitles[12];
	wchar_t *ShiftTitles[12];

	wchar_t *CtrlShiftTitles[12];
	wchar_t *AltShiftTitles[12];
	wchar_t *CtrlAltTitles[12];
};


enum OPERATION_MODES
{
	OPM_SILENT     =0x0001,
	OPM_FIND       =0x0002,
	OPM_VIEW       =0x0004,
	OPM_EDIT       =0x0008,
	OPM_TOPLEVEL   =0x0010,
	OPM_DESCR      =0x0020,
	OPM_QUICKVIEW  =0x0040,
	OPM_PGDN       =0x0080,
	OPM_COMMANDS   =0x0100,
};

struct OpenPluginInfo
{
	int                           StructSize;
	DWORD                         Flags;
	const wchar_t*                HostFile;
	const wchar_t*                CurDir;
	const wchar_t*                Format;
	const wchar_t*                PanelTitle;
	const struct InfoPanelLine*   InfoLines;
	int                           InfoLinesNumber;
	const wchar_t* const*         DescrFiles;
	int                           DescrFilesNumber;
	const struct PanelMode*       PanelModesArray;
	int                           PanelModesNumber;
	int                           StartPanelMode;
	int                           StartSortMode;
	int                           StartSortOrder;
	const struct KeyBarTitles*    KeyBar;
	const wchar_t*                ShortcutData;
	long                          Reserved;
};

enum OPENPLUGIN_OPENFROM
{
	OPEN_FROM_MASK          = 0x000000FF,

	OPEN_DISKMENU           = 0,
	OPEN_PLUGINSMENU        = 1,
	OPEN_FINDLIST           = 2,
	OPEN_SHORTCUT           = 3,
	OPEN_COMMANDLINE        = 4,
	OPEN_EDITOR             = 5,
	OPEN_VIEWER             = 6,
	OPEN_FILEPANEL          = 7,
	OPEN_DIALOG             = 8,
	OPEN_ANALYSE            = 9,

	OPEN_FROMMACRO          = 12,
	OPEN_LUAMACRO           = 100,
};

enum FAR_PKF_FLAGS
{
	PKF_CONTROL     = 0x00000001,
	PKF_ALT         = 0x00000002,
	PKF_SHIFT       = 0x00000004,
	PKF_PREPROCESS  = 0x00080000, // for "Key", function ProcessKey()
};

enum FAR_EVENTS
{
	FE_CHANGEVIEWMODE =0,
	FE_REDRAW         =1,
	FE_IDLE           =2,
	FE_CLOSE          =3,
	FE_BREAK          =4,
	FE_COMMAND        =5,

	FE_GOTFOCUS       =6,
	FE_KILLFOCUS      =7,
};

enum FAR_PLUGINS_CONTROL_COMMANDS
{
	PCTL_LOADPLUGIN         = 0,
	PCTL_UNLOADPLUGIN       = 1,
	PCTL_FORCEDLOADPLUGIN   = 2,

	PCTL_CACHEFORGET        = 3, // forgets cached information for specified plugin

	PCTL_FINDPLUGIN           = 4,
	PCTL_GETPLUGININFORMATION = 5,
	PCTL_GETPLUGINS           = 6,
};

enum FAR_PLUGIN_LOAD_TYPE
{
	PLT_PATH = 0,
};

enum FAR_PLUGIN_FIND_TYPE
{
	PFM_SYSID      = 0,
	PFM_MODULENAME = 1,
};

enum FAR_PLUGIN_FLAGS
{
	FPF_LOADED     = 0x00000001,
	FPF_ANSI       = 0x10000000,
	FPF_NONE       = 0,
};

enum FAR_FILE_FILTER_CONTROL_COMMANDS
{
	FFCTL_CREATEFILEFILTER = 0,
	FFCTL_FREEFILEFILTER,
	FFCTL_OPENFILTERSMENU,
	FFCTL_STARTINGTOFILTER,
	FFCTL_ISFILEINFILTER,
};

enum FAR_FILE_FILTER_TYPE
{
	FFT_PANEL = 0,
	FFT_FINDFILE,
	FFT_COPY,
	FFT_SELECT,
	FFT_CUSTOM,
};

enum FAR_REGEXP_CONTROL_COMMANDS
{
	RECTL_CREATE=0,
	RECTL_FREE,
	RECTL_COMPILE,
	RECTL_OPTIMIZE,
	RECTL_MATCHEX,
	RECTL_SEARCHEX,
	RECTL_BRACKETSCOUNT
};

struct RegExpMatch
{
	int start,end;
};

struct RegExpSearch
{
	const wchar_t* Text;
	int Position;
	int Length;
	struct RegExpMatch* Match;
	int Count;
	void* Reserved;
};

struct VersionInfo
{
	DWORD Major;
	DWORD Minor;
	DWORD Revision;
	DWORD Build;
};

struct GlobalInfo
{
	size_t StructSize;
	DWORD SysID;
	struct VersionInfo Version;
	const wchar_t *Title;
	const wchar_t *Description;
	const wchar_t *Author;
};

struct FarGetPluginInformation
{
	size_t StructSize;
	const wchar_t *ModuleName;
	DWORD Flags;
	struct PluginInfo *PInfo;
	struct GlobalInfo *GInfo;
};

struct AnalyseData
{
	size_t          StructSize;
	const wchar_t  *FileName;
	const void     *Buffer;
	size_t          BufferSize;
	int             OpMode;
};

struct ConfigureInfo
{
	size_t StructSize;
	const GUID* Guid;
};
	void   __stdcall  PluginModuleOpen(const char *path);
	void   __stdcall  ClosePluginW(HANDLE hPlugin);
	int    __stdcall  CompareW(HANDLE hPlugin,const struct PluginPanelItem *Item1,const struct PluginPanelItem *Item2,unsigned int Mode);
	int    __stdcall  ConfigureW(int ItemNumber);
	int    __stdcall  ConfigureV3W(const struct ConfigureInfo *Info);
	int    __stdcall  DeleteFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
	void   __stdcall  ExitFARW(void);
	int    __stdcall  MayExitFARW(void);
	void   __stdcall  FreeFindDataW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber);
	void   __stdcall  FreeVirtualFindDataW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber);
	int    __stdcall  GetFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t **DestPath,int OpMode);
	int    __stdcall  GetFindDataW(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
	int    __stdcall  GetLinkTargetW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,wchar_t *Target,size_t TargetSize,int OpMode);
	int    __stdcall  GetMinFarVersionW(void);
	void   __stdcall  GetOpenPluginInfoW(HANDLE hPlugin,struct OpenPluginInfo *Info);
	void   __stdcall  GetPluginInfoW(struct PluginInfo *Info);
	int    __stdcall  GetVirtualFindDataW(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,const wchar_t *Path);
	int    __stdcall  MakeDirectoryW(HANDLE hPlugin,const wchar_t **Name,int OpMode);
	HANDLE __stdcall  OpenFilePluginW(const wchar_t *Name,const unsigned char *Data,int DataSize,int OpMode);
	HANDLE __stdcall  OpenPluginW(int OpenFrom,INT_PTR Item);
	int    __stdcall  ProcessDialogEventW(int Event,void *Param);
	int    __stdcall  ProcessEditorEventW(int Event,void *Param);
	int    __stdcall  ProcessEditorInputW(const INPUT_RECORD *Rec);
	int    __stdcall  ProcessEventW(HANDLE hPlugin,int Event,void *Param);
	int    __stdcall  ProcessHostFileW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
	int    __stdcall  ProcessKeyW(HANDLE hPlugin,int Key,unsigned int ControlState);
	int    __stdcall  ProcessSynchroEventW(int Event,void *Param);
	int    __stdcall  ProcessViewerEventW(int Event,void *Param);
	int    __stdcall  PutFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t *SrcPath,int OpMode);
	int    __stdcall  SetDirectoryW(HANDLE hPlugin,const wchar_t *Dir,int OpMode);
	int    __stdcall  SetFindListW(HANDLE hPlugin,const struct PluginPanelItem *PanelItem,int ItemsNumber);
	void   __stdcall  SetStartupInfoW(const struct PluginStartupInfo *Info);
	void   __stdcall  GetGlobalInfoW(struct GlobalInfo *Info);
	int    __stdcall  ProcessConsoleInputW(INPUT_RECORD *Rec);
	int    __stdcall  AnalyseW(const struct AnalyseData *pData);

]=]
