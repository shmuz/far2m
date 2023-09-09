#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xFF5449D0;
  aInfo->Version       = Version;
  aInfo->Title         = L"Inside";
  aInfo->Description   = L"Shows what is inside ELF and some other file formats";
  aInfo->Author        = L"elfmz";
}
