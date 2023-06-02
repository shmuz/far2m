#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x11ADD930;
  aInfo->MinFarVersion = 0x00020004;
  aInfo->Version       = 0;
  aInfo->Title         = L"Editor Autocomplete";
  aInfo->Description   = L"Editor Autocomplete";
  aInfo->Author        = L"MikeMirzayanov, FAR People";
}
