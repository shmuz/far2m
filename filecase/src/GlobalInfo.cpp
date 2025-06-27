#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xADAC3050;
  aInfo->Version       = Version;
  aInfo->Title         = L"FileCase";
  aInfo->Description   = L"File names case conversion";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
  aInfo->UseMenuGuids  = 0;
}
