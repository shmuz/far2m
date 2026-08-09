#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x65642111;
  aInfo->Version       = Version;
  aInfo->Title         = L"ArcLite";
  aInfo->Description   = L"Archive support for Far Manager (based on 7-Zip project)";
  aInfo->Author        = L"Eugene Roshal & Far Group, anta999";
  aInfo->UseMenuGuids  = 0;
}
