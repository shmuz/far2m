# far2m
A fork of far2l (https://github.com/elfmz/far2l)

Linux fork of FAR Manager v2 (http://farmanager.com/)

Works also on OSX/MacOS and BSD (but later not tested on regular manner)

BETA VERSION.
**Use at your own risk!**

#### Included plug-ins
Advanced compare, Align block, Auto wrap, Calculator, Draw lines, EditCase,
Editor autocomplete, FarColorer, FileCase, HlfViewer, Incremental search,
Inside, LuaMacro, MultiArc, NetCfg, NetRocks (SFTP/SCP/FTP/FTPS/SMB/NFS/WebDAV),
Python (optional scripting support), SimpleIndent, TmpPanel.

#### License: GNU/GPLv2

### Used code from projects

* FAR for Windows and some of its plugins
* WINE
* ANSICON
* Portable UnRAR
* 7z ANSI-C Decoder
* utf-cpp by ww898

## Contributing, Hacking
#### Required dependencies

* gawk
* m4
* libwxgtk3.0-gtk3-dev (or libwxgtk3.2-dev in newer distributions, or libwxgtk3.0-dev in older ones, optional - needed for GUI backend, not needed with -DUSEWX=no)
* libx11-dev (optional - needed for X11 extension that provides better UX for TTY backend wherever X11 is available)
* libxi-dev (optional - needed for X11/Xi extension that provides best UX for TTY backend wherever X11 Xi extension is available)
* libxml2-dev (optional - needed for Colorer plugin, not needed with -DCOLORER=no)
* libuchardet-dev
* libssh-dev (optional - needed for NetRocks/SFTP)
* libssl-dev (optional - needed for NetRocks/FTPS)
* libsmbclient-dev (optional - needed for NetRocks/SMB)
* libnfs-dev (optional - needed for NetRocks/NFS)
* libneon27-dev (or later, optional - needed for NetRocks/WebDAV)
* libarchive-dev (optional - needed for better archives support in multiarc)
* libpcre2-dev (needed for custom archives support in multiarc)
* libluajit-5.1-dev
* uuid-dev
* cmake ( >= 3.5 )
* pkg-config
* g++
* git (needed for downloading source code)

#### Or simply on Debian/Ubuntu:
``` sh
apt-get install gawk m4 libwxgtk3.0-gtk3-dev libx11-dev libxi-dev libpcre2-dev libxml2-dev libuchardet-dev libssh-dev libssl-dev libsmbclient-dev libnfs-dev libneon27-dev libarchive-dev libluajit-5.1-dev uuid-dev cmake g++ git

```
In older distributives: use libwxgtk3.0-dev instead of libwxgtk3.0-gtk3-dev

#### Clone and Build
 * Clone current master `git clone https://github.com/shmuz/far2m`
 * Prepare build directory:
``` sh
mkdir -p far2m/_build
cd far2m/_build
```

 * Build:
_with make:_
``` sh
cmake -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc --all)
```
_or with ninja (you need **ninja-build** package installed)_
``` sh
cmake -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release -G Ninja ..
ninja
```

 * If above commands finished without errors - you may also install far2m, _with make:_ `sudo make install` _or with ninja:_ `sudo ninja install`

 * Also its possible to create far2m_2.X.X_ARCH.deb or ...tar.gz packages in `_build` directory by running `cpack` command

##### Additional build configuration options:

To build without WX backend (console version only): change -DUSEWX=yes to -DUSEWX=no also in this case dont need to install libwxgtk\*-dev package

To force-disable TTY|X and TTY|Xi backends: add argument -DTTYX=no; to disable only TTY|Xi - add argument -DTTYXI=no

To eliminate libuchardet requirement to reduce far2m dependencies by cost of losing automatic charset detection functionality: add -DUSEUCD=no

To build with Python plugin: add argument `-DPYTHON=yes`
but you must have installed additional packages within yours system:
python3-dev,
python3-cffi

There're also options to toggle other plugins build in same way: ALIGN AUTOWRAP CALC COLORER COMPARE DRAWLINE EDITCASE EDITORCOMP FARFTP FILECASE INCSRCH INSIDE MULTIARC NETROCKS SIMPLEINDENT TMPPANEL

### Useful 3rd-party extras

 * A collection of plugins and macros for far2m: https://github.com/shmuz/LuaFAR-2M
 * Fork of Putty (Windows SSH client) with added far2l TTY extensions support (fluent keypresses, clipboard sharing etc): https://github.com/unxed/putty4far2l
 * Similar fork of Kitty: https://github.com/mihmig/KiTTY
 * Tool to import color schemes from windows FAR manager 2 .reg format: https://github.com/unxed/far2l-deb/blob/master/far2l_import.pl

## Notes on porting

I (elfmz) implemented/borrowed from WINE some commonly used WinAPI functions. They are all declared in WinPort/WinPort.h and corresponding defines can be found in WinPort/WinCompat.h (both are included by WinPort/windows.h). Note that this stuff may not be 1-to-1 to corresponding Win32 functionality also doesn't provide full-UNIX functionality, but it simplifies porting and can be considered as temporary scaffold.

However, only the main executable is linked statically to WinPort, although it also _exports_ WinPort functionality, so plugins use it without the neccessity to bring their own copies of this code. This is the reason that each plugin's binary should not statically link to WinPort.

While FAR internally is UTF16 (because WinPort contains UTF16-related stuff), native Linux wchar_t size is 4 bytes (rather than 2 bytes) so potentially Linux FAR may be fully UTF32-capable console interaction in the future, but while it uses Win32-style UTF16 functions it does not. However, programmers need to be aware that wchar_t is not 2 bytes long anymore.

Inspect all printf format strings: unlike Windows, in Linux both wide and multibyte printf-like functions have the same multibyte and wide specifiers. This means that %s is always multibyte while %ls is always wide. So, any %s used in wide-printf-s or %ws used in any printf should be replaced with %ls.

Update from 27aug: now it's possible by defining WINPORT_DIRECT to avoid renaming used Windows API and also to avoid changing format strings as swprintf will be intercepted by a compatibility wrapper.

## Plugin API

Plugins API based on FAR Manager v2 plus following changes:

### Added following entries to FarStandardFunctions:

* `int Execute(const wchar_t *CmdStr, unsigned int ExecFlags);`
...where ExecFlags - combination of values of EXECUTEFLAGS.
Executes given command line, if EF_HIDEOUT and EF_NOWAIT are not specified then command will be executed on far2l virtual terminal.

* `int ExecuteLibrary(const wchar_t *Library, const wchar_t *Symbol, const wchar_t *CmdStr, unsigned int ExecFlags)`
Executes given shared library symbol in separate process (process creation behaviour is the same as for Execute).
symbol function must be defined as: `int 'Symbol'(int argc, char *argv[])`

* `void DisplayNotification(const wchar_t *action, const wchar_t *object);`
Shows (depending on settings - always or if far2l in background) system shell-wide notification with given title and text.

* `int DispatchInterThreadCalls();`
far2l supports calling APIs from different threads by marshalling API calls from non-main threads into main one and dispatching them on main thread at certain known-safe points inside of dialog processing loops. DispatchInterThreadCalls() allows plugin to explicitely dispatch such calls and plugin must use it periodically in case it blocks main thread with some non-UI activity that may wait for other threads.

* `void BackgroundTask(const wchar_t *Info, BOOL Started);`
If plugin implements tasks running in background it may invoke this function to indicate about pending task in left-top corner.
Info is a short description of task or just its owner and must be same string when invoked with Started TRUE or FALSE.

### Added following commands into FILE_CONTROL_COMMANDS:
* `FCTL_GETPANELPLUGINHANDLE`
Can be used to interract with plugin that renders other panel.
`hPlugin` can be set to `PANEL_ACTIVE` or `PANEL_PASSIVE`.
`Param1` ignored.
`Param2` points to value of type `HANDLE`, call sets that value to handle of plugin that renders specified panel or `INVALID_HANDLE_VALUE`.

### Added following plugin-exported functions:
* `int MayExitFARW();`
far2l asks plugin if it can exit now. If plugin has some background tasks pending it may block exiting of far2l, however it highly recommended to give user choice using UI prompt.

### Added following dialog messages:
* `DM_GETCOLOR` - retrieves get current color attributes of selected dialog item
* `DM_SETCOLOR` - changes current color attributes of selected dialog item

## Known issues:
* Only valid translations are English, Russian and Ukrainian, all other languages require deep correction.
* Characters that occupy more than single cell or diacritic-like characters are rendered buggy, that means Chinese and Japanese texts are hardly readable in some cases.
