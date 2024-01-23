return {
  MCODE_F_NOFUNC=0x380C00;
  MCODE_F_ABS=0x380C01; -- N=abs(N)
  MCODE_F_AKEY=0x380C02; -- V=akey(Mode[,Type])
  MCODE_F_ASC=0x380C03; -- N=asc(S)
  MCODE_F_ATOI=0x380C04; -- N=atoi(S[,radix])
  MCODE_F_CLIP=0x380C05; -- V=clip(N[,V])
  MCODE_F_DATE=0x380C06; -- S=date([S])
  MCODE_F_DLG_GETVALUE=0x380C07; -- V=Dlg.GetValue(ID,N)
  MCODE_F_EDITOR_SEL=0x380C08; -- V=Editor.Sel(Action[,Opt])
  MCODE_F_EDITOR_SET=0x380C09; -- N=Editor.Set(N,Var)
  MCODE_F_EDITOR_UNDO=0x380C0A; -- V=Editor.Undo(N)
  MCODE_F_EDITOR_POS=0x380C0B; -- N=Editor.Pos(Op,What[,Where])
  MCODE_F_EDITOR_DELLINE=0x380C0C; -- N=Editor.DelLine([Line])
  MCODE_F_EDITOR_INSSTR=0x380C0D; -- N=Editor.InsStr([S[,Line]])
  MCODE_F_ENVIRON=0x380C0E; -- S=env(S)
  MCODE_F_FATTR=0x380C0F; -- N=fattr(S)
  MCODE_F_FEXIST=0x380C10; -- S=fexist(S)
  MCODE_F_FSPLIT=0x380C11; -- S=fsplit(S,N)
  MCODE_F_FMATCH=0x380C12; -- N=FMatch(S,Mask)
  MCODE_F_IIF=0x380C13; -- V=iif(C,V1,V2)
  MCODE_F_INDEX=0x380C14; -- S=index(S1,S2[,Mode])
  MCODE_F_INT=0x380C15; -- N=int(V)
  MCODE_F_ITOA=0x380C16; -- S=itoa(N[,radix])
  MCODE_F_KEY=0x380C17; -- S=key(V)
  MCODE_F_LCASE=0x380C18; -- S=lcase(S1)
  MCODE_F_LEN=0x380C19; -- N=len(S)
  MCODE_F_MAX=0x380C1A; -- N=max(N1,N2)
  MCODE_F_MENU_CHECKHOTKEY=0x380C1B; -- N=checkhotkey(S[,N])
  MCODE_F_MENU_GETHOTKEY=0x380C1C; -- S=gethotkey([N])
  MCODE_F_MENU_SELECT=0x380C1D; -- N=Menu.Select(S[,N[,Dir]])
  MCODE_F_MIN=0x380C1E; -- N=min(N1,N2)
  MCODE_F_MOD=0x380C1F; -- N=mod(a,b) == a %  b
  MCODE_F_MLOAD=0x380C20; -- B=mload(var)
  MCODE_F_MSAVE=0x380C21; -- B=msave(var)
  MCODE_F_MSGBOX=0x380C22; -- N=msgbox(["Title"[,"Text"[,flags]]])
  MCODE_F_PANEL_FATTR=0x380C23; -- N=Panel.FAttr(panelType,fileMask)
  MCODE_F_PANEL_SETPATH=0x380C24; -- N=panel.SetPath(panelType,pathName[,fileName])
  MCODE_F_PANEL_FEXIST=0x380C25; -- N=Panel.FExist(panelType,fileMask)
  MCODE_F_PANEL_SETPOS=0x380C26; -- N=Panel.SetPos(panelType,fileName)
  MCODE_F_PANEL_SETPOSIDX=0x380C27; -- N=Panel.SetPosIdx(panelType,Idx[,InSelection])
  MCODE_F_PANEL_SELECT=0x380C28; -- V=Panel.Select(panelType,Action[,Mode[,Items]])
  MCODE_F_PANELITEM=0x380C29; -- V=Panel.Item(Panel,Index,TypeInfo)
  MCODE_F_EVAL=0x380C2A; -- N=eval(S[,N])
  MCODE_F_RINDEX=0x380C2B; -- S=rindex(S1,S2[,Mode])
  MCODE_F_SLEEP=0x380C2C; -- Sleep(N)
  MCODE_F_STRING=0x380C2D; -- S=string(V)
  MCODE_F_SUBSTR=0x380C2E; -- S=substr(S,start[,length])
  MCODE_F_UCASE=0x380C2F; -- S=ucase(S1)
  MCODE_F_WAITKEY=0x380C30; -- V=waitkey([N,[T]])
  MCODE_F_XLAT=0x380C31; -- S=xlat(S)
  MCODE_F_FLOCK=0x380C32; -- N=FLock(N,N)
  MCODE_F_CALLPLUGIN=0x380C33; -- V=callplugin(SysID[,param])
  MCODE_F_REPLACE=0x380C34; -- S=replace(sS,sF,sR[,Count[,Mode]])
  MCODE_F_PROMPT=0x380C35; -- S=prompt("Title"[,"Prompt"[,flags[, "Src"[, "History"]]]])
  MCODE_F_BM_ADD=0x380C36; -- N=BM.Add()  - добавить текущие координаты и обрезать хвост
  MCODE_F_BM_CLEAR=0x380C37; -- N=BM.Clear() - очистить все закладки
  MCODE_F_BM_DEL=0x380C38; -- N=BM.Del([Idx]) - удаляет закладку с указанным индексом (x=1...), 0 - удаляет текущую закладку
  MCODE_F_BM_GET=0x380C39; -- N=BM.Get(Idx,M) - возвращает координаты строки (M==0) или колонки (M==1) закладки с индексом (Idx=1...)
  MCODE_F_BM_GOTO=0x380C3A; -- N=BM.Goto([n]) - переход на закладку с указанным индексом (0 --> текущую)
  MCODE_F_BM_NEXT=0x380C3B; -- N=BM.Next() - перейти на следующую закладку
  MCODE_F_BM_POP=0x380C3C; -- N=BM.Pop() - восстановить текущую позицию из закладки в конце стека и удалить закладку
  MCODE_F_BM_PREV=0x380C3D; -- N=BM.Prev() - перейти на предыдущую закладку
  MCODE_F_BM_BACK=0x380C3E; -- N=BM.Back() - перейти на предыдущую закладку с возможным сохранением текущей позиции
  MCODE_F_BM_PUSH=0x380C3F; -- N=BM.Push() - сохранить текущую позицию в виде закладки в конце стека
  MCODE_F_BM_STAT=0x380C40; -- N=BM.Stat([M]) - возвращает информацию о закладках, N=0 - текущее количество закладок	MCODE_F_TRIM,                     // S=trim(S[,N])
  MCODE_F_TRIM=0x380C41; -- S=trim(S[,N])
  MCODE_F_FLOAT=0x380C42; -- N=float(V)
  MCODE_F_TESTFOLDER=0x380C43; -- N=testfolder(S)
  MCODE_F_PRINT=0x380C44; -- N=Print(Str)
  MCODE_F_MMODE=0x380C45; -- N=MMode(Action[,Value])
  MCODE_F_EDITOR_SETTITLE=0x380C46; -- N=Editor.SetTitle([Title])
  MCODE_F_MENU_GETVALUE=0x380C47; -- S=Menu.GetValue([N])
  MCODE_F_MENU_ITEMSTATUS=0x380C48; -- N=Menu.ItemStatus([N])
  MCODE_F_MENU_FILTER=0x380C49; -- N=Menu.Filter(Action[,Mode])
  MCODE_F_MENU_FILTERSTR=0x380C4A; -- S=Menu.FilterStr([Action[,S]])
  MCODE_F_BEEP=0x380C4B; -- N=beep([N])
  MCODE_F_KBDLAYOUT=0x380C4C; -- N=kbdLayout([N])
  MCODE_F_WINDOW_SCROLL=0x380C4D; -- N=Window.Scroll(Lines[,Axis])
  MCODE_F_CHECKALL=0x380C4E; -- B=CheckAll(Area,Flags[,Callback[,CallbackId]])
  MCODE_F_GETOPTIONS=0x380C4F; -- N=GetOptions()
  MCODE_F_USERMENU=0x380C50; -- UserMenu([Param])
  MCODE_F_SETCUSTOMSORTMODE=0x380C51;
  MCODE_F_KEYMACRO=0x380C52;
  MCODE_F_FAR_GETCONFIG=0x380C53;
  MCODE_F_MACROSETTINGS=0x380C54;
  MCODE_F_SIZE2STR=0x380C55; -- S=Size2Str(Size,Flags[,Width])
  MCODE_F_STRWRAP=0x380C56; -- S=StrWrap(Text,Width[,Break[,Flags]])
  MCODE_F_DLG_SETFOCUS=0x380C57; -- N=Dlg->SetFocus([ID])
  MCODE_F_PLUGIN_CALL=0x380C58;
  MCODE_F_PLUGIN_EXIST=0x380C59; -- N=Plugin.Exist(SysId)
  MCODE_F_KEYBAR_SHOW=0x380C5A; -- N=keybar.show([Mode])
  MCODE_F_FAR_CFG_GET=0x380C5B;
  MCODE_F_FAR_CFG_SET=0x380C5C;
  MCODE_C_AREA_OTHER=0x380400; -- Режим копирования текста с экрана, вертикальные меню
  MCODE_C_AREA_SHELL=0x380401; -- Файловые панели
  MCODE_C_AREA_VIEWER=0x380402; -- Внутренняя программа просмотра
  MCODE_C_AREA_EDITOR=0x380403; -- Редактор
  MCODE_C_AREA_DIALOG=0x380404; -- Диалоги
  MCODE_C_AREA_SEARCH=0x380405; -- Быстрый поиск в панелях
  MCODE_C_AREA_DISKS=0x380406; -- Меню выбора дисков
  MCODE_C_AREA_MAINMENU=0x380407; -- Основное меню
  MCODE_C_AREA_MENU=0x380408; -- Прочие меню
  MCODE_C_AREA_HELP=0x380409; -- Система помощи
  MCODE_C_AREA_INFOPANEL=0x38040A; -- Информационная панель
  MCODE_C_AREA_QVIEWPANEL=0x38040B; -- Панель быстрого просмотра
  MCODE_C_AREA_TREEPANEL=0x38040C; -- Панель дерева папок
  MCODE_C_AREA_FINDFOLDER=0x38040D; -- Поиск папок
  MCODE_C_AREA_USERMENU=0x38040E; -- Меню пользователя
  MCODE_C_AREA_AUTOCOMPLETION=0x38040F; -- Список автодополнения
  MCODE_C_FULLSCREENMODE=0x380410; -- полноэкранный режим?
  MCODE_C_ISUSERADMIN=0x380411; -- Administrator status
  MCODE_C_BOF=0x380412; -- начало файла/активного каталога?
  MCODE_C_EOF=0x380413; -- конец файла/активного каталога?
  MCODE_C_EMPTY=0x380414; -- ком.строка пуста?
  MCODE_C_SELECTED=0x380415; -- выделенный блок есть?
  MCODE_C_ROOTFOLDER=0x380416; -- аналог MCODE_C_APANEL_ROOT для активной панели
  MCODE_C_APANEL_BOF=0x380417; -- начало активного  каталога?
  MCODE_C_PPANEL_BOF=0x380418; -- начало пассивного каталога?
  MCODE_C_APANEL_EOF=0x380419; -- конец активного  каталога?
  MCODE_C_PPANEL_EOF=0x38041A; -- конец пассивного каталога?
  MCODE_C_APANEL_ISEMPTY=0x38041B; -- активная панель:  пуста?
  MCODE_C_PPANEL_ISEMPTY=0x38041C; -- пассивная панель: пуста?
  MCODE_C_APANEL_SELECTED=0x38041D; -- активная панель:  выделенные элементы есть?
  MCODE_C_PPANEL_SELECTED=0x38041E; -- пассивная панель: выделенные элементы есть?
  MCODE_C_APANEL_ROOT=0x38041F; -- это корневой каталог активной панели?
  MCODE_C_PPANEL_ROOT=0x380420; -- это корневой каталог пассивной панели?
  MCODE_C_APANEL_VISIBLE=0x380421; -- активная панель:  видима?
  MCODE_C_PPANEL_VISIBLE=0x380422; -- пассивная панель: видима?
  MCODE_C_APANEL_PLUGIN=0x380423; -- активная панель:  плагиновая?
  MCODE_C_PPANEL_PLUGIN=0x380424; -- пассивная панель: плагиновая?
  MCODE_C_APANEL_FILEPANEL=0x380425; -- активная панель:  файловая?
  MCODE_C_PPANEL_FILEPANEL=0x380426; -- пассивная панель: файловая?
  MCODE_C_APANEL_FOLDER=0x380427; -- активная панель:  текущий элемент каталог?
  MCODE_C_PPANEL_FOLDER=0x380428; -- пассивная панель: текущий элемент каталог?
  MCODE_C_APANEL_LEFT=0x380429; -- активная панель левая?
  MCODE_C_PPANEL_LEFT=0x38042A; -- пассивная панель левая?
  MCODE_C_APANEL_LFN=0x38042B; -- на активной панели длинные имена?
  MCODE_C_PPANEL_LFN=0x38042C; -- на пассивной панели длинные имена?
  MCODE_C_APANEL_FILTER=0x38042D; -- на активной панели включен фильтр?
  MCODE_C_PPANEL_FILTER=0x38042E; -- на пассивной панели включен фильтр?
  MCODE_C_CMDLINE_BOF=0x38042F; -- курсор в начале cmd-строки редактирования?
  MCODE_C_CMDLINE_EOF=0x380430; -- курсор в конце cmd-строки редактирования?
  MCODE_C_CMDLINE_EMPTY=0x380431; -- ком.строка пуста?
  MCODE_C_CMDLINE_SELECTED=0x380432; -- в ком.строке есть выделение блока?
  MCODE_C_MSX=0x380433; -- "MsX"
  MCODE_C_MSY=0x380434; -- "MsY"
  MCODE_C_MSBUTTON=0x380435; -- "MsButton"
  MCODE_C_MSCTRLSTATE=0x380436; -- "MsCtrlState"
  MCODE_C_MSEVENTFLAGS=0x380437; -- "MsEventFlags"
  MCODE_C_MSLASTCTRLSTATE=0x380438; -- "MsLastCtrlState"
  MCODE_V_FAR_WIDTH=0x380800; -- Far.Width - ширина консольного окна
  MCODE_V_FAR_HEIGHT=0x380801; -- Far.Height - высота консольного окна
  MCODE_V_FAR_TITLE=0x380802; -- Far.Title - текущий заголовок консольного окна
  MCODE_V_FAR_UPTIME=0x380803; -- Far.UpTime - время работы Far в миллисекундах
  MCODE_V_FAR_PID=0x380804; -- Far.PID - содержит ИД текущей запущенной копии Far Manager
  MCODE_V_MACRO_AREA=0x380805; -- MacroArea - имя текущей макрос области
  MCODE_V_APANEL_CURRENT=0x380806; -- APanel.Current - имя файла на активной панели
  MCODE_V_PPANEL_CURRENT=0x380807; -- PPanel.Current - имя файла на пассивной панели
  MCODE_V_APANEL_SELCOUNT=0x380808; -- APanel.SelCount - активная панель:  число выделенных элементов
  MCODE_V_PPANEL_SELCOUNT=0x380809; -- PPanel.SelCount - пассивная панель: число выделенных элементов
  MCODE_V_APANEL_PATH=0x38080A; -- APanel.Path - активная панель:  путь на панели
  MCODE_V_PPANEL_PATH=0x38080B; -- PPanel.Path - пассивная панель: путь на панели
  MCODE_V_APANEL_PATH0=0x38080C; -- APanel.Path0 - активная панель:  путь на панели до вызова плагинов
  MCODE_V_PPANEL_PATH0=0x38080D; -- PPanel.Path0 - пассивная панель: путь на панели до вызова плагинов
  MCODE_V_APANEL_UNCPATH=0x38080E; -- APanel.UNCPath - активная панель:  UNC-путь на панели
  MCODE_V_PPANEL_UNCPATH=0x38080F; -- PPanel.UNCPath - пассивная панель: UNC-путь на панели
  MCODE_V_APANEL_WIDTH=0x380810; -- APanel.Width - активная панель:  ширина панели
  MCODE_V_PPANEL_WIDTH=0x380811; -- PPanel.Width - пассивная панель: ширина панели
  MCODE_V_APANEL_TYPE=0x380812; -- APanel.Type - тип активной панели
  MCODE_V_PPANEL_TYPE=0x380813; -- PPanel.Type - тип пассивной панели
  MCODE_V_APANEL_ITEMCOUNT=0x380814; -- APanel.ItemCount - активная панель:  число элементов
  MCODE_V_PPANEL_ITEMCOUNT=0x380815; -- PPanel.ItemCount - пассивная панель: число элементов
  MCODE_V_APANEL_CURPOS=0x380816; -- APanel.CurPos - активная панель:  текущий индекс
  MCODE_V_PPANEL_CURPOS=0x380817; -- PPanel.CurPos - пассивная панель: текущий индекс
  MCODE_V_APANEL_OPIFLAGS=0x380818; -- APanel.OPIFlags - активная панель: флаги открытого плагина
  MCODE_V_PPANEL_OPIFLAGS=0x380819; -- PPanel.OPIFlags - пассивная панель: флаги открытого плагина
  MCODE_V_APANEL_DRIVETYPE=0x38081A; -- APanel.DriveType - активная панель: тип привода
  MCODE_V_PPANEL_DRIVETYPE=0x38081B; -- PPanel.DriveType - пассивная панель: тип привода
  MCODE_V_APANEL_HEIGHT=0x38081C; -- APanel.Height - активная панель:  высота панели
  MCODE_V_PPANEL_HEIGHT=0x38081D; -- PPanel.Height - пассивная панель: высота панели
  MCODE_V_APANEL_COLUMNCOUNT=0x38081E; -- APanel.ColumnCount - активная панель:  количество колонок
  MCODE_V_PPANEL_COLUMNCOUNT=0x38081F; -- PPanel.ColumnCount - пассивная панель: количество колонок
  MCODE_V_APANEL_HOSTFILE=0x380820; -- APanel.HostFile - активная панель:  имя Host-файла
  MCODE_V_PPANEL_HOSTFILE=0x380821; -- PPanel.HostFile - пассивная панель: имя Host-файла
  MCODE_V_APANEL_PREFIX=0x380822; -- APanel.Prefix
  MCODE_V_PPANEL_PREFIX=0x380823; -- PPanel.Prefix
  MCODE_V_APANEL_FORMAT=0x380824; -- APanel.Format
  MCODE_V_PPANEL_FORMAT=0x380825; -- PPanel.Format
  MCODE_V_ITEMCOUNT=0x380826; -- ItemCount - число элементов в текущем объекте
  MCODE_V_CURPOS=0x380827; -- CurPos - текущий индекс в текущем объекте
  MCODE_V_TITLE=0x380828; -- Title - заголовок текущего объекта
  MCODE_V_HEIGHT=0x380829; -- Height - высота текущего объекта
  MCODE_V_WIDTH=0x38082A; -- Width - ширина текущего объекта
  MCODE_V_EDITORFILENAME=0x38082B; -- Editor.FileName - имя редактируемого файла
  MCODE_V_EDITORLINES=0x38082C; -- Editor.Lines - количество строк в редакторе
  MCODE_V_EDITORCURLINE=0x38082D; -- Editor.CurLine - текущая линия в редакторе (в дополнении к Count)
  MCODE_V_EDITORCURPOS=0x38082E; -- Editor.CurPos - текущая поз. в редакторе
  MCODE_V_EDITORREALPOS=0x38082F; -- Editor.RealPos - текущая поз. в редакторе без привязки к размеру табуляции
  MCODE_V_EDITORSTATE=0x380830; -- Editor.State
  MCODE_V_EDITORVALUE=0x380831; -- Editor.Value - содержимое текущей строки
  MCODE_V_EDITORSELVALUE=0x380832; -- Editor.SelValue - содержит содержимое выделенного блока
  MCODE_V_DLGITEMTYPE=0x380833; -- Dlg.ItemType
  MCODE_V_DLGITEMCOUNT=0x380834; -- Dlg.ItemCount
  MCODE_V_DLGCURPOS=0x380835; -- Dlg.CurPos
  MCODE_V_DLGINFOID=0x380836; -- Dlg.Info.Id
  MCODE_V_VIEWERFILENAME=0x380837; -- Viewer.FileName - имя просматриваемого файла
  MCODE_V_VIEWERSTATE=0x380838; -- Viewer.State
  MCODE_V_CMDLINE_ITEMCOUNT=0x380839; -- CmdLine.ItemCount
  MCODE_V_CMDLINE_CURPOS=0x38083A; -- CmdLine.CurPos
  MCODE_V_CMDLINE_VALUE=0x38083B; -- CmdLine.Value
  MCODE_V_DRVSHOWPOS=0x38083C; -- Drv.ShowPos - меню выбора дисков отображено: 1=слева (Alt-F1), 2=справа (Alt-F2), 0="нету его"
  MCODE_V_DRVSHOWMODE=0x38083D; -- Drv.ShowMode - режимы отображения меню выбора дисков
  MCODE_V_HELPFILENAME=0x38083E; -- Help.FileName
  MCODE_V_HELPTOPIC=0x38083F; -- Help.Topic
  MCODE_V_HELPSELTOPIC=0x380840; -- Help.SelTopic
  MCODE_V_MENU_VALUE=0x380841; -- Menu.Value
  MCODE_V_DLGINFOOWNER=0x380842; -- N=Dlg.Owner
  MCODE_V_DLGPREVPOS=0x380843; -- Dlg.PrevPos
  MCODE_V_MENUINFOID=0x380844; -- Menu.Id
  MCODE_UDLIST_SPLIT=0x380845; -- User defined list
}
