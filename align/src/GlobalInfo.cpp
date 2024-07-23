#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xB076F0B0;
  aInfo->Version       = Version;
  aInfo->Title         = L"Align";
  aInfo->Description   = L"Align blocks in Far editor";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
