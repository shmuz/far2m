#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x11ADD930;
  aInfo->Version       = MAKEPLUGVERSION(0,0,0,0);
  aInfo->Title         = L"Editor Autocomplete";
  aInfo->Description   = L"Editor Autocomplete";
  aInfo->Author        = L"MikeMirzayanov, FAR People";
}
