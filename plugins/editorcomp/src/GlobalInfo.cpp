#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(0,0,0,0);
  aInfo->SysID         = 0x11ADD930;
  aInfo->Title         = L"Editor Autocomplete";
  aInfo->Description   = L"Editor Autocomplete";
  aInfo->Author        = L"MikeMirzayanov, FAR People";
  aInfo->UseMenuGuids  = 0;
}
