#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(0,0,0,0);
  aInfo->SysID         = 0xB076F0B0;
  aInfo->Title         = L"Align";
  aInfo->Description   = L"Align blocks in Far editor";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
  aInfo->UseMenuGuids  = 0;
}
