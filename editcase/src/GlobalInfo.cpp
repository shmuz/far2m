#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x0E92FC81;
  aInfo->Version       = Version;
  aInfo->Title         = L"EditCase";
  aInfo->Description   = L"Text case conversion";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
  aInfo->UseMenuGuids  = 0;
}
