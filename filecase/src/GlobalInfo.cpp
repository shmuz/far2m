#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xADAC3050;
  aInfo->Version       = Version;
  aInfo->Title         = L"FileCase";
  aInfo->Description   = L"File names case conversion";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
