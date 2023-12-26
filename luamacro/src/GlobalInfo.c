#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 3,0,0,799 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x4EBBEFC8;
  aInfo->Version       = Version;
  aInfo->Title         = L"LuaMacro";
  aInfo->Description   = L"Far macros in Lua";
  aInfo->Author        = L"Shmuel Zeigerman & Far Group";
}
