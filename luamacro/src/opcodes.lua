return {
  MCODE_F_NOFUNC=0x80C00;
  MCODE_F_ABS=0x80C01; -- N=abs(N)
  MCODE_F_AKEY=0x80C02; -- V=akey(Mode[,Type])
  MCODE_F_ASC=0x80C03; -- N=asc(S)
  MCODE_F_ATOI=0x80C04; -- N=atoi(S[,radix])
  MCODE_F_CLIP=0x80C05; -- V=clip(N[,V])
  MCODE_F_CHR=0x80C06; -- S=chr(N)
  MCODE_F_DATE=0x80C07; -- S=date([S])
  MCODE_F_DLG_GETVALUE=0x80C08; -- V=Dlg.GetValue(ID,N)
  MCODE_F_EDITOR_SEL=0x80C09; -- V=Editor.Sel(Action[,Opt])
  MCODE_F_EDITOR_SET=0x80C0A; -- N=Editor.Set(N,Var)
  MCODE_F_EDITOR_UNDO=0x80C0B; -- V=Editor.Undo(N)
  MCODE_F_EDITOR_POS=0x80C0C; -- N=Editor.Pos(Op,What[,Where])
  MCODE_F_EDITOR_DELLINE=0x80C0D; -- N=Editor.DelLine([Line])
  MCODE_F_EDITOR_INSSTR=0x80C0E; -- N=Editor.InsStr([S[,Line]])
  MCODE_F_ENVIRON=0x80C0F; -- S=env(S)
  MCODE_F_FATTR=0x80C10; -- N=fattr(S)
  MCODE_F_FEXIST=0x80C11; -- S=fexist(S)
  MCODE_F_FSPLIT=0x80C12; -- S=fsplit(S,N)
  MCODE_F_FMATCH=0x80C13; -- N=FMatch(S,Mask)
  MCODE_F_IIF=0x80C14; -- V=iif(C,V1,V2)
  MCODE_F_INDEX=0x80C15; -- S=index(S1,S2[,Mode])
  MCODE_F_INT=0x80C16; -- N=int(V)
  MCODE_F_ITOA=0x80C17; -- S=itoa(N[,radix])
  MCODE_F_KEY=0x80C18; -- S=key(V)
  MCODE_F_LCASE=0x80C19; -- S=lcase(S1)
  MCODE_F_LEN=0x80C1A; -- N=len(S)
  MCODE_F_MAX=0x80C1B; -- N=max(N1,N2)
  MCODE_F_MENU_CHECKHOTKEY=0x80C1C; -- N=checkhotkey(S[,N])
  MCODE_F_MENU_GETHOTKEY=0x80C1D; -- S=gethotkey([N])
  MCODE_F_MENU_SELECT=0x80C1E; -- N=Menu.Select(S[,N[,Dir]])
  MCODE_F_MIN=0x80C1F; -- N=min(N1,N2)
  MCODE_F_MOD=0x80C20; -- N=mod(a,b) == a %  b
  MCODE_F_MLOAD=0x80C21; -- B=mload(var)
  MCODE_F_MSAVE=0x80C22; -- B=msave(var)
  MCODE_F_MSGBOX=0x80C23; -- N=msgbox(["Title"[,"Text"[,flags]]])
  MCODE_F_PANEL_FATTR=0x80C24; -- N=Panel.FAttr(panelType,fileMask)
  MCODE_F_PANEL_SETPATH=0x80C25; -- N=panel.SetPath(panelType,pathName[,fileName])
  MCODE_F_PANEL_FEXIST=0x80C26; -- N=Panel.FExist(panelType,fileMask)
  MCODE_F_PANEL_SETPOS=0x80C27; -- N=Panel.SetPos(panelType,fileName)
  MCODE_F_PANEL_SETPOSIDX=0x80C28; -- N=Panel.SetPosIdx(panelType,Idx[,InSelection])
  MCODE_F_PANEL_SELECT=0x80C29; -- V=Panel.Select(panelType,Action[,Mode[,Items]])
  MCODE_F_PANELITEM=0x80C2A; -- V=Panel.Item(Panel,Index,TypeInfo)
  MCODE_F_EVAL=0x80C2B; -- N=eval(S[,N])
  MCODE_F_RINDEX=0x80C2C; -- S=rindex(S1,S2[,Mode])
  MCODE_F_SLEEP=0x80C2D; -- Sleep(N)
  MCODE_F_STRING=0x80C2E; -- S=string(V)
  MCODE_F_SUBSTR=0x80C2F; -- S=substr(S,start[,length])
  MCODE_F_UCASE=0x80C30; -- S=ucase(S1)
  MCODE_F_WAITKEY=0x80C31; -- V=waitkey([N,[T]])
  MCODE_F_XLAT=0x80C32; -- S=xlat(S)
  MCODE_F_FLOCK=0x80C33; -- N=FLock(N,N)
  MCODE_F_CALLPLUGIN=0x80C34; -- V=callplugin(SysID[,param])
  MCODE_F_REPLACE=0x80C35; -- S=replace(sS,sF,sR[,Count[,Mode]])
  MCODE_F_PROMPT=0x80C36; -- S=prompt("Title"[,"Prompt"[,flags[, "Src"[, "History"]]]])
  MCODE_F_BM_ADD=0x80C37; -- N=BM.Add()  - добавить текущие координаты и обрезать хвост
  MCODE_F_BM_CLEAR=0x80C38; -- N=BM.Clear() - очистить все закладки
  MCODE_F_BM_DEL=0x80C39; -- N=BM.Del([Idx]) - удаляет закладку с указанным индексом (x=1...), 0 - удаляет текущую закладку
  MCODE_F_BM_GET=0x80C3A; -- N=BM.Get(Idx,M) - возвращает координаты строки (M==0) или колонки (M==1) закладки с индексом (Idx=1...)
  MCODE_F_BM_GOTO=0x80C3B; -- N=BM.Goto([n]) - переход на закладку с указанным индексом (0 --> текущую)
  MCODE_F_BM_NEXT=0x80C3C; -- N=BM.Next() - перейти на следующую закладку
  MCODE_F_BM_POP=0x80C3D; -- N=BM.Pop() - восстановить текущую позицию из закладки в конце стека и удалить закладку
  MCODE_F_BM_PREV=0x80C3E; -- N=BM.Prev() - перейти на предыдущую закладку
  MCODE_F_BM_BACK=0x80C3F; -- N=BM.Back() - перейти на предыдущую закладку с возможным сохранением текущей позиции
  MCODE_F_BM_PUSH=0x80C40; -- N=BM.Push() - сохранить текущую позицию в виде закладки в конце стека
  MCODE_F_BM_STAT=0x80C41; -- N=BM.Stat([M]) - возвращает информацию о закладках, N=0 - текущее количество закладок	MCODE_F_TRIM,                     // S=trim(S[,N])
  MCODE_F_TRIM=0x80C42; -- S=trim(S[,N])
  MCODE_F_FLOAT=0x80C43; -- N=float(V)
  MCODE_F_TESTFOLDER=0x80C44; -- N=testfolder(S)
  MCODE_F_PRINT=0x80C45; -- N=Print(Str)
  MCODE_F_MMODE=0x80C46; -- N=MMode(Action[,Value])
  MCODE_F_EDITOR_SETTITLE=0x80C47; -- N=Editor.SetTitle([Title])
  MCODE_F_MENU_GETVALUE=0x80C48; -- S=Menu.GetValue([N])
  MCODE_F_MENU_ITEMSTATUS=0x80C49; -- N=Menu.ItemStatus([N])
  MCODE_F_BEEP=0x80C4A; -- N=beep([N])
  MCODE_F_KBDLAYOUT=0x80C4B; -- N=kbdLayout([N])
  MCODE_F_WINDOW_SCROLL=0x80C4C; -- N=Window.Scroll(Lines[,Axis])
  MCODE_F_CHECKALL=0x80C4D; -- B=CheckAll(Area,Flags[,Callback[,CallbackId]])
  MCODE_F_GETOPTIONS=0x80C4E; -- N=GetOptions()
  MCODE_F_USERMENU=0x80C4F; -- UserMenu([Param])
  MCODE_F_SETCUSTOMSORTMODE=0x80C50;
  MCODE_F_KEYMACRO=0x80C51;
  MCODE_F_FAR_GETCONFIG=0x80C52;
  MCODE_F_MACROSETTINGS=0x80C53;
  MCODE_F_SIZE2STR=0x80C54; -- S=Size2Str(Size,Flags[,Width])
  MCODE_F_STRWRAP=0x80C55; -- S=StrWrap(Text,Width[,Break[,Flags]])
  MCODE_F_DLG_SETFOCUS=0x80C56; -- N=Dlg->SetFocus([ID])
  MCODE_F_PLUGIN_CALL=0x80C57;
  MCODE_F_PLUGIN_EXIST=0x80C58; -- N=Plugin.Exist(SysId)
  MCODE_F_KEYBAR_SHOW=0x80C59; -- N=keybar.show([Mode])
  MCODE_F_FAR_CFG_GET=0x80C5A; -- V=Far.Cfg_Get(Key,Name)
  MCODE_C_AREA_OTHER=0x80400; -- Режим копирования текста с экрана, вертикальные меню
  MCODE_C_AREA_SHELL=0x80401; -- Файловые панели
  MCODE_C_AREA_VIEWER=0x80402; -- Внутренняя программа просмотра
  MCODE_C_AREA_EDITOR=0x80403; -- Редактор
  MCODE_C_AREA_DIALOG=0x80404; -- Диалоги
  MCODE_C_AREA_SEARCH=0x80405; -- Быстрый поиск в панелях
  MCODE_C_AREA_DISKS=0x80406; -- Меню выбора дисков
  MCODE_C_AREA_MAINMENU=0x80407; -- Основное меню
  MCODE_C_AREA_MENU=0x80408; -- Прочие меню
  MCODE_C_AREA_HELP=0x80409; -- Система помощи
  MCODE_C_AREA_INFOPANEL=0x8040A; -- Информационная панель
  MCODE_C_AREA_QVIEWPANEL=0x8040B; -- Панель быстрого просмотра
  MCODE_C_AREA_TREEPANEL=0x8040C; -- Панель дерева папок
  MCODE_C_AREA_FINDFOLDER=0x8040D; -- Поиск папок
  MCODE_C_AREA_USERMENU=0x8040E; -- Меню пользователя
  MCODE_C_AREA_AUTOCOMPLETION=0x8040F; -- Список автодополнения
  MCODE_C_FULLSCREENMODE=0x80410; -- полноэкранный режим?
  MCODE_C_ISUSERADMIN=0x80411; -- Administrator status
  MCODE_C_BOF=0x80412; -- начало файла/активного каталога?
  MCODE_C_EOF=0x80413; -- конец файла/активного каталога?
  MCODE_C_EMPTY=0x80414; -- ком.строка пуста?
  MCODE_C_SELECTED=0x80415; -- выделенный блок есть?
  MCODE_C_ROOTFOLDER=0x80416; -- аналог MCODE_C_APANEL_ROOT для активной панели
  MCODE_C_APANEL_BOF=0x80417; -- начало активного  каталога?
  MCODE_C_PPANEL_BOF=0x80418; -- начало пассивного каталога?
  MCODE_C_APANEL_EOF=0x80419; -- конец активного  каталога?
  MCODE_C_PPANEL_EOF=0x8041A; -- конец пассивного каталога?
  MCODE_C_APANEL_ISEMPTY=0x8041B; -- активная панель:  пуста?
  MCODE_C_PPANEL_ISEMPTY=0x8041C; -- пассивная панель: пуста?
  MCODE_C_APANEL_SELECTED=0x8041D; -- активная панель:  выделенные элементы есть?
  MCODE_C_PPANEL_SELECTED=0x8041E; -- пассивная панель: выделенные элементы есть?
  MCODE_C_APANEL_ROOT=0x8041F; -- это корневой каталог активной панели?
  MCODE_C_PPANEL_ROOT=0x80420; -- это корневой каталог пассивной панели?
  MCODE_C_APANEL_VISIBLE=0x80421; -- активная панель:  видима?
  MCODE_C_PPANEL_VISIBLE=0x80422; -- пассивная панель: видима?
  MCODE_C_APANEL_PLUGIN=0x80423; -- активная панель:  плагиновая?
  MCODE_C_PPANEL_PLUGIN=0x80424; -- пассивная панель: плагиновая?
  MCODE_C_APANEL_FILEPANEL=0x80425; -- активная панель:  файловая?
  MCODE_C_PPANEL_FILEPANEL=0x80426; -- пассивная панель: файловая?
  MCODE_C_APANEL_FOLDER=0x80427; -- активная панель:  текущий элемент каталог?
  MCODE_C_PPANEL_FOLDER=0x80428; -- пассивная панель: текущий элемент каталог?
  MCODE_C_APANEL_LEFT=0x80429; -- активная панель левая?
  MCODE_C_PPANEL_LEFT=0x8042A; -- пассивная панель левая?
  MCODE_C_APANEL_LFN=0x8042B; -- на активной панели длинные имена?
  MCODE_C_PPANEL_LFN=0x8042C; -- на пассивной панели длинные имена?
  MCODE_C_APANEL_FILTER=0x8042D; -- на активной панели включен фильтр?
  MCODE_C_PPANEL_FILTER=0x8042E; -- на пассивной панели включен фильтр?
  MCODE_C_CMDLINE_BOF=0x8042F; -- курсор в начале cmd-строки редактирования?
  MCODE_C_CMDLINE_EOF=0x80430; -- курсор в конце cmd-строки редактирования?
  MCODE_C_CMDLINE_EMPTY=0x80431; -- ком.строка пуста?
  MCODE_C_CMDLINE_SELECTED=0x80432; -- в ком.строке есть выделение блока?
  MCODE_C_MSX=0x80433; -- "MsX"
  MCODE_C_MSY=0x80434; -- "MsY"
  MCODE_C_MSBUTTON=0x80435; -- "MsButton"
  MCODE_C_MSCTRLSTATE=0x80436; -- "MsCtrlState"
  MCODE_C_MSEVENTFLAGS=0x80437; -- "MsEventFlags"
  MCODE_C_MSLASTCTRLSTATE=0x80438; -- "MsLastCtrlState"
  MCODE_V_FAR_WIDTH=0x80800; -- Far.Width - ширина консольного окна
  MCODE_V_FAR_HEIGHT=0x80801; -- Far.Height - высота консольного окна
  MCODE_V_FAR_TITLE=0x80802; -- Far.Title - текущий заголовок консольного окна
  MCODE_V_FAR_UPTIME=0x80803; -- Far.UpTime - время работы Far в миллисекундах
  MCODE_V_FAR_PID=0x80804; -- Far.PID - содержит ИД текущей запущенной копии Far Manager
  MCODE_V_MACRO_AREA=0x80805; -- MacroArea - имя текущей макрос области
  MCODE_V_APANEL_CURRENT=0x80806; -- APanel.Current - имя файла на активной панели
  MCODE_V_PPANEL_CURRENT=0x80807; -- PPanel.Current - имя файла на пассивной панели
  MCODE_V_APANEL_SELCOUNT=0x80808; -- APanel.SelCount - активная панель:  число выделенных элементов
  MCODE_V_PPANEL_SELCOUNT=0x80809; -- PPanel.SelCount - пассивная панель: число выделенных элементов
  MCODE_V_APANEL_PATH=0x8080A; -- APanel.Path - активная панель:  путь на панели
  MCODE_V_PPANEL_PATH=0x8080B; -- PPanel.Path - пассивная панель: путь на панели
  MCODE_V_APANEL_PATH0=0x8080C; -- APanel.Path0 - активная панель:  путь на панели до вызова плагинов
  MCODE_V_PPANEL_PATH0=0x8080D; -- PPanel.Path0 - пассивная панель: путь на панели до вызова плагинов
  MCODE_V_APANEL_UNCPATH=0x8080E; -- APanel.UNCPath - активная панель:  UNC-путь на панели
  MCODE_V_PPANEL_UNCPATH=0x8080F; -- PPanel.UNCPath - пассивная панель: UNC-путь на панели
  MCODE_V_APANEL_WIDTH=0x80810; -- APanel.Width - активная панель:  ширина панели
  MCODE_V_PPANEL_WIDTH=0x80811; -- PPanel.Width - пассивная панель: ширина панели
  MCODE_V_APANEL_TYPE=0x80812; -- APanel.Type - тип активной панели
  MCODE_V_PPANEL_TYPE=0x80813; -- PPanel.Type - тип пассивной панели
  MCODE_V_APANEL_ITEMCOUNT=0x80814; -- APanel.ItemCount - активная панель:  число элементов
  MCODE_V_PPANEL_ITEMCOUNT=0x80815; -- PPanel.ItemCount - пассивная панель: число элементов
  MCODE_V_APANEL_CURPOS=0x80816; -- APanel.CurPos - активная панель:  текущий индекс
  MCODE_V_PPANEL_CURPOS=0x80817; -- PPanel.CurPos - пассивная панель: текущий индекс
  MCODE_V_APANEL_OPIFLAGS=0x80818; -- APanel.OPIFlags - активная панель: флаги открытого плагина
  MCODE_V_PPANEL_OPIFLAGS=0x80819; -- PPanel.OPIFlags - пассивная панель: флаги открытого плагина
  MCODE_V_APANEL_DRIVETYPE=0x8081A; -- APanel.DriveType - активная панель: тип привода
  MCODE_V_PPANEL_DRIVETYPE=0x8081B; -- PPanel.DriveType - пассивная панель: тип привода
  MCODE_V_APANEL_HEIGHT=0x8081C; -- APanel.Height - активная панель:  высота панели
  MCODE_V_PPANEL_HEIGHT=0x8081D; -- PPanel.Height - пассивная панель: высота панели
  MCODE_V_APANEL_COLUMNCOUNT=0x8081E; -- APanel.ColumnCount - активная панель:  количество колонок
  MCODE_V_PPANEL_COLUMNCOUNT=0x8081F; -- PPanel.ColumnCount - пассивная панель: количество колонок
  MCODE_V_APANEL_HOSTFILE=0x80820; -- APanel.HostFile - активная панель:  имя Host-файла
  MCODE_V_PPANEL_HOSTFILE=0x80821; -- PPanel.HostFile - пассивная панель: имя Host-файла
  MCODE_V_APANEL_PREFIX=0x80822; -- APanel.Prefix
  MCODE_V_PPANEL_PREFIX=0x80823; -- PPanel.Prefix
  MCODE_V_APANEL_FORMAT=0x80824; -- APanel.Format
  MCODE_V_PPANEL_FORMAT=0x80825; -- PPanel.Format
  MCODE_V_ITEMCOUNT=0x80826; -- ItemCount - число элементов в текущем объекте
  MCODE_V_CURPOS=0x80827; -- CurPos - текущий индекс в текущем объекте
  MCODE_V_TITLE=0x80828; -- Title - заголовок текущего объекта
  MCODE_V_HEIGHT=0x80829; -- Height - высота текущего объекта
  MCODE_V_WIDTH=0x8082A; -- Width - ширина текущего объекта
  MCODE_V_EDITORFILENAME=0x8082B; -- Editor.FileName - имя редактируемого файла
  MCODE_V_EDITORLINES=0x8082C; -- Editor.Lines - количество строк в редакторе
  MCODE_V_EDITORCURLINE=0x8082D; -- Editor.CurLine - текущая линия в редакторе (в дополнении к Count)
  MCODE_V_EDITORCURPOS=0x8082E; -- Editor.CurPos - текущая поз. в редакторе
  MCODE_V_EDITORREALPOS=0x8082F; -- Editor.RealPos - текущая поз. в редакторе без привязки к размеру табуляции
  MCODE_V_EDITORSTATE=0x80830; -- Editor.State
  MCODE_V_EDITORVALUE=0x80831; -- Editor.Value - содержимое текущей строки
  MCODE_V_EDITORSELVALUE=0x80832; -- Editor.SelValue - содержит содержимое выделенного блока
  MCODE_V_DLGITEMTYPE=0x80833; -- Dlg.ItemType
  MCODE_V_DLGITEMCOUNT=0x80834; -- Dlg.ItemCount
  MCODE_V_DLGCURPOS=0x80835; -- Dlg.CurPos
  MCODE_V_DLGINFOID=0x80836; -- Dlg.Info.Id
  MCODE_V_VIEWERFILENAME=0x80837; -- Viewer.FileName - имя просматриваемого файла
  MCODE_V_VIEWERSTATE=0x80838; -- Viewer.State
  MCODE_V_CMDLINE_ITEMCOUNT=0x80839; -- CmdLine.ItemCount
  MCODE_V_CMDLINE_CURPOS=0x8083A; -- CmdLine.CurPos
  MCODE_V_CMDLINE_VALUE=0x8083B; -- CmdLine.Value
  MCODE_V_DRVSHOWPOS=0x8083C; -- Drv.ShowPos - меню выбора дисков отображено: 1=слева (Alt-F1), 2=справа (Alt-F2), 0="нету его"
  MCODE_V_DRVSHOWMODE=0x8083D; -- Drv.ShowMode - режимы отображения меню выбора дисков
  MCODE_V_HELPFILENAME=0x8083E; -- Help.FileName
  MCODE_V_HELPTOPIC=0x8083F; -- Help.Topic
  MCODE_V_HELPSELTOPIC=0x80840; -- Help.SelTopic
  MCODE_V_MENU_VALUE=0x80841; -- Menu.Value
  MCODE_V_DLGINFOOWNER=0x80842; -- N=Dlg.Owner
  MCODE_V_DLGPREVPOS=0x80843; -- Dlg.PrevPos
  MCODE_V_MENUINFOID=0x80844; -- Menu.Id
}
