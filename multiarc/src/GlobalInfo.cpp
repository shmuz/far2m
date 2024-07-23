#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x14CA31E6;
  aInfo->Version       = Version;
  aInfo->Title         = L"MultiArc";
  aInfo->Description   = L"Archive support plugin";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
