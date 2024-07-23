#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xB77C964B;
  aInfo->Version       = Version;
  aInfo->Title         = L"TmpPanel";
  aInfo->Description   = L"Temporary Panel";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
