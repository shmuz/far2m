/*
main.cpp

Функция main.
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"
#include <signal.h>

#include "chgprior.hpp"
#include "clipboard.hpp"
#include "cmdline.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "ConfigOpt.hpp"
#include "console.hpp"
#include "ctrlobj.hpp"
#include "dirmix.hpp"
#include "execute.hpp"
#include "fileedit.hpp"
#include "filepanels.hpp"
#include "fileview.hpp"
#include "interf.hpp"
#include "InterThreadCall.hpp"
#include "keyboard.hpp"
#include "manager.hpp"
#include "pathmix.hpp"
#include "SafeMMap.hpp"
#include "scrbuf.hpp"
#include "syslog.hpp"
#include "udlist.hpp"
#include "vtshell.h"

struct CommandLineParams {
	int StartLine = -1;
	int StartChar = -1;
	int CntDestName = 0;
	FARString DestNames[2];

	void AddDestName(const FARString &Name, bool cdCommand)
	{
		if (CntDestName < 2 && !Name.IsEmpty()) {
			if (!cdCommand && IsPluginPrefixPath(Name)) {
				DestNames[CntDestName++] = Name;
			}
			else {
				FARString tmpStr;
				ConvertNameToFull(Name, tmpStr);
				if (apiGetFileAttributes(tmpStr) != INVALID_FILE_ATTRIBUTES)
					DestNames[CntDestName++] = tmpStr;
			}
		}
	}
};

static unsigned int gMainThreadID;

static void print_help(const char *self)
{
	printf("FAR2M - dual-panel file manager with built-in terminal\n"
		"Usage: %s [switches] [[-cd] apath [[-cd] ppath]]\n\n"

		"where\n"
		"  apath - path to folder or file, or plugin command with prefix\n"
		"          for the active panel\n"
		"  ppath - path to folder or file, or plugin command with prefix\n"
		"          for the passive panel\n\n"

		"The following switches may be used in the command line:\n\n"

		"  -h                         This help\n"
		"  -a                         Disable display of characters with codes\n"
		"                             0 - 31 and 255\n"
		"  -ag                        Disable display of pseudographics with codes > 127\n"
		"  -an                        Disable display of pseudographics characters\n"
		"                             completely\n"
		"  -co                        Load plugins from the cache only\n"
		"  -cd <path>                 Change panel's directory to specified path\n"
		"  -m                         Do not load macros\n"
		"  -ma                        Do not execute auto run macros\n"
		"  -p[<path>]                 Search for \"common\" plugins in the directory\n"
		"                             specified by <path>; several search paths can\n"
		"                             be specified separated by ‘:’\n"
		"  -u <identity> OR\n"
		"  -u </path/name>            Specify separate settings via identity\n"
		"                             or FS location\n"
		"  -v <filename>              View the specified file\n"
		"  -v - <command line>        Execute given command line and open viewer\n"
		"                             with its output\n"
		"  -e[<line>[:<pos>]] [filename]\n"
		"                             Edit the specified file with optional cursor\n"
		"                             position specification or empty new file\n"
		"  -e[<line>[:<pos>]] - <command line>\n"
		"                             Execute given command line and open editor\n"
		"                             with its output\n"
		"  -set:<parameter>=<value>   Override the configuration parameter\n"
		"                             Example: far2m -set:Language.Main=English\n"
		"                                            -set:Screen.Clock=false\n"
		"                                            -set:XLat.Flags=0xff\n"
		"\n",
		self);
	WinPortHelp();
}

static FARString ReconstructCommandLine(int argc, char **argv)
{
	FARString cmd;
	for (;argc; --argc, ++argv) {
		if (*argv) {
			if (!cmd.IsEmpty()) {
				cmd+= L" ";
			}
			std::string arg = *argv;
			QuoteCmdArg(arg);
			cmd+= arg;
		}
	}
	return cmd;
}

static void UpdatePathOptions(const FARString &strDestName, bool IsLeftPanel)
{
	if (strDestName.IsEmpty() || IsPluginPrefixPath(strDestName))
		return;

	FARString *outFolder, *outCurFile;

	if (IsLeftPanel) {
		Opt.LeftPanel.Type = FILE_PANEL;  // сменим моду панели
		Opt.LeftPanel.Visible = true;     // и включим ее
		outFolder = &Opt.strLeftFolder;
		outCurFile = &Opt.strLeftCurFile;
	}
	else {
		Opt.RightPanel.Type = FILE_PANEL;
		Opt.RightPanel.Visible = true;
		outFolder = &Opt.strRightFolder;
		outCurFile = &Opt.strRightCurFile;
	}

	auto Attr = apiGetFileAttributes(strDestName);
	if (Attr != INVALID_FILE_ATTRIBUTES) {
		if (Attr & FILE_ATTRIBUTE_DIRECTORY) {
			outCurFile->Clear();
			*outFolder = strDestName;
		}
		else {
			*outCurFile = PointToName(strDestName);
			*outFolder = strDestName;
			CutToSlash(*outFolder, true);
			if (outFolder->IsEmpty())
				*outFolder = L"/";
		}
	}
}

// See: github.com/elfmz/far2l/issues/2758
//  and far/bootstrap/far2m-cd.sh
static void Write_FAR2M_CWD()
{
	const char *far_cwd = getenv("FAR2M_CWD");
	if (far_cwd && *far_cwd) {
		int fd = open(far_cwd, O_WRONLY | O_CREAT | O_TRUNC, 0640);
		if (fd != -1) {
			FARString cur_dir;
			if (apiGetCurrentDirectory(cur_dir)) {
				const auto &cwd = cur_dir.GetMB();
				if (write(fd, cwd.c_str(), cwd.size()) == -1) {
					perror("write cwd");
				}
			}
			close(fd);
		}
	}
}

static void RunEditorOrViewerMode(const CommandLineParams &Params)
{
	clock_t cl_start = clock();
	Panel *DummyPanel = new Panel;
	CtrlObject->CreateFilePanels();
	CtrlObject->Cp()->LeftPanel = CtrlObject->Cp()->RightPanel = CtrlObject->Cp()->ActivePanel = DummyPanel;
	CtrlObject->Plugins.LoadPlugins();
	CtrlObject->Macro.LoadMacros(true);

	bool IsCmdOut = Opt.OnlyEditorViewerUsed == Options::ONLY_EDITOR_ON_CMDOUT
			|| Opt.OnlyEditorViewerUsed == Options::ONLY_VIEWER_ON_CMDOUT;

	bool IsEditor = Opt.OnlyEditorViewerUsed == Options::ONLY_EDITOR
			|| Opt.OnlyEditorViewerUsed == Options::ONLY_EDITOR_ON_CMDOUT;

	if (IsCmdOut)
		Opt.strEditViewArg = ExecuteCommandAndGrabItsOutput(Opt.strEditViewArg);

	if (IsEditor)
	{
		auto ShellEditor = new FileEditor(Opt.strEditViewArg, CP_AUTODETECT,
				FFILEEDIT_CANNEWFILE | FFILEEDIT_ENABLEF6, Params.StartLine, Params.StartChar);

		if (!ShellEditor->GetExitCode())
			FrameManager->ExitMainLoop(false);
	}
	else
	{
		FileViewerParams Params { Opt.strEditViewArg };
		auto ShellViewer = new FileViewer(Params);

		if (!ShellViewer->GetExitCode())
			FrameManager->ExitMainLoop(false);
	}

	fprintf(stderr, "STARTUP(E/V): %llu\n", (unsigned long long)(clock() - cl_start) );
	FrameManager->EnterMainLoop();

	if (IsCmdOut)
		unlink(Opt.strEditViewArg.GetMB().c_str());

	CtrlObject->Cp()->LeftPanel = CtrlObject->Cp()->RightPanel = CtrlObject->Cp()->ActivePanel = nullptr;
	delete DummyPanel;
}

static void RunPanelMode(const CommandLineParams &Params)
{
	clock_t cl_start = clock();

	// воспользуемся тем, что ControlObject::Init() создает панели, юзая Opt.*
	UpdatePathOptions(Params.DestNames[0], Opt.LeftPanel.Focus);
	UpdatePathOptions(Params.DestNames[1], !Opt.LeftPanel.Focus);

	// теперь все готово - создаем панели!
	CtrlObject->Init();

	// а теперь "провалимся" в каталог или хост-файл (если получится ;-)
	if (Params.CntDestName > 0)  // активная панель
	{
		// Always update pointers as prefixed plugin calls could recreate one or both panels
		auto ActivePanel = [&]() { return CtrlObject->Cp()->ActivePanel; };
		auto AnotherPanel = [&]() { return CtrlObject->Cp()->GetAnotherPanel(ActivePanel()); };

		FARString strCurDir;

		if (Params.CntDestName > 1)  // пассивная панель
		{
			AnotherPanel()->GetCurDir(strCurDir);
			FarChDir(strCurDir);

			if (IsPluginPrefixPath(Params.DestNames[1]))
			{
				AnotherPanel()->SetFocus();
				CtrlObject->CmdLine->ExecString(Params.DestNames[1], false);
				AnotherPanel()->SetFocus();
			}
		}

		ActivePanel()->GetCurDir(strCurDir);
		FarChDir(strCurDir);

		if (IsPluginPrefixPath(Params.DestNames[0]))
		{
			CtrlObject->CmdLine->ExecString(Params.DestNames[0], false);
		}

		// Сначала редравим пассивную панель, а потом активную!
		AnotherPanel()->Redraw();
		ActivePanel()->Redraw();
	}

	fprintf(stderr, "STARTUP: %llu\n", (unsigned long long)(clock() - cl_start));
	FrameManager->EnterMainLoop();
	Write_FAR2M_CWD();
}

static int MainProcess(const CommandLineParams &Params)
{
	SCOPED_ACTION(InterThreadCallsDispatcherThread);
	{
		SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
		ControlObject CtrlObj;
		uint64_t InitAttributes = 0;
		Console.GetTextAttributes(InitAttributes);
		SetFarColor(COL_COMMANDLINEUSERSCREEN, true);

		if (Opt.OnlyEditorViewerUsed == Options::INCLUDING_PANELS)
			RunPanelMode(Params);
		else
			RunEditorOrViewerMode(Params);

		// очистим за собой!
		SetScreen(0, 0, ScrX, ScrY, L' ', FarColorToReal(COL_COMMANDLINEUSERSCREEN));
		Console.SetTextAttributes(InitAttributes);
		ScrBuf.ResetShadow();
		ScrBuf.Flush();
		MoveRealCursor(0,0);
	}
	CloseConsole();
	return FarExitCode;
}

static void SetupFarPath(const char *Arg0)
{
	InMyTemp(); // pre-cache in env temp pathes
	InitCurrentDirectory();

	char buf[PATH_MAX + 1];
	ssize_t buf_sz = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (buf_sz > 0 && buf[0] == GOOD_SLASH) {
		buf[buf_sz] = 0;
		g_strFarModuleName = buf;

	} else {
		g_strFarModuleName = LookupExecutable(Arg0);
	}

	FARString dir = g_strFarModuleName; // e.g. /usr/local/bin/far2m
	CutToSlash(dir, true);
	const wchar_t *last_element = PointToName(dir);
	if (last_element && wcscmp(last_element, L"bin") == 0) {
		CutToSlash(dir);
		SetPathTranslationPrefix(dir);
	}
}

static void ProcessCommandLine(CommandLineParams &Params, int argc, char **argv)
{
	bool bCustomPlugins = false;

	for (int I=1; I<argc; I++)
	{
		if (strncmp(argv[I], "--", 2) == 0)
			continue; // 2024-Jul-06: --primary-selection, --maximize and --nomaximize may appear here

		bool switchHandled = false;
		bool cdCommand = false;
		FARString arg_w = argv[I];
		size_t argLen = arg_w.GetLength();
		if (argLen > 1 && arg_w[0] == L'-')
		{
			switchHandled = true;
			FARString argUpper = arg_w.Upper();

			if (argUpper == L"-A")
				Opt.CleanAscii = true;

			else if (argUpper == L"-AG")
				Opt.NoGraphics = true;

			else if (argUpper == L"-AN")
				Opt.NoBoxes = true;

			else if (argUpper == L"-CD") {
				if (++I < argc) {
					arg_w = argv[I];
					switchHandled = false;
					cdCommand = true;
				}
			}

			else if (argUpper == L"-CO")
				Opt.LoadPlug.PluginsCacheOnly = true;

			else if (argUpper == L"-I")
				Opt.SmallIcon = true;

			else if (argUpper == L"-M")
				Opt.Macro.DisableMacro |= MDOL_ALL;

			else if (argUpper == L"-MA")
				Opt.Macro.DisableMacro |= MDOL_AUTOSTART;

			else if (argUpper == L"-W")
				Opt.WindowMode = true;

			else if (argUpper.Begins(L"-SET:"))
				Opt.CmdLineStrings.emplace_back(arg_w.CPtr() + 5);

			else if (argUpper.Begins(L"-E"))
			{
				if (Opt.OnlyEditorViewerUsed != Options::INCLUDING_PANELS) { //skip as already handled
					if (++I == argc || strcmp(argv[I], "-") == 0) break;
					continue;
				}

				if (argLen > 2) {
					unsigned int stLine, stChar;
					int N = sscanf(argv[I] + 2, "%u:%u", &stLine, &stChar);
					if (N > 0) {
						Params.StartLine = stLine;
						if (N > 1) Params.StartChar = stChar;
					}
					else continue;
				}

				if (I+1 < argc) {
					if (strcmp(argv[I+1], "-") == 0) {
						Opt.OnlyEditorViewerUsed = Options::ONLY_EDITOR_ON_CMDOUT;
						Opt.strEditViewArg = ReconstructCommandLine(argc - I - 2, &argv[I+2]);
						break;
					}
					Opt.OnlyEditorViewerUsed = Options::ONLY_EDITOR;
					Opt.strEditViewArg = argv[++I];
				}
				else { // -e without filename => new file to editor
					Opt.OnlyEditorViewerUsed = Options::ONLY_EDITOR;
					Opt.strEditViewArg.Clear();
				}
			}

			else if (argUpper == L"-V")
			{
				if (Opt.OnlyEditorViewerUsed != Options::INCLUDING_PANELS) { //skip as already handled
					if (++I == argc || strcmp(argv[I], "-") == 0) break;
					continue;
				}

				if (I+1 < argc) {
					if (strcmp(argv[I+1], "-") == 0) {
						Opt.OnlyEditorViewerUsed = Options::ONLY_VIEWER_ON_CMDOUT;
						Opt.strEditViewArg = ReconstructCommandLine(argc - I - 2, &argv[I+2]);
						break;
					}
					Opt.OnlyEditorViewerUsed = Options::ONLY_VIEWER;
					Opt.strEditViewArg = argv[++I];
				}
			}

			else if (argUpper.Begins(L"-P"))
			{
				bCustomPlugins = true;
				if (argLen > 2)
				{
					UserDefinedList Udl(ULF_UNIQUE | ULF_CASESENSITIVE, L":");
					if (Udl.Set(arg_w.CPtr() + 2))
					{
						for (size_t i=0; i < Udl.Size(); i++)
						{
							FARString path = Udl.Get(i);
							apiExpandEnvironmentStrings(path, path);
							// Unquote(path);
							ConvertNameToFull(path, path);
							if (!Opt.LoadPlug.strCustomPluginsPath.IsEmpty()) {
								Opt.LoadPlug.strCustomPluginsPath += L':';
							}
							Opt.LoadPlug.strCustomPluginsPath += path;
						}
					}
				}
			}
		}

		if (!switchHandled) { // простые параметры. Их может быть max две штукА.
			Params.AddDestName(arg_w, cdCommand);
		}
	}

	if (bCustomPlugins) { //если есть ключ /p то он отменяет /co
		Opt.LoadPlug.MainPluginDir = false;
		Opt.LoadPlug.PluginsPersonal = false;
		Opt.LoadPlug.PluginsCacheOnly = false;
	}
	else {
		Opt.LoadPlug.MainPluginDir = !Opt.LoadPlug.PluginsCacheOnly;
		Opt.LoadPlug.PluginsPersonal = !Opt.LoadPlug.PluginsCacheOnly;
	}
}

int FarAppMain(int argc, char **argv)
{
	// avoid killing process due to broker terminated unexpectedly
	signal(SIGPIPE, SIG_IGN);

	fprintf(stderr, "argv[0]='%s' g_strFarModuleName='%ls' translation_prefix='%ls' temp='%s' config='%s'\n",
		argv[0], g_strFarModuleName.CPtr(), GetPathTranslationPrefix(), InMyTemp().c_str(), InMyConfig().c_str());

	// make current thread to be same as main one to avoid FARString reference-counter
	// from cloning main strings from current one
	OverrideInterThreadID(gMainThreadID);

	CharClasses::InitCharFlags();

	Opt.IsUserAdmin = (geteuid() == 0);
	if (Opt.IsUserAdmin)
		setenv("FARADMINMODE", "1", 1);
	else
		unsetenv("FARADMINMODE");

	setenv("FARPID", ToDec(getpid()).c_str(), 1);

	g_strFarPath = g_strFarModuleName;        // /usr/bin/far2m
	bool translated = TranslateFarString<TranslateInstallPath_Bin2Share>(g_strFarPath); // /usr/share/far2m
	CutToSlash(g_strFarPath, true);           // /usr/share
	if (translated) {
		g_strFarPath.Append("/" APP_BASENAME);  // /usr/share/far2m
	}
	setenv("FARHOME", g_strFarPath.GetMB().c_str(), 1);
	AddEndSlash(g_strFarPath);

	ConfigOptLoad();

	CommandLineParams Params;
	ProcessCommandLine(Params, argc, argv); // call it *after* ConfigOptLoad()

	//Инициализация массива клавиш. Должна быть после CopyGlobalSettings!
	InitKeysArray();
	//WaitForInputIdle(GetCurrentProcess(),0);
	std::set_new_handler(nullptr);

	FarColors::InitFarColors();
	InitConsole();
	WINPORT(SetConsoleCursorBlinkTime)(nullptr, Opt.CursorBlinkTime);

	if (!Lang.Init(g_strFarPath,true,Msg::MaxMsgId))
	{
		LPCWSTR LngMsg;
		switch (Lang.LastError())
		{
		case LERROR_BAD_FILE:
			LngMsg = L"\nError: language data is incorrect or damaged. Press any key to exit...";
			break;
		case LERROR_FILE_NOT_FOUND:
			LngMsg = L"\nError: cannot find language data. Press any key to exit...";
			break;
		default:
			LngMsg = L"\nError: cannot load language data. Press any key to exit...";
		}
		ControlObject::ShowStartupBanner(LngMsg);
		WaitKey(); // А стоит ли ожидать клавишу??? Стоит
		return 1;
	}
	setenv("FARLANG", Opt.strLanguage.GetMB().c_str(), 1);

	// (!!!) temporary STUB because now Editor can not input filename "", see: fileedit.cpp -> FileEditor::Init()
	// default Editor file name for new empty file
	if ( Opt.OnlyEditorViewerUsed == Options::ONLY_EDITOR && Opt.strEditViewArg.IsEmpty() )
		Opt.strEditViewArg = Msg::NewFileName;

	int Result = MainProcess(Params);

	EmptyInternalClipboard();
	VTShell_Shutdown();//ensure VTShell deinitialized before statics destructors called
	_OT(SysLog(L"[[[[[Exit of FAR]]]]]]]]]"));
	return Result;
}


/*void EncodingTest()
{
	std::wstring v = MB2Wide("\x80hello\x80""aaaaaaaaaaaa\x80""zzzzzzzzzzz\x80");
	printf("%u: '%ls'\n", (unsigned int)v.size(), v.c_str());
	for (size_t i = 0; i<v.size(); ++i)
		printf("%02x ", (unsigned int)v[i]);

	printf("\n");
	std::string a = StrWide2MB(v);
	for (size_t i = 0; i<a.size(); ++i)
		printf("%02x ", (unsigned char)a[i]);
	printf("\n");
}

void SudoTest()
{
	SudoClientRegion sdc_rgn;
	int fd = sdc_open("/root/foo", O_CREAT | O_RDWR, 0666);
	if (fd!=-1) {
		sdc_write(fd, "bar", 3);
		sdc_close(fd);
	} else
		perror("sdc_open");
	exit(0);
}
*/

static int libexec(const char *lib, const char *cd, const char *symbol, int argc, char *argv[])
{
	void *dl = dlopen(lib, RTLD_LOCAL|RTLD_LAZY);
	if (!dl) {
		const char* msg = dlerror();
		if (!msg) msg = "dlopen error";
		fprintf(stderr, "libexec('%s', '%s', %d) - %s\n", lib, symbol, argc, msg);
		return -1;
	}

	dlerror(); // clear stale state
	typedef int (*main_t)(int argc, char *argv[]);
	auto libexec_main = reinterpret_cast<main_t>(dlsym(dl, symbol));
	if (const char* errmsg = dlerror()) {
		fprintf(stderr, "libexec('%s', '%s', %d) - %s\n", lib, symbol, argc, errmsg);
		dlclose(dl);
		return -1;
	}

	if (cd && *cd && chdir(cd) == -1) {
		fprintf(stderr, "libexec('%s', '%s', %d) - chdir('%s') error %d\n", lib, symbol, argc, cd, errno);
	}

	int ret = libexec_main(argc, argv);
	dlclose(dl);
	return ret;
}

static void SetCustomSettings(const char *arg)
{
	std::string refined;
	if (arg[0] == '.' && arg[1] == GOOD_SLASH) {
		char cwdbuf[MAX_PATH + 1];
		const char *cwd = getcwd(cwdbuf, MAX_PATH);
		if (cwd) {
			refined = cwd;
			refined+= &arg[1];
		}
	}
	else {
		refined = arg;
	}

	while (!refined.empty() && refined.back() == GOOD_SLASH) {
		refined.pop_back();
	}

	fprintf(stderr, "%s: '%s'\n", __FUNCTION__, refined.c_str());

	if (!refined.empty()) {
		setenv(ENV_FARSETTINGS, refined.c_str(), 1);
	}
}

int _cdecl main(int argc, char *argv[])
{
	auto ClearArg = [&] (int pos) {
		static char ZeroChar = 0;
		argv[pos] = &ZeroChar;
	};

	Opt.OnlyEditorViewerUsed = Options::INCLUDING_PANELS;
	if (argc > 0) {
		const char *name = strrchr(argv[0], GOOD_SLASH);
		name = name ? name+1 : argv[0];

		if (strcmp(name, "far2medit") == 0) { // run by symlink in editor mode
			Opt.OnlyEditorViewerUsed = Options::ONLY_EDITOR;
			for (int I = 1; I < argc; I++) {
				if (strstr(argv[I], "--") != argv[I]) {
					Opt.strEditViewArg = argv[I];
					ClearArg(I);
					break;
				}
			}
		}
		else if (strcmp(name, "far2m_askpass") == 0)
			return sudo_main_askpass();
		else if (strcmp(name, "far2m_sudoapp") == 0)
			return sudo_main_dispatcher(argc - 1, argv + 1);
		else if (argc > 1) {
			if ((strcasecmp(argv[1], "--help") == 0
					|| strcasecmp(argv[1], "-h") == 0
					|| strcasecmp(argv[1], "-?") == 0)) {
				print_help(name);
				return 0;
			}
			else if (strcmp(argv[1], "--libexec") == 0) {
				return (argc >= 5) ? libexec(argv[2], argv[3], argv[4], argc - 5, argv + 5) : 0;
			}
		}
	}

	unsetenv(ENV_FARSETTINGS); // don't inherit from parent process

	const char *askpass = getenv("SUDO_ASKPASS");
	if (askpass && strstr(askpass, "far2l_askpass")) {
		unsetenv("SUDO_ASKPASS"); // far2m is run from far2l
	}

	// Process -U before WinPortMain():
	// WinPortMain performs early startup / command-line handling and expects
	// FARSETTINGS to already reflect the selected settings identity / path.
	for (int i = 1; i < argc; ++i) {
		if (!strcasecmp(argv[i], "-CD"))
			++i;
		else if (!strcasecmp(argv[i], "-V") || !strncasecmp(argv[i], "-E", 2)) {
			if (++i < argc && !strcmp(argv[i], "-"))
				break;
		}
		else if (!strcasecmp(argv[i], "-U")) {
			ClearArg(i);
			if (++i < argc) {
				SetCustomSettings(argv[i]);
				ClearArg(i);
			}
		}
	}

	setlocale(LC_ALL, "");//otherwise non-latin keys missing with XIM input method

	const char *lcc = getenv("LC_COLLATE");
	if (lcc && *lcc) {
		setlocale(LC_COLLATE, lcc);
	}

	SetupFarPath(argv[0]);

	SCOPED_ACTION(SafeMMap::SignalHandlerRegistrar);

	gMainThreadID = GetInterThreadID();

	return WinPortMain(g_strFarModuleName.GetMB().c_str(), argc, argv, FarAppMain);
}
