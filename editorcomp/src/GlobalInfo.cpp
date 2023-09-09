#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x11ADD930;
  aInfo->Version       = Version;
  aInfo->Title         = L"Editor Autocomplete";
  aInfo->Description   = L"Editor Autocomplete";
  aInfo->Author        = L"MikeMirzayanov, FAR People";
}
