#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x4F6BDE22;
  aInfo->Version       = Version;
  aInfo->Title         = L"Compare";
  aInfo->Description   = L"Advanced File Compare plugin";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
