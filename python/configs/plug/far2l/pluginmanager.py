import os
import os.path
import sys
import types
import configparser
import logging
import logging.config

USERHOME = os.path.expanduser("~/.config/far2m/plugins/python")

logging.basicConfig(level=logging.INFO)


def setup():
    if os.path.isdir(USERHOME):
        sys.path.insert(1, USERHOME)
        fname = os.path.join(USERHOME, "logger.ini")
        if os.path.isfile(fname):
            with open(fname, "rt") as fp:
                ini = configparser.ConfigParser()
                ini.read_file(fp)
                logging.config.fileConfig(ini)


setup()

log = logging.getLogger(__name__)
log.debug("%s start" % ("*" * 20))
log.debug("sys.path={0}".format(sys.path))
log.debug("userhome={0}".format(USERHOME))


def handle_exception(exc_type, exc_value, exc_traceback):
    if issubclass(exc_type, KeyboardInterrupt):
        sys.__excepthook__(exc_type, exc_value, exc_traceback)
        return

    log.error("Uncaught exception", exc_info=(exc_type, exc_value, exc_traceback))


def handle_error(func):
    def __inner(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except Exception:
            exc_type, exc_value, exc_tb = sys.exc_info()
            handle_exception(exc_type, exc_value, exc_tb)
    return __inner

from .plugin import PluginBase

# commands in shell window:
#     py:load <python modulename>
#     py:unload <registered python module name>

from .far2lcffi import ffi, ffic


class PluginManager:
    Info = None

    def __init__(self):
        self.openplugins = {}
        self.plugins = []

    @handle_error
    def SetStartupInfo(self, Info):
        log.debug("SetStartupInfo({0:08X})".format(Info))
        self.info = ffi.cast("struct PluginStartupInfo *", Info)
        self.ffi = ffi
        self.ffic = ffic
        fname = os.path.join(USERHOME, "plugins.ini")
        if os.path.isfile(fname):
            with open(fname, "rt") as fp:
                ini = configparser.ConfigParser()
                ini.read_file(fp)
                if ini.has_section('autoload'):
                    for name in ini.options('autoload'):
                        self.pluginInstall(name, ini.get('autoload', name)=='open')

    @handle_error
    def pluginRemove(self, name):
        log.debug("remove plugin: {0}".format(name))
        for hplugin, plugin in self.openplugins.items():
            if plugin.name == name:
                del self.openplugins[hplugin]
                break
        for i in range(len(self.plugins)):
            if self.plugins[i].Plugin.name == name:
                del self.plugins[i]
                del sys.modules[name]
                for i in range(len(self.plugins)):
                    self.plugins[i].Plugin.number = i
                return
        log.error("install plugin: {0} - not installed".format(name))

    @handle_error
    def pluginInstall(self, name, autorun=False):
        log.debug(f"install plugin: {name} autorun:{autorun}")
        for i in range(len(self.plugins)):
            if self.plugins[i].Plugin.name == name:
                log.error("install plugin: {0} - allready installed".format(name))
                return
        # plugin = __import__(name, globals(), locals(), [], 0)
        plugin = __import__(name)
        cls = getattr(plugin, "Plugin", None)
        if type(cls) == type(PluginBase) and issubclass(cls, PluginBase):
            # inject plugin name
            cls.USERHOME = USERHOME
            cls.name = name
            self.plugins.append(plugin)
            for i in range(len(self.plugins)):
                self.plugins[i].Plugin.number = i
            if autorun:
                plugin = plugin.Plugin(self, self.info, ffi, ffic)
                hplugin = id(plugin)
                self.openplugins[hplugin] = plugin
        else:
            log.error("install plugin: {0} - not a far2l python plugin".format(name))
            del sys.modules[name]

    def pluginGet(self, hPlugin):
        v = self.openplugins.get(hPlugin, None)
        if v is None:

            class Nop:
                def __getattr__(self, name):
                    log.debug("Nop.__getattr__({})".format(name))
                    return self

                def __call__(self, *args):
                    log.debug("Nop.__call__({0})".format(args))
                    return None

            v = Nop()
            log.error("unhandled pluginGet{0}".format(hPlugin))
        return v

    @handle_error
    def pluginGetFrom(self, OpenFrom, Item):
        id2name = {
            ffic.OPEN_DISKMENU: "DISKMENU",
            ffic.OPEN_PLUGINSMENU: "PLUGINSMENU",
            ffic.OPEN_FINDLIST: "FINDLIST",
            ffic.OPEN_SHORTCUT: "SHORTCUT",
            ffic.OPEN_COMMANDLINE: "COMMANDLINE",
            ffic.OPEN_EDITOR: "EDITOR",
            ffic.OPEN_DIALOG: "DIALOG",
            ffic.OPEN_VIEWER: "VIEWER",
            ffic.OPEN_FILEPANEL: "FILEPANEL",
        }
        name = id2name[OpenFrom]
        log.debug("pluginGetFrom({0} ({1}), {2})".format(OpenFrom, name, Item))
        if OpenFrom in [ffic.OPEN_DISKMENU, ffic.OPEN_FINDLIST]:
            for plugin in self.plugins:
                openFrom = plugin.Plugin.openFrom
                log.debug(
                    "pluginGetFrom(openok={0}, no={1} : {2})".format(
                        name in openFrom, Item, plugin.Plugin.name
                    )
                )
                if name in openFrom:
                    if not Item:
                        return plugin
                    Item -= 1
        elif OpenFrom == ffic.OPEN_DIALOG:
            plugins = []
            for plugin in self.plugins:
                openFrom = plugin.Plugin.openFrom
                log.debug(
                    "pluginGetFrom(openok={0}, no={1} : {2})".format(
                        name in openFrom, Item, plugin.Plugin.name
                    )
                )
                if name in openFrom:
                    plugins.append(plugin)
            if len(plugins) == 1:
                plugin = plugins[0]
            elif len(plugins) > 1:
                names = [p.name for p in plugins]
                n = self.menu(names, 'Select plugin')
                plugin = plugins[n]
            else:
                return None
            return plugin
        elif Item < len(self.plugins):
            return self.plugins[Item]
        return None

    def menu(self, names, title='', selected=0):
        """
        Simple menu. ``names`` is a list of items. Optional
        ``title`` can be provided.
        """
        items = self.ffi.new('struct FarMenuItem []', len(names))
        refs = []
        for i, name in enumerate(names):
            item = items[i]
            item.Checked = item.Separator = 0
            item.Selected = i == selected
            item.Text = txt = self.s2f(name)
            refs.append(txt)
        title = self.s2f(title)
        NULL = self.ffi.NULL
        return self.info.Menu(self.info.ModuleNumber, -1, -1, 0, 
                              self.ffic.FMENU_AUTOHIGHLIGHT
                              | self.ffic.FMENU_WRAPMODE, title,
                              NULL, NULL, NULL, NULL, items, len(items));

    def s2f(self, s):
        return self.ffi.new("wchar_t []", s)

    def f2s(self, s):
        return self.ffi.string(self.ffi.cast("wchar_t *", s))

    def Message(self, lines):
        _MsgItems = [
            self.s2f("Python"),
            self.s2f(""),
        ]
        for line in lines:
            _MsgItems.append(self.s2f(line))
        _MsgItems.extend(
            [
                self.s2f(""),
                self.s2f("\x01"),
                self.s2f("&Ok"),
            ]
        )
        # log.debug('_msgItems: %s', _MsgItems)
        MsgItems = self.ffi.new("wchar_t *[]", _MsgItems)
        self.info.Message(
            self.info.ModuleNumber,  # GUID
            self.ffic.FMSG_WARNING | self.ffic.FMSG_LEFTALIGN,  # Flags
            self.s2f("Contents"),  # HelpTopic
            MsgItems,  # Items
            len(MsgItems),  # ItemsNumber
            1,  # ButtonsNumber
        )

    # manager API
    @handle_error
    def ExitFAR(self):
        log.debug("ExitFAR()")
        for hplugin, plugin in self.openplugins.items():
            plugin.ExitFAR()

    @handle_error
    def GetMinFarVersion(self):
        log.debug("GetMinFarVersion()")

    @handle_error
    def GetPluginInfo(self, Info):
        # log.debug("GetPluginInfo({0:08X})".format(Info))
        Info = self.ffi.cast("struct PluginInfo *", Info)
        self._DiskItems = []
        self._MenuItems = []
        self._ConfigItems = []
        log.debug("GetPluginInfo")
        for plugin in self.plugins:
            openFrom = plugin.Plugin.openFrom
            if "DISKMENU" in openFrom:
                self._DiskItems.append(self.s2f(plugin.Plugin.label))
            if "PLUGINSMENU" in openFrom:
                self._MenuItems.append(self.s2f(plugin.Plugin.label))
            if plugin.Plugin.Configure is not None:
                self._ConfigItems.append(self.s2f(plugin.Plugin.label))
        self.DiskItems = self.ffi.new("wchar_t *[]", self._DiskItems)
        self.MenuItems = self.ffi.new("wchar_t *[]", self._MenuItems)
        self.ConfigItems = self.ffi.new("wchar_t *[]", self._ConfigItems)
        Info.Flags = self.ffic.PF_EDITOR | self.ffic.PF_VIEWER | self.ffic.PF_DIALOG
        Info.DiskMenuStrings = self.DiskItems
        Info.DiskMenuStringsNumber = len(self._DiskItems)
        Info.PluginMenuStrings = self.MenuItems
        Info.PluginMenuStringsNumber = len(self._MenuItems)
        Info.PluginConfigStrings = self.ConfigItems
        Info.PluginConfigStringsNumber = len(self._ConfigItems)
        self._commandprefix = self.s2f("py")
        Info.CommandPrefix = self._commandprefix

    @handle_error
    def OpenPlugin(self, OpenFrom, Item):
        log.debug("OpenPlugin({0}, {1})".format(OpenFrom, Item))
        if OpenFrom == self.ffic.OPEN_COMMANDLINE:
            line = self.f2s(Item)
            log.debug("cmd:{0}".format(line))
            line = line.lstrip()
            linesplit = line.split(" ", 1)
            if linesplit[0] == "unload":
                if len(linesplit) > 1:
                    self.pluginRemove(linesplit[1])
                else:
                    log.debug("missing plugin name in py:unload <plugin name>")
                    self.Message(
                        [
                            "Usage is:",
                            "py:unload <plugin name>",
                        ]
                    )
            elif linesplit[0] == "load":
                if len(linesplit) > 1:
                    line = line.split()
                    self.pluginInstall(line[1], len(line) >2 and line[2] == 'open')
                else:
                    log.debug("missing plugin name in py:load <plugin name> [open]")
                    self.Message(
                        [
                            "Usage is:",
                            "py:load <plugin name> [open]",
                        ]
                    )
            else:
                for plugin in self.plugins:
                    if plugin.Plugin.HandleCommandLine(linesplit[0]) is True:
                        plugin = plugin.Plugin(self, self.info, ffi, ffic)
                        plugin.CommandLine(line)
                        return
                msg = "no plugin handler for command: {}".format(line)
                log.debug(msg)
                self.Message([msg])
            return
        plugin = self.pluginGetFrom(OpenFrom, Item)
        if plugin is not None:
            plugin = plugin.Plugin(self, self.info, ffi, ffic)
            rc = plugin.OpenPlugin(OpenFrom)
            if rc not in (-1, None):
                rc = id(plugin)
                self.openplugins[rc] = plugin
        else:
            rc = None
        return rc

    @handle_error
    def ClosePlugin(self, hPlugin):
        # log.debug("ClosePlugin %08X" % hPlugin)
        plugin = self.openplugins.get(hPlugin, None)
        if plugin is not None:
            plugin.Close()
            del self.openplugins[hPlugin]

    @handle_error
    def OpenFilePlugin(self, Name, Data, DataSize, OpMode):
        log.debug(
            "OpenFilePlugin({0}, {1}, {2}, {3})".format(Name, Data, DataSize, OpMode)
        )
        rc = None
        for plugin in self.plugins:
            plugin = plugin.Plugin.OpenFilePlugin(self, self.info, ffi, ffic, Name, Data, DataSize, OpMode)
            if plugin is not False:
                rc = id(plugin)
                self.openplugins[rc] = plugin
                break
        return rc

    @handle_error
    def ProcessDialogEvent(self, Event, Param):
        for hplugin, plugin in self.openplugins.items():
            rc = plugin.ProcessDialogEvent(Event, Param)
            if rc:
                return rc
        return 0

    @handle_error
    def ProcessEditorEvent(self, Event, Param):
        for hplugin, plugin in self.openplugins.items():
            rc = plugin.ProcessEditorEvent(Event, Param)
            if rc:
                return rc
        return 0

    @handle_error
    def ProcessEditorInput(self, Rec):
        for hplugin, plugin in self.openplugins.items():
            rc = plugin.ProcessEditorInput(Rec)
            if rc:
                return rc
        return 0

    @handle_error
    def ProcessSynchroEvent(self, Event, Param):
        for hplugin, plugin in self.openplugins.items():
            rc = plugin.ProcessSynchroEvent(Event, Param)
            if rc:
                return rc
        return 0

    @handle_error
    def ProcessViewerEvent(self, Event, Param):
        for hplugin, plugin in self.openplugins.items():
            rc = plugin.ProcessViewerEvent(Event, Param)
            if rc:
                return rc
        return 0

    # common plugin functions
    @handle_error
    def GetOpenPluginInfo(self, hPlugin, OpenInfo):
        plugin = self.pluginGet(hPlugin)
        return plugin.GetOpenPluginInfo(OpenInfo)

    @handle_error
    def Configure(self, ItemNumber):
        log.debug("Configure({0})".format(ItemNumber))
        for plugin in self.plugins:
            if plugin.Plugin.Configure is not None:
                if ItemNumber == 0:
                    plugin = plugin.Plugin(self, self.info, ffi, ffic)
                    plugin.Configure()
                    return
                ItemNumber -= 1

    @handle_error
    def ProcessEvent(self, hPlugin, Event, Param):
        # log.debug("ProcessEvent({0}, {1}, {2})".format(hPlugin, Event, Param))
        plugin = self.pluginGet(hPlugin)
        return plugin.ProcessEvent(Event, Param)

    @handle_error
    def ProcessKey(self, hPlugin, Key, ControlState):
        # log.debug("ProcessKey({0}, {1}, {2})".format(hPlugin, Key, ControlState))
        plugin = self.pluginGet(hPlugin)
        return plugin.ProcessKey(Key, ControlState)

    # VFS functions
    @handle_error
    def Compare(self, hPlugin, PanelItem1, PanelItem2, Mode):
        plugin = self.pluginGet(hPlugin)
        return plugin.Compare(PanelItem1, PanelItem2, Mode)

    @handle_error
    def DeleteFiles(self, hPlugin, PanelItem, ItemsNumber, OpMode):
        plugin = self.pluginGet(hPlugin)
        return plugin.DeleteFiles(PanelItem, ItemsNumber, OpMode)

    @handle_error
    def FreeFindData(self, hPlugin, PanelItem, ItemsNumber):
        plugin = self.pluginGet(hPlugin)
        return plugin.FreeFindData(PanelItem, ItemsNumber)

    @handle_error
    def FreeVirtualFindData(self, hPlugin, PanelItem, ItemsNumber):
        plugin = self.pluginGet(hPlugin)
        return plugin.FreeVirtualFindData(PanelItem, ItemsNumber)

    @handle_error
    def GetFiles(self, hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode):
        plugin = self.pluginGet(hPlugin)
        return plugin.GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode)

    @handle_error
    def GetFindData(self, hPlugin, PanelItem, ItemsNumber, OpMode):
        plugin = self.pluginGet(hPlugin)
        return plugin.GetFindData(PanelItem, ItemsNumber, OpMode)

    @handle_error
    def GetVirtualFindData(self, hPlugin, PanelItem, ItemsNumber, Path):
        plugin = self.pluginGet(hPlugin)
        return plugin.GetVirtualFindData(PanelItem, ItemsNumber, Path)

    @handle_error
    def MakeDirectory(self, hPlugin, Name, OpMode):
        plugin = self.pluginGet(hPlugin)
        return plugin.MakeDirectory(Name, OpMode)

    @handle_error
    def ProcessHostFile(self, hPlugin, PanelItem, ItemsNumber, OpMode):
        plugin = self.pluginGet(hPlugin)
        return plugin.ProcessHostFile(PanelItem, ItemsNumber, OpMode)

    @handle_error
    def PutFiles(self, hPlugin, PanelItem, ItemsNumber, Move, SrcPath, OpMode):
        plugin = self.pluginGet(hPlugin)
        return plugin.PutFiles(PanelItem, ItemsNumber, Move, SrcPath, OpMode)

    @handle_error
    def SetDirectory(self, hPlugin, Dir, OpMode):
        plugin = self.pluginGet(hPlugin)
        return plugin.SetDirectory(Dir, OpMode)

    @handle_error
    def SetFindList(self, hPlugin, PanelItem, ItemsNumber):
        plugin = self.pluginGet(hPlugin)
        return plugin.SetFindList(PanelItem, ItemsNumber)

    @handle_error
    def MayExitFAR(self):
        #log.debug("MayExitFAR(): %s", self.openplugins.items())
        for hplugin, plugin in self.openplugins.items():
            log.debug(plugin.label)
            if not plugin.MayExitFAR():
                return 0
        return 1

    # not exposed from CPython
    @handle_error
    def Analyse(self, pData):
        #pData = AnalyseData
        return 0

    # not exposed from CPython
    @handle_error
    def GetCustomData(self, FilePath, CustomData):
        return 0

    # not exposed from CPython
    @handle_error
    def FreeCustomData(self, CustomData):
        pass
