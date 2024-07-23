#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xDEEC52C3;
  aInfo->Version       = Version;
  aInfo->Title         = L"AutoWrap";
  aInfo->Description   = L"Auto wrap for Far editor";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
